// The two dozen lines of the SD library that lib/6502/Cart uses, over stdio.
#ifndef _SD_SHIM_H
#define _SD_SHIM_H

#include <cstdio>
#include <cstdint>
#include <cstring>

#define FILE_WRITE 1

class File {
  public:
    File() {}
    explicit File(FILE *handle) : handle(handle) {}

    explicit operator bool() const { return handle != nullptr; }
    bool operator!() const { return handle == nullptr; }

    uint32_t size() {
      if (!handle) return 0;
      long at = ftell(handle);
      fseek(handle, 0, SEEK_END);
      long end = ftell(handle);
      fseek(handle, at, SEEK_SET);
      return (uint32_t) end;
    }

    int read(void *buffer, size_t count) {
      if (!handle) return -1;
      size_t got = fread(buffer, 1, count, handle);
      return (int) got;
    }

    size_t write(const void *buffer, size_t count) {
      if (!handle) return 0;
      return fwrite(buffer, 1, count, handle);
    }

    void close() {
      if (handle) { fclose(handle); handle = nullptr; }
    }

  private:
    FILE *handle = nullptr;
};

class SDClass {
  public:
    bool mediaPresent() { return true; }

    File open(const char *path) { return File(fopen(path, "rb")); }
    File open(const char *path, int mode) { (void) mode; return File(fopen(path, "ab")); }

    bool exists(const char *path) {
      FILE *f = fopen(path, "rb");
      if (!f) return false;
      fclose(f);
      return true;
    }

    bool remove(const char *path) { return ::remove(path) == 0; }
};

extern SDClass SD;

#endif
