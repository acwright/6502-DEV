#include "StorageCard.h"

StorageCard::StorageCard() {
  this->reset();
}

void StorageCard::begin() {
  if (!SD.exists(ST_STORAGE_FILE_NAME)) {
    File file = SD.open(ST_STORAGE_FILE_NAME, FILE_WRITE);
    uint8_t block[ST_INIT_BLOCK];

    // Written a block at a time - a byte at a time takes hours over 256MB
    memset(block, 0x00, ST_INIT_BLOCK);

    for (uint32_t i = 0; i < (ST_STORAGE_SIZE / ST_INIT_BLOCK); i++) {
      file.write(block, ST_INIT_BLOCK);
    }

    file.close();
  }
  // IDENTIFY is built in memory (generateIdentity()), so an Identity.bin
  // written by an older firmware is no longer read.
}

uint8_t StorageCard::read(uint16_t address) {
  switch(address & 0x0007) {
    case 0x00: // Data Register
      return this->readBuffer();
    case 0x01: // Error Register
      return this->error;
    case 0x02: // Sector Count Register
      return this->sectorCount;
    case 0x03: // LBA0 Register
      return this->lba0;
    case 0x04: // LBA1 Register
      return this->lba1;
    case 0x05: // LBA2 Register
      return this->lba2;
    case 0x06: // LBA3 Register
      return this->lba3;
    case 0x07: // Status Register
      return this->status;
    default:
      return 0x00; // Shouldn't happen
  }
}

void StorageCard::write(uint16_t address, uint8_t value) {
  switch(address & 0x0007) {
    case 0x00: // Data Register
      this->writeBuffer(value);
      break;
    case 0x01: // Feature Register
      this->feature = value;
      break;
    case 0x02: // Sector Count Register
      this->sectorCount = value;
      break;
    case 0x03: // LBA0 Register
      this->lba0 = value;
      break;
    case 0x04: // LBA1 Register
      this->lba1 = value;
      break;
    case 0x05: // LBA2 Register
      this->lba2 = value;
      break;
    case 0x06: // LBA3 Register
      this->lba3 = (value & 0x0F) | 0xE0;
      break;
    case 0x07: // Command Register
      this->command = value;
      this->executeCommand();
      break;
  }
}

void StorageCard::reset() {
  this->bufferIndex = 0x0000;
  this->commandDataSize = ST_SECTOR_SIZE;
  this->sectorOffset = 0;

  this->error = 0x00;
  this->feature = 0x00;
  this->sectorCount = 0x00;
  this->lba0 = 0x00;
  this->lba1 = 0x00;
  this->lba2 = 0x00;
  this->lba3 = 0xE0;
  this->status = ST_STATUS_RDY | ST_STATUS_DSC;
  this->command = 0x00;

  this->isIdentifying = false;
  this->isTransferring = false;

  for (int i = 0; i < 0x200; i++) {
    this->buffer[i] = 0x00;
  }
}

//
// Private
//

void StorageCard::executeCommand() {
  if (!SD.mediaPresent()) {
    // Abort with general error
    this->fail(ST_ERR_AMNF);
    return;
  } else if (this->isTransferring || this->isIdentifying) {
    // A command in the middle of another one's transfer ends that transfer
    // and is refused, as a real card does
    this->fail(ST_ERR_ABRT);
    return;
  }

  // New command so clear errors and flags
  this->status &= ~ST_STATUS_ERR;
  this->status &= ~ST_STATUS_DRQ;
  this->error = 0x00;
  this->commandDataSize = ST_SECTOR_SIZE * (uint32_t) (this->sectorCount ? this->sectorCount : 256); // ATA: 0 is 256
  this->bufferIndex = 0;
  this->sectorOffset = 0;

  switch (this->command) {
    case 0xC0: { // Erase sectors: `count` of them from the LBA
        File file = SD.open(ST_STORAGE_FILE_NAME, FILE_WRITE);

        for (uint32_t s = 0; s < this->transferSectors(); s++) {
          uint32_t sector = this->sectorIndex() + s;

          if (sector >= ST_SECTOR_COUNT) {
            // Past the end of the card so abort with IDNF flag
            this->fail(ST_ERR_ABRT | ST_ERR_IDNF);
            break;
          }
          if (file) {
            file.seek(sector * ST_SECTOR_SIZE);
            for (int i = 0; i < ST_SECTOR_SIZE; i++) {
              file.write((uint8_t) 0x00);
            }
          }
        }
        if (file) file.close();

        break;
      }
    case 0xEC: { // Identify drive
        this->generateIdentity(this->buffer);
        this->commandDataSize = ST_SECTOR_SIZE; // Identify drive fills buffer with 512 bytes of identity data
        this->status |= ST_STATUS_DRQ;
        this->isIdentifying = true;
        break;
      }
    case 0x20: // Read sector
    case 0x21:
      if (!this->sectorValid()) {
        // Not in sector range so abort with IDNF flag
        this->fail(ST_ERR_ABRT | ST_ERR_IDNF);
      } else {
        // Otherwise load the buffer with first sector and set the DRQ flag and continue
        File storage = SD.open(ST_STORAGE_FILE_NAME);

        if (storage) {
          storage.seek(this->sectorIndex() * ST_SECTOR_SIZE);
          for(int i = 0; i < ST_SECTOR_SIZE; i++) {
            this->buffer[i] = storage.read();
          }
          storage.close();
        }

        this->status |= ST_STATUS_DRQ;
        this->isTransferring = true;
      }
      break;
    case 0xEF: // Set features
      break; // We don't support setting features but we accept them without error
    case 0x30: // Write sector
    case 0x31:
      if (!this->sectorValid()) {
        // Not in sector range so abort with IDNF flag
        this->fail(ST_ERR_ABRT | ST_ERR_IDNF);
      } else {
        // Otherwise set the DRQ flag and continue
        this->status |= ST_STATUS_DRQ;
        this->isTransferring = true;
      }
      break;
    default:
      // All other commands are unsupported so abort with error
      this->fail(ST_ERR_ABRT);
      break;
  }
}

