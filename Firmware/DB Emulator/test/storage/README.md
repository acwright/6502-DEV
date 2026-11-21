# Storage Card tests

`lib/6502/IO/StorageCard` is the emulated CompactFlash card in I/O 4, and it has to answer as
a real card does. The reference is the SD controller firmware that answers on the bus as a
card (6502-COB `Firmware/SCP Controller/core/ata.c` and `identify.c`); 6502-EMULATOR's
`src/core/IO/Storage.ts` and the PicoCalc's `src/machine/storage.c` follow the same card, and
their tests check the same things.

So this runs on a desktop, where a Teensy is not:

```
./run.sh
```

`shim/` is the few lines of `Arduino.h` and `SD.h` that `StorageCard.cpp` uses. The SD card
holds one sparse `Storage.bin`: only the sectors written hold memory, so a test can reach the
card's last sector without a 256 MB file. It is a separate shim from `test/cart/`'s, which
writes through `stdio` in append mode and so can't seek a write.

`test_storage.cpp` checks what a real card does in the places the model once differed
(6502-DEV#1): IDENTIFY's word 0 and its byte-swapped strings, ready reading `$50`, a count of
0 moving 256 sectors, a command written in the middle of a transfer ending it, Erase Sectors
erasing its count, the data register ignored outside a transfer, a transfer stopping at the
end of the card, and no SD card answering AMNF.
