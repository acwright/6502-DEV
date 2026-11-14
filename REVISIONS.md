# Revisions

Every revision of every board in this repository, and what is known about each. The dates of the checks are at the end.

**Status** says what has happened to a revision:

- **Made, matches order:** JLCPCB's own copy of the order is byte-identical to the production zip here, and the PCB file regenerates the same copper, mask, outline and holes. This compares files with the order. It says nothing about whether the board works: that's the **Label**.
- **Made, not compared:** the board exists, but its files haven't been compared with an order.
- **Ordered:** placed, and not yet arrived.
- **Drawn:** the files are complete, and the board hasn't been ordered.
- **Never made**, or **To confirm** where nothing on record says either way.

**Label** says whether a builder can rely on a revision. Make the newest revision of a board, and check its label first:

- **Tested:** This revision was built and tested.
- **Proven design:** Not built at this revision, but its circuit is a tested revision's. The only changes are silkscreen text, part values, mounting holes, outline clean-up, parts moved a fraction of a millimetre onto their grid, capacitor pitch, socket outlines, a footprint swap with the same holes, a solder jumper's default, the solder mask or paste at a card edge, or tracks rerouted or pours refilled with DRC and the netlist check passing. A builder can treat it as tested.
- **Untested:** The circuit, the power path or the layout changed in a way that only a built board proves, or the board has never been built at all.

**Holes** is how far the corner mounting holes sit from the board's edges. The standard is 6 mm, and 5 mm on the ACE Board, the CF Adapter and the Keyboard Helper. A dash means a card or cart, which has no corner holes.

| Board | Rev | Status | Label | Order | Ordered | Made from | Tindie stock | Size (mm) | Holes | Top panel | Enclosure | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| DEV Board | 1.0 | Made, matches order | **Tested** | Y68 | 2025-10-28 | `6502-COB 9b9e95d` | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: J6 shield pads |
| DEV Board | 1.1 | Drawn | **Proven design**: rev 1.0's circuit; the IC designators placed by the family's rule |  |  |  | — | 100×100 | 6 mm | — | — | symbol has pins the footprint lacks: J6 shield pads |
| DEV Output Board | 1.0 | Made, matches order | **Tested** | Y88 | 2026-03-26 | `6502-COB ddaceb3` | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |
| DEV Output Board | 1.1 | Drawn | **Proven design**: silkscreen text only |  |  |  | — | 100×100 | 6 mm | yes | — | symbol has pins the footprint lacks: Teensy back-side pads |

JLCPCB's orders were checked on 2026-11-04: each production zip marked *Made, matches order* was compared byte for byte with JLCPCB's copy, and its PCB regenerated to the same copper, mask, outline and holes. Each time this table is written, every PCB is checked again against its production zip, for copper and drill holes. Tindie stock is from the listings as saved on 2026-10-23.

<!-- History: hand-written. Tools/revision-audit/register.py keeps everything from here to the end. -->

## History

What changed in each revision of each board, newest first. The table above has the facts about each one.

**IC designators and values, 2026-11-13.** On every revision not yet made, each IC's designator sits centred at one short end of its footprint, the text running along that end, and a through-hole IC's value sits at the other end. Where the centre is taken, the text is aligned to the end's left or right edge. A surface-mount IC's value is hidden, because a small package leaves no room for it, and a module whose outline names it keeps its silk as it is. A ZIF socket's texts sit on its silk outline, not its lever. `Tools/revision-audit/audit.py` checks the values. The DEV Board 1.1 is drawn with it (see its History); the DEV Output Board 1.1 has modules only. Silk only: no copper moved, and no label changed. The made revisions keep their silk as made.

**The board logo's place, 2026-11-13.** Every board logo on a revision not yet made sits in one place for its kind of board. A card's or cart's logo reads bottom to top on the left, 2.5 mm from the edge and centred on the body; any other board's runs along its edge on the line of that edge's corner holes, centred or with its end 11.5 mm from the corner (`Tools/logos/standard.json`, checked by `Tools/logos/placement.py`). Moved: DEV Output Board 1.1. Silk only: no copper moved, DRC is unchanged, and no label changed. The made revisions keep their logos where they are.

**JLCPCB's order-number marker, 2026-11-13.** The `JLCJLCJLCJLC` text is gone from the silkscreen of every revision not yet made: DEV Output Board 1.1. It told JLCPCB where to print its order number. JLCPCB stopped printing order numbers in May 2026, and its help page now asks for the text not to be there. Only the front silkscreen changed, so no label did. The made revisions keep the text, which JLCPCB ignores.

**Schematic drawing, 2026-11-06.** Every schematic's 6502 bus, cart and card connectors are drawn one way, made revisions included: a short wire and a global label on each used pin, a no-connect flag on each unused one, and GND and VCC as global labels. Bus lines, bus entries and local labels for the bus are gone. Each label's name matches the board's silk. Where pins 4 and 6 carry `EXP1` and `EXP0`, or a cartridge grounds them, a note on the schematic says what `HRDB` does with them (see the README's *Extra pins*). No netlist changed, so no board did: the schematic PDFs are regenerated, and a few PCB files carry the new net names, as text only.

### DEV Board

**Rev 1.1** (drawn, not yet made; Rev 1.0 is sold first)

- Drawn so the family's boards end as one set of Rev 1.1s under one set of rules (owner, 2026-11-13). The circuit, parts, copper, holes and outline are Rev 1.0's.
- U3's and U6's designators move to the ends of their chips, as every IC's on a revision not yet made does. The IC values stay hidden: all five are surface mount.
- The silkscreen's revision reads Rev 1.1.

**Rev 1.0**

- Initial release. Built and tested. `Hardware/DEV Board/Rev 1.0/` and `Production/DEV Board/Rev 1.0/` match the boards JLCPCB made (order Y68). The Gerbers and drill files in the production zip are byte-for-byte what JLCPCB received.
- The PCB file was restored to that layout on 2026-11-04. Later edits only changed pad shapes, when footprints were refreshed, so they don't make a Rev 1.1. The schematic and BOM are today's, and they match this copper.

### DEV Output Board

**Rev 1.1** (drawn, not yet made; Rev 1.0 is sold first)

- The silkscreen labels J1 `AUDIO`, with a `+` beside it. Rev 1.0's says `SPEAKER`, with no `+`.

**Rev 1.0**

- Initial release, and the board on sale. `Hardware/DEV Output Board/Rev 1.0/` and `Production/DEV Output Board/Rev 1.0/` match the boards JLCPCB made (order Y88). The Gerbers and drill files in the production zip are byte-for-byte what JLCPCB received.
