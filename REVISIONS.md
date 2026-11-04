# Revisions

Every revision of every board in this repository, and what is known about each. The audit date is at the end.

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
| DEV Output Board | 1.0 | Made, matches order | Not recorded | Y88 | 2026-03-26 | `6502-COB ddaceb3` | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |
| DEV Output Board | 1.1 | Drawn | — |  |  |  | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |

The audit last ran on 2026-11-04. Each production zip marked *Made, matches order* was compared byte for byte with JLCPCB's copy, and its PCB regenerated to the same copper, mask, outline and holes. Tindie stock is from the listings as saved on 2026-10-23.