// End any transfer with an error, as a real card does: DRQ clears and the
// data register stops moving bytes
void StorageCard::fail(uint8_t error) {
  this->status |= ST_STATUS_ERR;
  this->status &= ~ST_STATUS_DRQ;
  this->error = error;
  this->isTransferring = false;
  this->isIdentifying = false;
  this->bufferIndex = 0;
}

// The sectors this command moves: its count, with 0 meaning 256
uint32_t StorageCard::transferSectors() {
  return this->commandDataSize / ST_SECTOR_SIZE;
}

bool StorageCard::isReading() {
  return this->command == 0x20 || this->command == 0x21;
}

bool StorageCard::isWriting() {
  return this->command == 0x30 || this->command == 0x31;
}

uint8_t StorageCard::readBuffer() {
  if (this->isIdentifying) {
    uint8_t data = this->buffer[this->bufferIndex];

    if (this->bufferIndex < this->commandDataSize - 1) {
      this->bufferIndex++;
    } else {
      this->bufferIndex = 0;
      this->isIdentifying = false;
      this->status &= ~ST_STATUS_DRQ;
    }

    return data;
  } else if (this->isTransferring && this->isReading()) {
    uint8_t data = this->buffer[this->bufferIndex];

    if (this->bufferIndex < ST_SECTOR_SIZE - 1) {
      this->bufferIndex++;
    } else {
      this->bufferIndex = 0;
      this->sectorOffset++;

      if (this->sectorOffset < this->transferSectors()) { // Load the next sector
        uint32_t sector = this->sectorIndex() + this->sectorOffset;

        if (sector >= ST_SECTOR_COUNT) {
          // Past the end of the card
          this->fail(ST_ERR_ABRT | ST_ERR_IDNF);
        } else {
          File storage = SD.open(ST_STORAGE_FILE_NAME);

          if (storage) {
            storage.seek(sector * ST_SECTOR_SIZE);
            for(int i = 0; i < ST_SECTOR_SIZE; i++) {
              this->buffer[i] = storage.read();
            }
            storage.close();
          }
        }
      } else {
        this->isTransferring = false;
        this->status &= ~ST_STATUS_DRQ;
      }
    }

    return data;
  } else {
    return 0x00;
  }
}

void StorageCard::writeBuffer(uint8_t value) {
  // Only a write command's transfer takes data: outside one (DRQ clear) a real
  // card ignores the data register
  if (!this->isTransferring || !this->isWriting()) return;

  this->buffer[this->bufferIndex] = value;

  if (this->bufferIndex < ST_SECTOR_SIZE - 1) {
    this->bufferIndex++;
  } else {
    this->bufferIndex = 0;

    // Write the current sector to disk
    File file = SD.open(ST_STORAGE_FILE_NAME, FILE_WRITE);

    if (file) {
      file.seek((this->sectorIndex() + this->sectorOffset) * ST_SECTOR_SIZE);
      for (int i = 0; i < ST_SECTOR_SIZE; i++) {
        file.write(buffer[i]);
      }
      file.close();
    }

    this->sectorOffset++;

    // Check if all sectors have been written, or the next is past the end
    if (this->sectorOffset >= this->transferSectors()) {
      this->isTransferring = false;
      this->status &= ~ST_STATUS_DRQ;
    } else if (this->sectorIndex() + this->sectorOffset >= ST_SECTOR_COUNT) {
      this->fail(ST_ERR_ABRT | ST_ERR_IDNF);
    }
  }
}

