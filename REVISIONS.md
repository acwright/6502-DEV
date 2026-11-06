# Revisions

Every revision of every board in this repository, and what is known about each. The dates of the checks are at the end.

**Status** says what has happened to a revision:

- **Made, matches order:** JLCPCB's own copy of the order is byte-identical to the production zip here, and the PCB file regenerates the same copper, mask, outline and holes. This compares files with the order. It says nothing about whether the board works: that's **Tested**.
- **Made, not compared:** the board exists, but its files haven't been compared with an order.
- **Ordered:** placed, and not yet arrived.
- **Drawn:** the files are complete, and the board hasn't been ordered.
- **Never made**, or **To confirm** where nothing on record says either way.

**Holes** is how far the corner mounting holes sit from the board's edges. The standard is 6 mm, and 5 mm on the ACE Board, the CF Adapter and the Keyboard Helper. A dash means a card or cart, which has no corner holes.

| Board | Rev | Status | Tested | Order | Ordered | Made from | Tindie stock | Size (mm) | Holes | Top panel | Enclosure | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| DEV Board | 1.0 | Made, matches order | Yes | Y68 | 2025-10-28 | `6502-COB 9b9e95d` | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: J6 shield pads |
| DEV Output Board | 1.0 | Made, matches order | Yes | Y88 | 2026-03-26 | `6502-COB ddaceb3` | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |
| DEV Output Board | 1.1 | Drawn | — |  |  |  | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |

JLCPCB's orders were checked on 2026-11-04: each production zip marked *Made, matches order* was compared byte for byte with JLCPCB's copy, and its PCB regenerated to the same copper, mask, outline and holes. Each time this table is written, every PCB is checked again against its production zip, for copper and drill holes. Tindie stock is from the listings as saved on 2026-10-23.

<!-- History: hand-written. tools/revision-audit/register.py keeps everything from here to the end. -->

## History

What changed in each revision of each board, newest first. The table above has the facts about each one.

**Schematic drawing, 2026-11-06.** Every schematic's 6502 bus, cart and card connectors are drawn one way, made revisions included: a short wire and a global label on each used pin, a no-connect flag on each unused one, and GND and VCC as global labels. Bus lines, bus entries and local labels for the bus are gone. Each label's name matches the board's silk. Where pins 4 and 6 carry `EXP1` and `EXP0`, or a cartridge grounds them, a note on the schematic says what `HRDB` does with them (see the README's *Extra pins*). No netlist changed, so no board did: the schematic PDFs are regenerated, and a few PCB files carry the new net names, as text only.

### DEV Board

**Rev 1.0**

- Initial release. Built and tested. `Hardware/DEV Board/Rev 1.0/` and `Production/DEV Board/Rev 1.0/` match the boards JLCPCB made (order Y68). The Gerbers and drill files in the production zip are byte-for-byte what JLCPCB received.
- The PCB file was restored to that layout on 2026-11-04. Later edits only changed pad shapes, when footprints were refreshed, so they don't make a Rev 1.1. The schematic and BOM are today's, and they match this copper.

### DEV Output Board

**Rev 1.1** (drawn, not yet made; Rev 1.0 is sold first)

- The silkscreen labels J1 `AUDIO`, with a `+` beside it. Rev 1.0's says `SPEAKER`, with no `+`.

**Rev 1.0**

- Initial release, and the board on sale. `Hardware/DEV Output Board/Rev 1.0/` and `Production/DEV Output Board/Rev 1.0/` match the boards JLCPCB made (order Y88). The Gerbers and drill files in the production zip are byte-for-byte what JLCPCB received.
