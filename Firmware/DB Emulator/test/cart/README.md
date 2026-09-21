# Cart tests

`lib/6502/Cart/` is the one part of this firmware whose contract is written down somewhere
else. **6502-VCS `PLAN.md` §§1–4** is the normative copy — the naming scheme, the mapper,
the flash command set and the `.sav` container — because three emulators and one programmer
have to agree on them bit for bit.

So this runs on a desktop, where a Teensy is not:

```
./run.sh
```

`shim/` is the two dozen lines of `Arduino.h` and `SD.h` that `Cart.cpp` actually uses,
over `std::string` and `stdio`. Nothing else in the firmware is shimmed and nothing else
should be — this exists for the one class that has an external contract.

## The fixtures

`fixtures/` is a copy of `6502-VCS Firmware/FH Programmer/host/test/fixtures/`, the shared
oracle. 6502-EMULATOR's tests, the PicoCalc port's, `6502-flash`'s own and these all check
against **those files** rather than against each other, which is the only way four
implementations of one contract end up agreeing rather than sharing a bug.

| File | Bytes | CRC-32 |
|---|---|---|
| `legacy.crt` | 32,768 | `98a4e0fd` |
| `banked-128K.crt` | 131,072 | `e0e4f83c` |
| `sample.sav` | 12,316 | `45c4068e` |

The checksums are quoted in `test_cart.cpp`, so a stale copy fails rather than passing
against the wrong bytes. **If one of them moves in 6502-VCS, this copy has to be told.**
