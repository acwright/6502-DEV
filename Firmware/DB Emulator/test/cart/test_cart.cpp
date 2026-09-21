// The Cart class against the rollout's shared fixtures.
//
// 6502-VCS PLAN.md §§1-4 is the normative contract and
// `Firmware/FH Programmer/host/test/fixtures/` is the shared oracle: this test,
// 6502-EMULATOR's, the PicoCalc port's and `6502-flash`'s own all check against
// **those files** rather than against each other, which is the only way four
// implementations of one contract end up agreeing rather than sharing a bug.
//
// The fixtures' checksums are quoted below, so a stale copy fails here rather
// than passing against the wrong bytes.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Cart.h"

SDClass SD;
uint8_t external_psram_size = 8;

static int failures = 0;
static int checks = 0;

static void check(bool ok, const char *what) {
  checks++;
  if (!ok) {
    failures++;
    printf("  FAIL  %s\n", what);
  }
}

static void checkByte(uint8_t got, uint8_t want, const char *what) {
  checks++;
  if (got != want) {
    failures++;
    printf("  FAIL  %s: got $%02X, wanted $%02X\n", what, got, want);
  }
}

static const char *SCRATCH = "build";

static bool copyFile(const char *from, const char *to) {
  FILE *in = fopen(from, "rb");
  if (!in) return false;
  FILE *out = fopen(to, "wb");
  if (!out) { fclose(in); return false; }
  uint8_t buffer[4096];
  size_t got;
  while ((got = fread(buffer, 1, sizeof(buffer), in)) > 0) fwrite(buffer, 1, got, out);
  fclose(in);
  fclose(out);
  return true;
}

static uint32_t crcOf(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  uint32_t crc = 0xFFFFFFFFu;
  int c;
  while ((c = fgetc(f)) != EOF) {
    crc ^= (uint8_t) c;
    for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t) (-(int32_t) (crc & 1)));
  }
  fclose(f);
  return ~crc;
}

// The JEDEC unlock addresses are compared against the translated flash address
// masked to 15 bits, so they are only reachable from the banks that put them
// there: CPU $D555 in bank 2 translates to flash $5555, and CPU $CAAA in bank 1
// to $2AAA. This is the sequence a cartridge actually has to write.
static void unlock(Cart &cart) {
  cart.write(0xE000, 0x02);
  cart.write(0xD555, 0xAA);
  cart.write(0xE000, 0x01);
  cart.write(0xCAAA, 0x55);
  cart.write(0xE000, 0x02);
}

static void programByte(Cart &cart, uint8_t bank, uint16_t addr, uint8_t value) {
  unlock(cart);
  cart.write(0xD555, 0xA0);
  cart.write(0xE000, bank);
  cart.write(addr, value);
}

static void eraseSector(Cart &cart, uint8_t bank, uint16_t addr) {
  unlock(cart);
  cart.write(0xD555, 0x80);
  unlock(cart);
  cart.write(0xE000, bank);
  cart.write(addr, 0x30);
}

static void banner(const char *what) { printf("%s\n", what); }

