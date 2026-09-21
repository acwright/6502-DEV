#ifndef _CART_H
#define _CART_H

#include <Arduino.h>
#include <SD.h>
#include "constants.h"

// A cartridge in the slot: the 32 KB ROM cart this firmware has always taken,
// and — once the mapper lands — the banked Flash Cart.
//
// The contract is 6502-VCS `PLAN.md` §§1-3, which is the normative copy: three
// emulators and one programmer have to agree bit for bit, so the rules live in
// that plan rather than in any one implementation.

// The five lengths an image may legally be (§1). The mapper is decided by the
// file's size and never by its name — the name is for whoever is reading `ls`.
#define CART_SIZE_FLAT 0x8000UL
#define CART_SIZE_128K 0x20000UL
#define CART_SIZE_256K 0x40000UL
#define CART_SIZE_512K 0x80000UL
#define CART_SIZE_1M   0x100000UL

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
  CART_ERR_MAPPER, // a banked size, and no mapper yet
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

    void release();
    bool allocate(uint32_t size);

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

    // Both take a **CPU** address, $8000-$FFFF. The cart does its own
    // translation, which is what lets a banked image do something different
    // with it without main.cpp knowing.
    uint8_t read(uint32_t addr);
    void write(uint32_t addr, uint8_t value);

    uint32_t size() const { return imageSize; }

    // Rule 3: the name must agree with the size, and a disagreement is a
    // warning, not an error. `Foo-512K.crt` that is 262,144 bytes long loads as
    // a 256K cart and says so.
    bool nameMismatch() const { return nameDisagrees; }
};

#endif
