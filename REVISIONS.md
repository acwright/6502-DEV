# Revisions

Every revision of every board in this repository, and what is known about each. The audit date is at the end.

**Status** says what has happened to a revision:

- **Made, checked:** JLCPCB's own copy of the order is byte-identical to the production zip here. The revision's files match that board.
- **Made, not checked:** the board exists, but its files haven't been compared with an order.
- **Ordered:** placed, and not yet arrived.
- **Drawn:** the files are complete, and the board hasn't been ordered.
- **Never made**, or **To confirm** where nothing on record says either way.

**Holes** is how far the corner mounting holes sit from the board's edges. The standard is 6 mm, and 5 mm on the ACE Board, the CF Adapter and the Keyboard Helper. A dash means a card or cart, which has no corner holes.

| Board | Rev | Status | Order | Ordered | Made from | Tindie stock | Size (mm) | Holes | Top panel | Enclosure | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| DEV Board | 1.0 | To confirm |  |  |  | — | 100×100 | 6 mm | yes | — |  |
| DEV Output Board | 1.0 | Made, checked | Y88 | 2026-03-26 | `6502-COB ddaceb3` | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |
| DEV Output Board | 1.1 | Drawn |  |  |  | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |

The audit last ran on 2026-11-04. Each production zip marked *Made, checked* was compared byte for byte with JLCPCB's copy, and its PCB regenerated to the same copper, mask, outline and holes. Tindie stock is from the listings as saved on 2026-10-23.
