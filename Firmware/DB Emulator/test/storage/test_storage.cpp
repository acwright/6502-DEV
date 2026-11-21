// lib/6502/IO/StorageCard, the CompactFlash model, against what a real card does.
//
// The reference is the SD controller firmware that answers on the bus as a
// card does (6502-COB `Firmware/SCP Controller/core/ata.c` and `identify.c`).
// 6502-EMULATOR's Storage.ts and the PicoCalc's storage.c follow the same card,
// with the same checks. Each check below is one of the five places 6502-DEV#1
// found the model differing from a card, or one of the faults found with them.

#include <cstdio>
#include <cstring>

#include "SD.h"
#include "StorageCard.h"

SparseFile storageBin;
SDClass SD;

static int checks, failures;

static void check(bool ok, const char *what) {
  checks++;
  if (!ok) { failures++; printf("  FAIL  %s\n", what); }
}

static void checkByte(uint8_t got, uint8_t want, const char *what) {
  checks++;
  if (got != want) { failures++; printf("  FAIL  %s: got $%02X, wanted $%02X\n", what, got, want); }
}

// A fresh card over an empty Storage.bin, as begin() leaves it
static StorageCard *card;

static void fresh() {
  storageBin = SparseFile();
  storageBin.exists = true;
  SD.present = true;
  delete card;
  card = new StorageCard();
}

static void command(uint32_t lba, uint8_t count, uint8_t cmd) {
  card->write(2, count);
  card->write(3, (uint8_t) lba);
  card->write(4, (uint8_t) (lba >> 8));
  card->write(5, (uint8_t) (lba >> 16));
  card->write(6, (uint8_t) (0xE0 | ((lba >> 24) & 0x0F)));
  card->write(7, cmd);
}

static uint8_t status() { return card->read(7); }
static uint8_t image(uint32_t sector, uint32_t offset) { return storageBin.get(sector * 512 + offset); }
static void poke(uint32_t sector, uint32_t offset, uint8_t value) { storageBin.set(sector * 512 + offset, value); }

// ---------------------------------------------------------------------

static void identifyIsLaidOutAsACardSendsIt() {
  fresh();
  card->write(7, 0xEC);
  uint8_t id[512];
  for (int i = 0; i < 512; i++) id[i] = card->read(0);

  checkByte(id[0], 0x8A, "IDENTIFY word 0, low byte ($848A)");
  checkByte(id[1], 0x84, "IDENTIFY word 0, high byte ($848A)");

  // A driver swaps each word of a string back
  struct { int from, words; const char *want, *what; } strings[] = {
    {10, 10, "ACWD6502EMUCF1010101", "serial number, words 10-19"},
    {23, 4, "1.0     ", "firmware revision, words 23-26"},
    {27, 20, "ACWD6502EMUCF                           ", "model, words 27-46"},
  };
  for (auto &s : strings) {
    char text[41];
    int n = s.words * 2;
    for (int i = 0; i < n; i++) text[i] = (char) id[s.from * 2 + (i ^ 1)];
    text[n] = 0;
    if (strcmp(text, s.want) != 0) printf("        got \"%s\"\n", text);
    check(strcmp(text, s.want) == 0, s.what);
  }
  checkByte(id[20], 'C', "the pair \"AC\" is sent C first");
  checkByte(id[21], 'A', "the pair \"AC\" is sent A second");

  uint32_t w7 = id[14] | id[15] << 8, w8 = id[16] | id[17] << 8;
  check((w7 << 16 | w8) == ST_SECTOR_COUNT, "words 7-8, sectors per card, high word first");
  uint32_t w60 = id[120] | id[121] << 8, w61 = id[122] | id[123] << 8;
  check((w61 << 16 | w60) == ST_SECTOR_COUNT, "words 60-61, LBA sectors, low word first");
}

static void readyReadsRdyAndDsc() {
  fresh();
  checkByte(status(), 0x50, "status when ready");
  card->write(7, 0xEF);
  checkByte(status(), 0x50, "status after SET FEATURES");
  card->write(7, 0xFF);
  checkByte(status(), 0x51, "status after an unknown command: DSC kept");
  card->reset();
  checkByte(status(), 0x50, "status after a reset");
}