int main(void) {
  char crt[256], sav[256], flat[256];
  snprintf(crt, sizeof(crt), "%s/banked-128K.crt", SCRATCH);
  snprintf(sav, sizeof(sav), "%s/banked-128K.sav", SCRATCH);
  snprintf(flat, sizeof(flat), "%s/legacy.crt", SCRATCH);

  // The fixtures are the oracle; a stale copy fails here rather than passing
  // against the wrong bytes.
  check(crcOf("fixtures/banked-128K.crt") == 0xE0E4F83Cu, "banked-128K.crt is e0e4f83c");
  check(crcOf("fixtures/legacy.crt") == 0x98A4E0FDu, "legacy.crt is 98a4e0fd");
  check(crcOf("fixtures/sample.sav") == 0x45C4068Eu, "sample.sav is 45c4068e");

  if (!copyFile("fixtures/banked-128K.crt", crt) || !copyFile("fixtures/legacy.crt", flat)) {
    printf("could not stage the fixtures into %s/\n", SCRATCH);
    return 1;
  }
  remove(sav);

  banner("the flat 32K cart is untouched");
  {
    Cart cart;
    check(cart.load(flat) == CART_OK, "legacy.crt loads");
    check(cart.size() == CART_SIZE_FLAT, "legacy.crt is 32,768 bytes");
    check(!cart.banked(), "legacy.crt is not banked");
    check(!cart.nameMismatch(), "a name with no target suffix claims the flat cart");
    // $C000-$C00F is ASCII, and everything else in the cartridge half echoes
    // the high byte of its own CPU address.
    for (int i = 0; i < 16; i++) {
      checkByte(cart.read(0xC000 + i), (uint8_t) "AC6502 LEGACY C0"[i], "legacy $C000 tag");
    }
    checkByte(cart.read(0xD123), 0xD1, "legacy $D123 echoes its own high byte");
    checkByte(cart.read(0xFFFC), 0x00, "legacy RESET vector low");
    checkByte(cart.read(0xFFFD), 0xE0, "legacy RESET vector high");
    // A ROM cart has no WEB: a write goes nowhere, including one that would be
    // a bank register latch on a banked cart.
    cart.write(0xE000, 0x05);
    checkByte(cart.read(0xD123), 0xD1, "a write to a ROM cart changes nothing");
  }

  banner("§2 the mapper contract");
  {
    Cart cart;
    check(cart.load(crt) == CART_OK, "banked-128K.crt loads");
    check(cart.size() == CART_SIZE_128K, "it is 131,072 bytes");
    check(cart.banked(), "it is banked");
    check(strcmp(cartTarget(cart.size()), "128K") == 0, "its target is 128K");
    check(cart.nameMismatch() == false, "the name agrees with the size");

    // Every byte of a bank is that bank's own number, so reading the wrong bank
    // is a wrong byte rather than a hang.
    for (uint8_t reg = 0x00; reg <= 0x0E; reg++) {
      cart.write(0xE000, reg);
      checkByte(cart.read(0xD000), reg, "an ordinary bank select");
      checkByte(cart.bank(), reg, "the register reads back");
    }

    // A 010A has no A17/A18, so high bank bits alias down.
    cart.write(0xE000, 0x10);
    checkByte(cart.read(0xD000), 0x00, "$10 aliases to bank 0");
    cart.write(0xE000, 0x2E);
    checkByte(cart.read(0xD000), 0x0E, "$2E aliases to bank 14");

    // Bit 7 is latched and goes nowhere: 128-255 mirror 0-127.
    cart.write(0xE000, 0x85);
    checkByte(cart.read(0xD000), 0x05, "$85 mirrors bank 5");
    checkByte(cart.bank(), 0x85, "and the register still holds $85");

    // B6 selects U2, which a 128K cart has not got: open bus, modelled as $FF.
    cart.write(0xE000, 0x40);
    checkByte(cart.read(0xD000), 0xFF, "the absent U2 reads open bus");
    checkByte(cart.read(0xC000), 0xFF, "open bus all the way across the window");

    // The fixed region ignores the register entirely, which is what keeps the
    // vectors reachable from every bank.
    for (uint16_t reg = 0x00; reg <= 0xFF; reg += 0x11) {
      cart.write(0xE000, (uint8_t) reg);
      checkByte(cart.read(0xF000), 0x0F, "the fixed region ignores the register");
    }
    cart.write(0xE000, 0x00);
    for (int i = 0; i < 16; i++) {
      checkByte(cart.read(0xE000 + i), (uint8_t) "AC6502 FIXED $0F"[i], "the fixed bank's tag");
    }

    // The 16-byte tags at each end of a bank catch an offset error inside one.
    cart.write(0xE000, 0x03);
    for (int i = 0; i < 16; i++) {
      checkByte(cart.read(0xC000 + i), (uint8_t) "AC6502 BANK $03\n"[i], "bank 3's opening tag");
    }
    for (int i = 0; i < 16; i++) {
      checkByte(cart.read(0xDFF0 + i), (uint8_t) "END OF BANK $03\n"[i], "bank 3's closing tag");
    }

    // RESB clears the register. A cart that boots once and not twice is the
    // symptom of forgetting it.
    cart.write(0xE000, 0x07);
    cart.reset();
    checkByte(cart.bank(), 0x00, "reset clears the bank register");
    checkByte(cart.read(0xD000), 0x00, "and the window is bank 0 again");
  }

  banner("§3 the flash command set");
  {
    Cart cart;
    check(cart.load(crt) == CART_OK, "banked-128K.crt loads");

    // The mitigation the plan asks for by name: a byte programmed into the live
    // window has to read straight back, or the CPU is reading stale bytes out
    // of the RAM cache and a save appears to work and then does not.
    //
    // $F0 as the value is deliberate: it is also the reset-to-read-mode
    // command, and it must not be taken as one while a program is pending.
    programByte(cart, 0x0E, 0xC010, 0xF0);
    cart.write(0xE000, 0x0E);
    checkByte(cart.read(0xC010), 0x00, "a byte programmed into the live window reads back");
    checkByte(cart.read(0xC011), 0x0E, "and its neighbour is untouched");

    // Byte program clears bits only. A program that expects a 0 bit back as a 1
    // without an erase is a bug on hardware, and has to be one here too.
    programByte(cart, 0x0E, 0xC010, 0xFF);
    cart.write(0xE000, 0x0E);
    checkByte(cart.read(0xC010), 0x00, "program clears bits only");

    // Sector erase: 4 KB to $FF, on a 4 KB boundary. Bank 14 is flash
    // $1C000-$1DFFF, so $C010 is in sector 28 and $D010 in sector 29.
    programByte(cart, 0x0E, 0xD010, 0x00);
    eraseSector(cart, 0x0E, 0xC010);
    cart.write(0xE000, 0x0E);
    checkByte(cart.read(0xC010), 0xFF, "the erased sector is $FF");
    checkByte(cart.read(0xCFFF), 0xFF, "to its last byte");
    checkByte(cart.read(0xD000), 0x0E, "and the next sector is untouched");
    checkByte(cart.read(0xD010), 0x00, "including the byte programmed into it");

    // Software ID: flash xx00h is the manufacturer, xx01h the device. An
    // SST39SF010A is $BF/$B5, derived from CHIP_SIZE so that this and `id` over
    // the Flash Helper agree.
    unlock(cart);
    cart.write(0xD555, 0x90);
    cart.write(0xE000, 0x03);
    checkByte(cart.read(0xC000), 0xBF, "ID mode: the manufacturer");
    checkByte(cart.read(0xC001), 0xB5, "ID mode: an SST39SF010A");
    checkByte(cart.read(0xE000), 0xBF, "ID mode answers in the fixed region too");
    cart.write(0xC000, 0xF0);
    checkByte(cart.read(0xC000), (uint8_t) 'A', "and $F0 leaves it");
    checkByte(cart.read(0xE000), (uint8_t) 'A', "in both windows");

    // The cache invalidation that the fixed region makes possible: bank 15 is
    // the fixed bank, so with the register on $0F the same 8 KB is visible
    // through both windows at once and a program has to land in both.
    cart.write(0xE000, 0x0F);
    checkByte(cart.read(0xC010), cart.read(0xE010), "bank 15 is the fixed bank");
    programByte(cart, 0x0F, 0xC010, 0x00);
    cart.write(0xE000, 0x0F);
    checkByte(cart.read(0xC010), 0x00, "programmed through the window");
    checkByte(cart.read(0xE010), 0x00, "and visible in the fixed region");

    // A bare write outside a sequence is swallowed.
    cart.write(0xE000, 0x03);
    cart.write(0xC020, 0x00);
    checkByte(cart.read(0xC020), 0x03, "a bare flash write does nothing");
  }

  banner("§4 the .sav sidecar");
  {
    // sample.sav holds sectors 0, 28 and 29 of banked-128K.crt, handed to the
    // writer out of order so that a reader proves records come back ascending.
    check(copyFile("fixtures/sample.sav", sav), "sample.sav staged beside the cart");

    Cart cart;
    check(cart.load(crt) == CART_OK, "the cart loads with a sidecar beside it");
    check(cart.save() == CART_SAVE_APPLIED, "the sidecar belongs to this build and is applied");
    check(cart.dirty(), "an applied overlay counts as dirty, or it would be thrown away");

    // Each sector opens with an ASCII line naming itself, so a misplaced sector
    // is visible immediately. Sector 28 is bank 14's first 4 KB.
    cart.write(0xE000, 0x0E);
    check(cart.read(0xC000) != 0x0E, "sector 28 is no longer the fixture's fill");

    // Writing it straight back must reproduce the file byte for byte: same
    // header, same three records, ascending.
    check(cart.saveDirty(), "the overlay writes back");
    check(crcOf(sav) == 0x45C4068Eu, "and it is sample.sav, byte for byte");

    // A sidecar whose image_crc does not match is the ordinary consequence of
    // rebuilding a cartridge. It is reported, the cart starts without it, and
    // the file is left on the card.
    FILE *f = fopen(sav, "r+b");
    check(f != nullptr, "the sidecar can be re-opened");
    fseek(f, 12, SEEK_SET);
    uint8_t wrong[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    fwrite(wrong, 1, 4, f);
    fclose(f);

    Cart other;
    check(other.load(crt) == CART_OK, "the cart still loads");
    check(other.save() == CART_SAVE_MISMATCH, "a save from a different build is refused");
    other.write(0xE000, 0x0E);
    checkByte(other.read(0xC000), (uint8_t) 'A', "and none of it was applied");
    check(!other.dirty(), "so there is nothing to write back over it");
    check(crcOf(sav) != 0, "and the file is still on the card");
  }

  remove(sav);

  printf("\n%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
