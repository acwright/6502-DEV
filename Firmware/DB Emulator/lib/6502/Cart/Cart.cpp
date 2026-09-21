#include "Cart.h"

// Defined by the Teensy core's startup, in megabytes, and 0 when no PSRAM chip
// is fitted. It has no header of its own.
extern "C" uint8_t external_psram_size;

const char *cartStatusMessage(CartStatus status) {
  switch (status) {
    case CART_OK:          return "Loaded";
    case CART_ERR_OPEN:    return "Cartridge could not be opened";
    case CART_ERR_SIZE:    return "Not a cartridge image size (32K, 128K, 256K, 512K or 1M)";
    case CART_ERR_MEMORY:  return "Not enough memory for this cartridge";
    case CART_ERR_PSRAM:   return "This cartridge needs PSRAM, which is not fitted";
    case CART_ERR_READ:    return "Cartridge could not be read to the end";
    default:               return "Unknown error";
  }
}

const char *cartTarget(uint32_t size) {
  switch (size) {
    case CART_SIZE_FLAT: return "32K";
    case CART_SIZE_128K: return "128K";
    case CART_SIZE_256K: return "256K";
    case CART_SIZE_512K: return "512K";
    case CART_SIZE_1M:   return "1M";
    default:             return nullptr;
  }
}

// The target suffix the file name claims, by §1's spelling: "-VDP" comes first
// and the target is always last, so only the tail is examined. A name with no
// suffix at all claims the flat cart, which is what every cart was called
// before the scheme existed.
static const char *claimedTarget(const char *path) {
  const char *name = path;
  for (const char *p = path; *p; p++) {
    if (*p == '/') name = p + 1;
  }

  // The stem: up to the last dot, if there is one.
  const char *dot = nullptr;
  for (const char *p = name; *p; p++) {
    if (*p == '.') dot = p;
  }
  size_t stem = dot ? (size_t) (dot - name) : strlen(name);

  static const char *suffixes[] = { "-128K", "-256K", "-512K", "-1M" };
  static const char *targets[]  = { "128K", "256K", "512K", "1M" };

  for (uint8_t i = 0; i < 4; i++) {
    size_t len = strlen(suffixes[i]);
    if (stem < len) continue;
    if (strncasecmp(name + stem - len, suffixes[i], len) == 0) return targets[i];
  }
  return "32K";
}

Cart::Cart() {}

Cart::~Cart() {
  this->release();
}

void Cart::release() {
  if (this->data) {
    if (this->inPsram) {
      extmem_free(this->data);
    } else {
      free(this->data);
    }
    this->data = nullptr;
  }
  this->imageSize = 0;
  this->inPsram = false;
  this->nameDisagrees = false;
  this->isBanked = false;
  this->chips = 0;
  this->chipSize = 0;
  this->bankMask = 0;
  this->bankReg = 0;
  this->selChip = 0;
  this->selBank = 0;
  this->deviceId = 0;
  this->seq[0] = this->seq[1] = CART_SEQ_IDLE;
  this->idMode[0] = this->idMode[1] = false;
  memset(this->dirtyBits, 0, sizeof(this->dirtyBits));
}

bool Cart::allocate(uint32_t size) {
  this->inPsram = (size >= CART_PSRAM_FROM);
  this->data = this->inPsram ? (uint8_t *) extmem_malloc(size)
                             : (uint8_t *) malloc(size);
  if (!this->data) {
    this->inPsram = false;
    return false;
  }
  this->imageSize = size;
  return true;
}

// The fixed region is always the **primary** chip's last 8 KB. `chip` never
// applies to it, which is what guarantees the reset vectors cannot be banked
// out from under the CPU.
void Cart::refillFixed() {
  if (!this->isBanked) { return; }
  if (this->idMode[0]) { this->fillId(this->fixedWindow); return; }
  memcpy(this->fixedWindow, this->data + this->chipSize - CART_BANK_SIZE, CART_BANK_SIZE);
}

