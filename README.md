6502-DEV
========

![6502-DEV.png](./6502-DEV.png)

An **AC6502** retro-style 8-bit computer based on the **65C02** microprocessor.

---

## Overview

The AC6502 ecosystem is a family of open-source, 65C02-based computers sharing a common architecture, memory map, and [BIOS](https://github.com/acwright/6502-BIOS). Each computer in the family is purpose-built for a different use case but runs the same software and firmware.

The **DEV** is an emulation-based development system. It replaces the physical 65C02 CPU with a [Teensy 4.1](https://www.pjrc.com/store/teensy41.html) running cycle-accurate 65C02 emulation via [vrEmu6502](https://github.com/visrealm/vrEmu6502). It is the ideal platform for writing, testing, and debugging software without needing to burn ROMs or work with physical chips.

---

## Architecture

All AC6502 computers share:

- **CPU**: 65C02 (or accurate emulation)
- **Memory**: 32KB RAM + 32KB ROM, with optional banked RAM expansion
- **Memory Map**: Standardized across the ecosystem — zero page, stack, I/O space ($8000–$9FFF), system ROM, and interrupt vectors at $FFFA–$FFFF
- **Bus**: 16-bit address bus, 8-bit bidirectional data bus, standard 65C02 control signals (RW, PHI2, RESET, IRQ, NMI, RDY, SYNC)
- **BIOS**: A common [BIOS](https://github.com/acwright/6502-BIOS) provides the kernel, monitor, and BASIC interpreter across all systems

---

## Hardware

This repository contains KiCad 7.0+ PCB designs for the two boards that make up the DEV system.

### Dev Board
`Hardware/Dev Board/`

The main board hosting the Teensy 4.1. Provides 65C02 bus emulation, SD card storage, Ethernet, USB keyboard/joystick input, and hardware control buttons (Run/Stop, Step, Reset, Frequency). Includes a bus connector and card slot with level shifters for optional real hardware expansion.

### Dev Output Board
`Hardware/Dev Output Board/`

The AV output board hosting a Teensy 4.0 and a 2.4" ILI9341 TFT LCD. Emulates a TMS9918A Video Display Processor and a MOS 6581 SID audio synthesizer, driven in real time via high-speed serial from the Dev Board.

---

## Firmware

This repository contains [PlatformIO](https://platformio.org/)-based firmware for both boards.

### DB Emulator
`Firmware/DB Emulator/`

Firmware for the Teensy 4.1 on the Dev Board. Provides:

- Cycle-accurate 65C02 emulation
- SD card ROM/cartridge/program loading and memory snapshots
- Ethernet with mDNS (`6502.local`) and an embedded web control interface
- USB keyboard and joystick support (Xbox 360/One)
- Serial terminal interface (115200 baud)
- Hardware button control (Run/Stop, Step, Reset, Frequency)
- Variable CPU clock speed

See [Firmware/DB Emulator/README.md](./Firmware/DB%20Emulator/README.md) for setup and usage instructions.

### DOB Display
`Firmware/DOB Display/`

Firmware for the Teensy 4.0 on the Dev Output Board. Provides:

- TMS9918A VDP emulation (all four display modes, 256×192 active area)
- MOS 6581 SID audio emulation (3 voices, ADSR envelopes, PWM audio output)
- AV packet stream input at 6 Mbps over hardware UART from the DB Emulator

See [Firmware/DOB Display/README.md](./Firmware/DOB%20Display/README.md) for setup and usage instructions.

---

## CAD
`CAD/`

3D-printable enclosure parts and laser-cut top panels for the DEV system.

---

## Production
`Production/`

JLCPCB-ready Gerber files and BOM/CPL for PCB fabrication and assembly.

---

## Schematics
`Schematics/`

PDF schematics for each board.

---

## Libraries
`Libraries/`

Shared KiCad symbol and footprint libraries used across all AC6502 hardware projects.

---

## AC6502 Projects

| Project | Description |
|---------|-------------|
| [6502-BIOS](https://github.com/acwright/6502-BIOS) | The shared BIOS (kernel, monitor, BASIC) for all AC6502 computers |
| [6502-PRG](https://github.com/acwright/6502-PRG) | Template for writing assembly language programs |
| [6502-CRT](https://github.com/acwright/6502-CRT) | Template for writing assembly language cartridges |
| [6502-EMULATOR](https://github.com/acwright/6502-EMULATOR) | Node.js-based AC6502 emulator |
| [6502-WEBULATOR](https://github.com/acwright/6502-WEBULATOR) | Web-based AC6502 emulator |

---

## AC6502 Systems

| Project | Description |
|---------|-------------|
| [6502-ACE](https://github.com/acwright/6502-ACE) | All-in-one single-PCB computer — the COB experience without the backplane |
| [6502-COB](https://github.com/acwright/6502-COB) | Computer on a Backplane — modular desktop computer with expandable card slots |
| [6502-DEV](https://github.com/acwright/6502-DEV) | Development Environment Vehicle — emulation-based dev system (you are here) |
| [6502-KIM](https://github.com/acwright/6502-KIM) | KIM-1 inspired minimal computer |
| [6502-VCS](https://github.com/acwright/6502-VCS) | Video Computer System — cartridge-based retro gaming console |

---

## License

Hardware designs are released under the [CERN Open Hardware Licence Version 2 – Permissive](https://ohwr.org/cern_ohl_p_v2.txt).  
Firmware is released under the [MIT License](./Firmware/DB%20Emulator/LICENSE).