static void aCountOf0Moves256Sectors() {
  fresh();
  for (uint32_t s = 0; s < 257; s++) { poke(s, 0, (uint8_t) s); poke(s, 1, (uint8_t) (s >> 8)); }
  command(0, 0, 0x20);
  int moved = 0, wrong = 0;
  while ((status() & 0x08) && moved < 300) {
    uint8_t lo = card->read(0), hi = card->read(0);
    if ((lo | hi << 8) != moved) wrong++;
    for (int i = 2; i < 512; i++) card->read(0);
    moved++;
  }
  check(moved == 256, "a read with a count of 0 moves 256 sectors");
  check(wrong == 0, "each sector of the 256 is the right one");

  command(0, 0, 0x30);
  moved = 0;
  while ((status() & 0x08) && moved < 300) {
    for (int i = 0; i < 512; i++) card->write(0, 0xA5);
    moved++;
  }
  check(moved == 256, "a write with a count of 0 takes 256 sectors");
  checkByte(image(255, 511), 0xA5, "the 256th sector written");
  checkByte(image(256, 1), 0x01, "and not the 257th");
}

static void aCommandMidTransferEndsIt() {
  fresh();
  poke(0, 0, 0x11);
  poke(0, 1, 0x22);
  command(0, 1, 0x20);
  checkByte(card->read(0), 0x11, "the read's first byte");
  card->write(7, 0x20);
  checkByte(status(), 0x51, "a command mid-read: ERR, DRQ clear");
  checkByte(card->read(1), 0x04, "a command mid-read: ABRT");
  checkByte(card->read(0), 0x00, "the old read moves no more bytes");
  card->write(7, 0x20);
  checkByte(status(), 0x58, "the next command starts afresh");
  checkByte(card->read(0), 0x11, "from the first byte");

  fresh();
  command(3, 1, 0x30);
  for (int i = 0; i < 100; i++) card->write(0, 0x77);
  card->write(7, 0xEC);
  checkByte(status(), 0x51, "a command mid-write: ERR, DRQ clear");
  for (int i = 0; i < 512; i++) card->write(0, 0x77);
  checkByte(image(3, 0), 0x00, "the old write puts nothing on the card");

  fresh();
  card->write(7, 0xEC);
  card->read(0);
  card->write(7, 0xEF);
  checkByte(status(), 0x51, "a command mid-IDENTIFY: ERR, DRQ clear");
  checkByte(card->read(0), 0x00, "the old IDENTIFY moves no more bytes");
}

static void theDataRegisterIsIgnoredOutsideATransfer() {
  fresh();
  for (int i = 0; i < 512; i++) card->write(0, 0x99);
  checkByte(image(0, 0), 0x00, "512 data writes with no command put nothing on the card");
}

static void eraseErasesCountSectors() {
  fresh();
  for (uint32_t s = 0; s < 300; s++) poke(s, 0, 0xEE);
  command(10, 3, 0xC0);
  checkByte(status(), 0x50, "erase of 3: no error");
  checkByte(image(9, 0), 0xEE, "erase leaves the sector before");
  checkByte(image(10, 0), 0x00, "erase clears the first");
  checkByte(image(12, 0), 0x00, "erase clears the third");
  checkByte(image(13, 0), 0xEE, "erase leaves the sector after");
  command(20, 0, 0xC0);
  checkByte(image(275, 0), 0x00, "erase of 0 clears 256 sectors");
  checkByte(image(276, 0), 0xEE, "and not the 257th");
}

static void aTransferStopsAtTheEndOfTheCard() {
  const uint32_t last = ST_SECTOR_COUNT - 1;
  fresh();
  command(last, 2, 0x20);
  for (int i = 0; i < 512; i++) card->read(0);
  checkByte(status() & 0x09, 0x01, "a read past the end: ERR, DRQ clear");
  checkByte(card->read(1), 0x14, "a read past the end: ABRT and IDNF");

  command(last, 2, 0x30);
  for (int i = 0; i < 1024; i++) card->write(0, 0x55);
  checkByte(status() & 0x09, 0x01, "a write past the end: ERR, DRQ clear");
  checkByte(image(last, 511), 0x55, "the last sector is written");
  check(storageBin.sectors.count(last + 1) == 0, "and nothing past it");

  command(last, 2, 0xC0);
  checkByte(status() & 0x01, 0x01, "an erase past the end: ERR");
  checkByte(image(last, 0), 0x00, "the last sector is erased");
}

static void noMediaIsAGeneralError() {
  fresh();
  SD.present = false;
  command(0, 1, 0x20);
  checkByte(status() & 0x09, 0x01, "no SD card: ERR, DRQ clear");
  checkByte(card->read(1), 0x01, "no SD card: AMNF");
}

int main() {
  identifyIsLaidOutAsACardSendsIt();
  readyReadsRdyAndDsc();
  aCountOf0Moves256Sectors();
  aCommandMidTransferEndsIt();
  theDataRegisterIsIgnoredOutsideATransfer();
  eraseErasesCountSectors();
  aTransferStopsAtTheEndOfTheCard();
  noMediaIsAGeneralError();
  printf("%d checks, %d failed\n", checks, failures);
  delete card;
  return failures ? 1 : 0;
}