// A chip in software ID mode answers every read from its identification pair
// rather than from its array: flash xx00h is the manufacturer, xx01h the
// device. Both window bases are even, so the pattern is the same in either.
//
// Painting it into the cache rather than branching on it in read() is what
// keeps the hot path at two branches and an array index. ID mode is entered
// and left by a command, and both ends refill.
void Cart::fillId(uint8_t *window) {
  for (uint32_t i = 0; i < CART_BANK_SIZE; i++) {
    window[i] = (i & 1) ? this->deviceId : 0xBF;
  }
}

// The 8 KB the register currently selects. Selecting the absent U2 — a 512 KB
// image, or a 1 MB board with only one chip fitted — reads open bus, which
// every emulator in the rollout models as $FF.
void Cart::refillWindow() {
  if (!this->isBanked) { return; }

  if (this->selChip >= this->chips) {
    memset(this->bankWindow, 0xFF, CART_BANK_SIZE);
    return;
  }

  if (this->idMode[this->selChip]) { this->fillId(this->bankWindow); return; }

  uint32_t base = (uint32_t) this->selChip * this->chipSize +
                  (uint32_t) this->selBank * CART_BANK_SIZE;
  memcpy(this->bankWindow, this->data + base, CART_BANK_SIZE);
}

// A write anywhere in $E000-$FFFF latches the register and reaches no flash.
//
// The guard on "actually changed" is not an optimisation to be tidied away:
// SetBank in the 6502-CRT cartridge template writes the register
// unconditionally, so a game that calls it in a loop would otherwise copy 8 KB
// per call.
void Cart::latchBank(uint8_t value) {
  this->bankReg = value;

  uint8_t chip = (value >> 6) & 1;
  // The NC high address pins on a 010A or a 020A are exactly this mask, so a
  // register value above the part's bank count aliases down — and bit 7 is
  // latched and goes nowhere, so 128-255 mirror 0-127.
  uint8_t bank = value & this->bankMask;

  if (chip == this->selChip && bank == this->selBank) { return; }

  this->selChip = chip;
  this->selBank = bank;
  this->refillWindow();
}

CartStatus Cart::load(const char *path) {
  if (!SD.mediaPresent()) { return CART_ERR_OPEN; }

  File file = SD.open(path);
  if (!file) { return CART_ERR_OPEN; }

  uint32_t size = (uint32_t) file.size();

  if (!cartTarget(size)) { file.close(); return CART_ERR_SIZE; }

  if (size >= CART_PSRAM_FROM && external_psram_size == 0) {
    file.close();
    return CART_ERR_PSRAM;
  }

  // The old image goes before the new one is asked for, so a 1 MB cart loading
  // over a 1 MB cart needs one megabyte of PSRAM and not two.
  this->release();

  if (!this->allocate(size)) { file.close(); return CART_ERR_MEMORY; }

  // A page at a time rather than a byte at a time: at 1 MB the difference is
  // the whole load.
  uint32_t done = 0;
  while (done < size) {
    uint32_t want = size - done;
    if (want > PAGE_SIZE) { want = PAGE_SIZE; }
    int got = file.read(this->data + done, want);
    if (got <= 0) { break; }
    done += (uint32_t) got;
  }
  file.close();

  if (done != size) { this->release(); return CART_ERR_READ; }

  // §2's derived constants. On a -1M image U1 is the first 512 KB and U2 the
  // second (§1 rule 4), so the file is linear and so is the bank numbering.
  this->isBanked = (size != CART_SIZE_FLAT);
  if (this->isBanked) {
    this->chips = (size > CART_SIZE_512K) ? 2 : 1;
    this->chipSize = size / this->chips;
    this->bankMask = (uint8_t) ((this->chipSize / CART_BANK_SIZE) - 1);
    this->deviceId = (this->chipSize == CART_SIZE_128K) ? 0xB5
                   : (this->chipSize == CART_SIZE_256K) ? 0xB6
                                                        : 0xB7;
    this->reset();
  }

  const char *claimed = claimedTarget(path);
  this->nameDisagrees = (strcmp(claimed, cartTarget(size)) != 0);

  return CART_OK;
}

