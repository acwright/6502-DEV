#include "Cart.h"

// Defined by the Teensy core's startup, in megabytes, and 0 when no PSRAM chip
// is fitted. It has no header of its own.
extern "C" uint8_t external_psram_size;

const char *cartStatusMessage(CartStatus status) {
  switch (status) {
    case CART_OK:          return "Loaded";
    case CART_ERR_OPEN:    return "Cartridge could not be opened";
    case CART_ERR_SIZE:    return "Not a cartridge image size (32K, 128K, 256K, 512K or 1M)";
    case CART_ERR_MAPPER:  return "Banked cartridges need the mapper";
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

CartStatus Cart::load(const char *path) {
  if (!SD.mediaPresent()) { return CART_ERR_OPEN; }

  File file = SD.open(path);
  if (!file) { return CART_ERR_OPEN; }

  uint32_t size = (uint32_t) file.size();

  if (!cartTarget(size)) { file.close(); return CART_ERR_SIZE; }

  // The mapper is not written yet, so a banked image is refused rather than
  // loaded and misread as a flat one.
  if (size != CART_SIZE_FLAT) { file.close(); return CART_ERR_MAPPER; }

  if (size >= CART_PSRAM_FROM && external_psram_size == 0) {
    file.close();
    return CART_ERR_PSRAM;
  }

  // The old image goes before the new one is asked for, so a 1 MB cart loading
  // over a 1 MB cart needs one megabyte of PSRAM and not two.
  this->release();

  if (!this->allocate(size)) { file.close(); return CART_ERR_MEMORY; }

  // A quarter of a kilobyte at a time rather than a byte at a time: at 1 MB the
  // difference is the whole load.
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

  const char *claimed = claimedTarget(path);
  this->nameDisagrees = (strcmp(claimed, cartTarget(size)) != 0);

  return CART_OK;
}

void Cart::unload() {
  this->release();
  this->file = "None";
}

// The flat cart is the whole 32 KB image indexed from a $8000 origin. Only
// $C000-$FFFF is ever asked for — main.cpp decodes the I/O window and the ROM
// in front of the rest — so the low half is loaded and never read, which is
// exactly what the byte-at-a-time loader's 16 KB skip amounted to.
FASTRUN uint8_t Cart::read(uint32_t addr) {
  return this->data[addr - CART_START];
}

// A ROM cart has no WEB. Writes to it go nowhere, as they do on the board.
FASTRUN void Cart::write(uint32_t addr, uint8_t value) {
  (void) addr;
  (void) value;
}