uint32_t StorageCard::sectorIndex() {
  return ((this->lba3 & 0x0F) << 24) | (this->lba2 << 16) | (this->lba1 << 8) | this->lba0;
}

bool StorageCard::sectorValid() {
  return this->sectorIndex() < ST_SECTOR_COUNT;
}

// An IDENTIFY string over `words` words, space padded. ATA puts the first
// character of each pair in the word's high byte, so it is sent second: the
// pair "AC" goes out as 'C', 'A', and a driver swaps each word back.
void StorageCard::identityText(uint8_t *identity, int word, int words, const char *text) {
  size_t length = strlen(text);

  for (int i = 0; i < words * 2; i++) {
    identity[word * 2 + (i ^ 1)] = (size_t) i < length ? (uint8_t) text[i] : ' ';
  }
}

void StorageCard::generateIdentity(uint8_t *identity) {
  // Generate emulated 256MB CF card identity, laid out as a real card sends
  // it: every word low byte first
  // Some data taken from real Promaster 128MB CF card
  // Geometry is 2048 cylinders x 8 heads x 32 sectors = 524288 sectors (256MB)

  // Fill with zeros first
  memset(identity, 0, ST_SECTOR_SIZE);

  identity[0]   = 0x8A;
  identity[1]   = 0x84; // General configuration $848A: a CompactFlash card
  identity[2]   = 0x00;
  identity[3]   = 0x08; // # of cylinders
  identity[4]   = 0x00;
  identity[5]   = 0x00; // Reserved
  identity[6]   = 0x08;
  identity[7]   = 0x00; // # of heads
  identity[8]   = 0x00;
  identity[9]   = 0x40; // # of unformatted bytes per track
  identity[10]  = 0x00;
  identity[11]  = 0x02; // # of unformatted bytes per sector
  identity[12]  = 0x20;
  identity[13]  = 0x00; // # of sectors per track
  identity[14]  = 0x08;
  identity[15]  = 0x00;
  identity[16]  = 0x00;
  identity[17]  = 0x00; // # of sectors per card, high word first
  identity[18]  = 0x00;
  identity[19]  = 0x00; // Reserved

  this->identityText(identity, 10, 10, "ACWD6502EMUCF1010101"); // Serial #

  identity[40]  = 0x01;
  identity[41]  = 0x00; // Buffer type
  identity[42]  = 0x04;
  identity[43]  = 0x00; // Buffer size in 512 byte increments
  identity[44]  = 0x04;
  identity[45]  = 0x00; // # of ECC bytes passed

  this->identityText(identity, 23, 4, "1.0");            // Firmware revision
  this->identityText(identity, 27, 20, "ACWD6502EMUCF"); // Model #

  identity[94]  = 0x01;
  identity[95]  = 0x00; // Max of 1 sector on R/W multiple cmd
  identity[96]  = 0x00;
  identity[97]  = 0x00; // Double word not supported
  identity[98]  = 0x00;
  identity[99]  = 0x02; // Capabilites: LBA supported
  identity[100] = 0x00;
  identity[101] = 0x00; // Reserved
  identity[102] = 0x00;
  identity[103] = 0x02; // PIO data transfer
  identity[104] = 0x00;
  identity[105] = 0x00; // DMA transfer cycle not supported
  identity[106] = 0x01;
  identity[107] = 0x00; // Field validity
  identity[108] = 0x00;
  identity[109] = 0x08; // Current # of cylinders
  identity[110] = 0x08;
  identity[111] = 0x00; // Current # of heads
  identity[112] = 0x20;
  identity[113] = 0x00; // Current sectors per track
  identity[114] = 0x00;
  identity[115] = 0x00;
  identity[116] = 0x08;
  identity[117] = 0x00; // Current capacity in sectors (LBAs)
  identity[118] = 0x01;
  identity[119] = 0x01; // Multiple sector setting
  identity[120] = 0x00;
  identity[121] = 0x00;
  identity[122] = 0x08;
  identity[123] = 0x00; // Total # of sectors in LBA mode

  // All zeros from here on out
}