void Cart::unload() {
  this->release();
  this->file = "None";
}

void Cart::reset() {
  if (!this->isBanked) { return; }

  this->bankReg = 0;
  this->selChip = 0;
  this->selBank = 0;
  this->seq[0] = this->seq[1] = CART_SEQ_IDLE;
  this->idMode[0] = this->idMode[1] = false;
  this->refillFixed();
  this->refillWindow();
}

// The flat cart is the whole 32 KB image indexed from a $8000 origin. Only
// $C000-$FFFF is ever asked for — main.cpp decodes the I/O window and the ROM
// in front of the rest — so the low half is loaded and never read, which is
// exactly what the byte-at-a-time loader's 16 KB skip amounted to.
FASTRUN uint8_t Cart::read(uint32_t addr) {
  if (!this->isBanked) { return this->data[addr - CART_START]; }
  if (addr >= CART_FIXED_BASE) { return this->fixedWindow[addr - CART_FIXED_BASE]; }
  return this->bankWindow[addr - CART_WINDOW_BASE];
}

FASTRUN void Cart::write(uint32_t addr, uint8_t value) {
  // A ROM cart has no WEB. Writes to it go nowhere, as they do on the board.
  if (!this->isBanked) { return; }

  if (addr >= CART_FIXED_BASE) {
    this->latchBank(value);
    return;
  }

  // $C000-$DFFF is a flash write at the translated address, on the chip the
  // register selects. It only does anything as part of a JEDEC command
  // sequence; a bare write outside one is swallowed.
  if (this->selChip >= this->chips) { return; } // U2 is not fitted

  uint32_t at = (uint32_t) this->selBank * CART_BANK_SIZE + (addr - CART_WINDOW_BASE);
  this->command(this->selChip, at, value);
}

// The command set of §3, over one chip's own address space.
//
// Unlock addresses are compared against the translated flash address masked to
// 15 bits, because the SST39SF0x0 only decodes A14-A0 for commands. That falls
// out for free once writes are translated through the same bank mapping as
// reads: CPU $D555 in bank 2 translates to flash $5555, and CPU $CAAA in bank 1
// to $2AAA.
void Cart::command(uint8_t chip, uint32_t at, uint8_t value) {
  uint32_t ua = at & 0x7FFF;
  uint8_t *st = &this->seq[chip];

  // F0 at any address returns the chip to read mode. It is only a command when
  // a program is not pending: the byte a program writes may itself be $F0.
  if (*st != CART_SEQ_PROGRAM && value == 0xF0) {
    bool wasId = this->idMode[chip];
    this->idMode[chip] = false;
    *st = CART_SEQ_IDLE;
    if (wasId) { this->refillFixed(); this->refillWindow(); }
    return;
  }

  switch (*st) {
    case CART_SEQ_IDLE:
      *st = (ua == 0x5555 && value == 0xAA) ? CART_SEQ_UNLOCK1 : CART_SEQ_IDLE;
      return;
    case CART_SEQ_UNLOCK1:
      *st = (ua == 0x2AAA && value == 0x55) ? CART_SEQ_UNLOCK2 : CART_SEQ_IDLE;
      return;
    case CART_SEQ_UNLOCK2:
      if (ua != 0x5555) { *st = CART_SEQ_IDLE; return; }
      switch (value) {
        case 0xA0: *st = CART_SEQ_PROGRAM; return;
        case 0x80: *st = CART_SEQ_ERASE1; return;
        case 0x90:
          this->idMode[chip] = true;
          *st = CART_SEQ_IDLE;
          this->refillFixed();
          this->refillWindow();
          return;
        default: *st = CART_SEQ_IDLE; return;
      }
    case CART_SEQ_PROGRAM:
      *st = CART_SEQ_IDLE;
      this->program(chip, at, value);
      return;
    case CART_SEQ_ERASE1:
      *st = (ua == 0x5555 && value == 0xAA) ? CART_SEQ_ERASE2 : CART_SEQ_IDLE;
      return;
    case CART_SEQ_ERASE2:
      *st = (ua == 0x2AAA && value == 0x55) ? CART_SEQ_ERASE3 : CART_SEQ_IDLE;
      return;
    case CART_SEQ_ERASE3:
      *st = CART_SEQ_IDLE;
      if (ua == 0x5555 && value == 0x10) { this->eraseChip(chip); }
      else if (value == 0x30) { this->eraseSector(chip, at); }
      return;
    default:
      *st = CART_SEQ_IDLE;
      return;
  }
}

