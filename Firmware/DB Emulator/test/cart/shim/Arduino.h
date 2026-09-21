// Just enough Arduino to compile lib/6502/Cart on a desktop.
//
// The Cart class is the one piece of this firmware whose contract is written
// down somewhere else — 6502-VCS PLAN.md §§1-4 — and checked against fixtures
// that three other implementations also check against. That makes it worth
// running somewhere a Teensy is not, so this shim exists and nothing else does.
#ifndef _ARDUINO_SHIM_H
#define _ARDUINO_SHIM_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <string>

// Teensy section attributes. On a desktop every function is in the same place.
#define FASTRUN
#define DMAMEM

// PSRAM is the default heap here.
inline void *extmem_malloc(size_t size) { return malloc(size); }
inline void extmem_free(void *ptr) { free(ptr); }

class String {
  public:
    String() {}
    String(const char *s) : value(s ? s : "") {}

    size_t length() const { return value.size(); }
    const char *c_str() const { return value.c_str(); }

    int lastIndexOf(char c) const {
      size_t at = value.rfind(c);
      return at == std::string::npos ? -1 : (int) at;
    }

    void remove(int index) {
      if (index >= 0 && (size_t) index < value.size()) value.resize((size_t) index);
    }

    String operator+(const char *rhs) const { return String((value + rhs).c_str()); }
    bool operator==(const char *rhs) const { return value == rhs; }

  private:
    std::string value;
};

#endif
