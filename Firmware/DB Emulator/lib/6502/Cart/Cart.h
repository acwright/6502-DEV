#ifndef _CART_H
#define _CART_H

#include <Arduino.h>
#include <SD.h>
#include "constants.h"

// A cartridge in the slot: the 32 KB ROM cart this firmware has always taken,
// and the banked Flash Cart.
//
// The contract is 6502-VCS `PLAN.md` §§1-3, which is the normative copy: three
// emulators and one programmer have to agree bit for bit, so the rules live in
// that plan rather than in any one implementation. The section markers below
// cite it.

// The five lengths an image may legally be (§1). The mapper is decided by the
// file's size and never by its name — the name is for whoever is reading `ls`.
#define CART_SIZE_FLAT 0x8000UL
#define CART_SIZE_128K 0x20000UL
#define CART_SIZE_256K 0x40000UL
#define CART_SIZE_512K 0x80000UL
#define CART_SIZE_1M   0x100000UL

// The switchable window, $C000-$DFFF, and the fixed region above it, which is
// always the primary chip's last 8 KB whatever the bank register holds.
#define CART_BANK_SIZE   0x2000UL
#define CART_WINDOW_BASE 0xC000UL
#define CART_FIXED_BASE  0xE000UL

// The SST39SF0x0's erase unit, and so the granularity of everything that
// tracks what a cartridge has written to itself.
#define CART_SECTOR_SIZE 0x1000UL

// At and above this an image goes to PSRAM rather than to the default heap.
// The Teensy 4.1's OCRAM takes 256 KB without complaint; half a megabyte is
// where that stops being true.
#define CART_PSRAM_FROM CART_SIZE_512K

// Why a load failed, so the library stays silent and main.cpp does the talking
// — as every other class in lib/6502 does.
enum CartStatus : uint8_t {
  CART_OK,
  CART_ERR_OPEN,   // the file is not there, or the SD card is not
  CART_ERR_SIZE,   // not one of the five lengths of §1
  CART_ERR_MEMORY, // the allocation failed
  CART_ERR_PSRAM,  // a 512 KB or 1 MB image on a board with no PSRAM fitted
  CART_ERR_READ    // the file ended early, or the SD card went away mid-read
};

// One sentence for each of the above, in the register the serial menu prints in.
const char *cartStatusMessage(CartStatus status);

// "32K", "128K", "256K", "512K", "1M" — §1's table — or NULL for a length that
// is not a cartridge at all.
const char *cartTarget(uint32_t size);

class Cart {
  private:
    uint8_t *data = nullptr;
    uint32_t imageSize = 0;
    bool inPsram = false;
    bool nameDisagrees = false;

    // §2's derived constants. All zero while the slot holds a flat cart, which
    // has no mapper at all.
    bool isBanked = false;
    uint8_t chips = 0;
    uint32_t chipSize = 0;
    uint8_t bankMask = 0;

    // The latched bank register: write-only, 8 bits, at any address in
    // $E000-$FFFF. Bit 7 is latched and goes nowhere, which is why the decoded
    // halves are kept beside it rather than re-derived on every access.
    uint8_t bankReg = 0;
    uint8_t selChip = 0;
    uint8_t selBank = 0;

    // The whole design, in two arrays.
    //
    // A banked cart *executes from the cart*: every opcode fetch in
    // $C000-$FFFF is a cart read, and on a 512 KB or 1 MB image the bytes are
    // in PSRAM, which is a great deal slower than internal RAM. Serving fetches
    // out of PSRAM would slow the emulated CPU down across the board, and the
    // slowdown would be blamed on banking rather than on where the bytes live.
    //
    // So the two 8 KB regions the CPU can actually see are held in internal
    // RAM. A read is then two branches and an array index with no PSRAM access
    // at all; a bank change costs one 8 KB copy, and happens far less often
    // than a fetch. 16 KB against the Teensy 4.1's 512 KB of OCRAM is not a
    // constraint.
    uint8_t fixedWindow[CART_BANK_SIZE];
    uint8_t bankWindow[CART_BANK_SIZE];

    void release();
    bool allocate(uint32_t size);
    void refillFixed();
    void refillWindow();
    void latchBank(uint8_t value);

  public:
    Cart();
    ~Cart();

    String file = "None";

    // Reads an image off the SD card, sizing the allocation from the file's own
    // length. Replaces the byte-at-a-time loader, which could only be right for
    // one length and silently overran the buffer for any other.
    CartStatus load(const char *path);

    // Empties the slot and gives the image's memory back. The caller clears
    // `cartLoaded` first — the bus must stop asking before the bytes go away.
    void unload();

    // RESB: the bank register clears to 0 and both chips go back to read mode.
    // Flash contents are untouched. The one place where forgetting produces a
    // cart that boots the first time and not the second — the register's /MR is
    // on RESB, so a warm reset clears it too.
    void reset();

    // Both take a **CPU** address, $8000-$FFFF. The cart does its own
    // translation, which is what keeps main.cpp from having to know whether it
    // is holding a flat image or a banked one.
    uint8_t read(uint32_t addr);
    void write(uint32_t addr, uint8_t value);

    uint32_t size() const { return imageSize; }
    uint8_t bank() const { return bankReg; }
    bool banked() const { return isBanked; }

    // Rule 3: the name must agree with the size, and a disagreement is a
    // warning, not an error. `Foo-512K.crt` that is 262,144 bytes long loads as
    // a 256K cart and says so.
    bool nameMismatch() const { return nameDisagrees; }
};

#endif
