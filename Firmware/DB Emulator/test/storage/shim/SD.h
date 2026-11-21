// The part of the SD library that lib/6502/IO/StorageCard uses, over one
// sparse in-memory file: only the sectors written hold memory, so a test can
// reach the card's last sector without a 256 MB Storage.bin.
#ifndef _SD_SHIM_H
#define _SD_SHIM_H

#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

#define FILE_WRITE 1

struct SparseFile {
  std::map<uint32_t, std::vector<uint8_t>> sectors; // 512-byte sectors, only those written
  bool exists = false;

  uint8_t get(uint32_t at) const {
    auto s = sectors.find(at / 512);
    return s == sectors.end() ? 0 : s->second[at % 512];
  }
  void set(uint32_t at, uint8_t value) {
    auto s = sectors.find(at / 512);
    if (s == sectors.end()) {
      if (!value) return; // a zero into an unwritten sector: still zero
      s = sectors.emplace(at / 512, std::vector<uint8_t>(512, 0)).first;
    }
    s->second[at % 512] = value;
  }
};

extern SparseFile storageBin;

class File {
  public:
    File() {}
    explicit File(SparseFile *file) : file(file) {}

    explicit operator bool() const { return file != nullptr; }
    bool operator!() const { return file == nullptr; }

    bool seek(uint32_t at) { position = at; return true; }
    int read() { return file ? file->get(position++) : -1; }
    size_t write(uint8_t value) { if (!file) return 0; file->set(position++, value); return 1; }
    size_t write(const uint8_t *data, size_t count) { for (size_t i = 0; i < count; i++) write(data[i]); return file ? count : 0; }
    void close() { file = nullptr; }

  private:
    SparseFile *file = nullptr;
    uint32_t position = 0;
};

class SDClass {
  public:
    bool mediaPresent() { return present; }
    bool exists(const char *path) { (void) path; return storageBin.exists; }
    File open(const char *path) { (void) path; return File(storageBin.exists ? &storageBin : nullptr); }
    File open(const char *path, int mode) { (void) path; (void) mode; storageBin.exists = true; return File(&storageBin); }

    bool present = true;
};

extern SDClass SD;

#endif