// Byte program: **clears bits only**, new = old & data. A program that expects
// a 0 bit to come back as a 1 without an erase is a bug on hardware, and
// reproducing it here is how that bug shows up before the chip is real.
//
// Program and erase complete instantly. The emulated CPU here is free-running,
// so there is no cycle count to hang a busy window on; both standard polling
// loops still terminate, because a read after the write already returns data.
// The consequence is in the README's accuracy notes: a self-programming routine
// that runs from ROM instead of RAM works here and hangs on real hardware.
// 6502-EMULATOR models the window and is where that bug is meant to be caught.
void Cart::program(uint8_t chip, uint32_t at, uint8_t value) {
  uint32_t index = (uint32_t) chip * this->chipSize + at;
  uint8_t stored = this->data[index] & value;
  this->data[index] = stored;
  this->markDirty(index / CART_SECTOR_SIZE);
  this->cacheStore(chip, at, stored);
}

// Sector erase: 4 KB to $FF, aligned on a 4 KB boundary.
void Cart::eraseSector(uint8_t chip, uint32_t at) {
  uint32_t sector = at / CART_SECTOR_SIZE;
  uint32_t index = (uint32_t) chip * this->chipSize + sector * CART_SECTOR_SIZE;
  memset(this->data + index, 0xFF, CART_SECTOR_SIZE);
  this->markDirty(index / CART_SECTOR_SIZE);
  // An erase is rare and is 4 KB wide, so both windows are refilled rather than
  // picked over. Getting this invalidation wrong is the one bug the window
  // cache introduces that a straightforward implementation does not have: the
  // CPU would read back stale bytes out of RAM, and a save would appear to work
  // and then not.
  this->refillFixed();
  this->refillWindow();
}

// Chip erase: the whole part to $FF. Each chip has its own state machine, so
// this is one chip of a -1M pair and not both.
void Cart::eraseChip(uint8_t chip) {
  uint32_t base = (uint32_t) chip * this->chipSize;
  memset(this->data + base, 0xFF, this->chipSize);
  for (uint32_t s = 0; s < this->chipSize / CART_SECTOR_SIZE; s++) {
    this->markDirty(base / CART_SECTOR_SIZE + s);
  }
  this->refillFixed();
  this->refillWindow();
}

// A byte that has just landed in the image has to land in any window that is
// showing it as well. Both can be showing it at once — the fixed region is
// chip 0's last 8 KB, and a register value that selects that same bank puts it
// in the window too — so neither test is an else.
void Cart::cacheStore(uint8_t chip, uint32_t at, uint8_t value) {
  if (chip == 0 && at >= this->chipSize - CART_BANK_SIZE && !this->idMode[0]) {
    this->fixedWindow[at - (this->chipSize - CART_BANK_SIZE)] = value;
  }
  if (chip == this->selChip && !this->idMode[chip]) {
    uint32_t base = (uint32_t) this->selBank * CART_BANK_SIZE;
    if (at >= base && at < base + CART_BANK_SIZE) {
      this->bankWindow[at - base] = value;
    }
  }
}

void Cart::markDirty(uint32_t sector) {
  if (sector >= sizeof(this->dirtyBits) * 8) { return; }
  this->dirtyBits[sector >> 3] |= (uint8_t) (1u << (sector & 7));
}
