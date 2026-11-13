# Bill of materials: 6502-DEV

Every part for every board in this repository, by revision, with supplier numbers.

## How to use this

1. **Find your board and revision** in [Which revision do I have?](#which-revision-do-i-have)
2. **Read the [known issues](#known-issues)** for that revision, because they change what you buy or fit.
3. **Order from the table for your revision.** Each revision links to files you can upload to Digi-Key and Mouser. Those files are for one board, so multiply the quantities if you're building more. If a site asks which column is which, the first column is its part number and the third is the quantity. A cart only holds what that supplier sells, so check the table for anything else, such as chips from Jameco.
4. **Set the jumpers** before the first power-up. The README's [Jumpers](README.md#jumpers) section says which ones each board needs, and the table for your revision lists the shunts.

**Pre-soldered parts.** Some boards on Tindie come with their surface-mount parts already soldered. Parts marked **Yes** under Pre-soldered are the ones soldered on those boards. Check your listing, because others are sold as a bare PCB. Each revision that has such parts also has a `through-hole` cart that leaves them out. To solder a bare board yourself, use the full cart.

**Suppliers.** Every part has one set of numbers, so the same resistor is the same order line on every board. The numbers were read from Digi-Key and Mouser on 2026-11-04, and stock changes. A blank supplier cell means that supplier doesn't sell the part, or only sells it in bulk.

**The `bom.csv` files** under `Production/` are for JLCPCB assembly. They list the same parts. A blank LCSC cell there means JLCPCB doesn't place that part, and JLCPCB's "missing parts" warning about it is expected.

## Which revision do I have?

Look on the board: the silk prints the revision next to the copyright line. [REVISIONS.md](REVISIONS.md) has the facts for each one, and its history lists what changed between them.

**To make a new board, make the newest revision**, and check its label first. The README's [Which revision to make](README.md#which-revision-to-make) explains the three labels: **Tested**, **Proven design** and **Untested**.

| Board | Revision | What the silk says | Status | Label |
|---|---|---|---|---|
| [DEV Board](#dev-board) | 1.0 | Rev 1.0 | Made, tested | **Tested** |
| [DEV Output Board](#dev-output-board) | 1.0 | Rev 1.0 | Made, tested | **Tested** |
| [DEV Output Board](#dev-output-board) | 1.1 | Rev 1.1 | Drawn, not made yet | **Proven design**: silkscreen text only |

## Known issues

These change what you buy or fit. The boards themselves stay as they were made.

| Board | Revision | Parts | Issue |
|---|---|---|---|
| DEV Board | 1.0 | R1–R4 | Fit 10k, not 1k. If your board came with 1k soldered here, replace it. See the build notes. |
| DEV Board | 1.0 | BT1 | `BAT-HLD-001-THM`, on the tested boards, is obsolete. `BAT-HLD-001-THM-TR` is its listed continuation, with no stock today and no board built with it yet. |

### Build notes

How to build each board, where it needs more than its parts list.

<!-- Build notes: hand-written. Tools/bom/gen_bom.py keeps everything between these two markers. -->

**DEV Board**

**Rev 1.0's limits:**
- **Pull-ups:** fit 10kΩ for R1–R4, as the parts list says. Rev 1.0 was first drawn with 1kΩ there, and its files have said 10kΩ since 2026-11-04. With a 1kΩ pull-up, a device that pulls `IRQB` low sinks 5 mA, more than the 3.2 mA that the W65C22's and W65C21's IRQ outputs are rated for.

**What it connects to:**
- **`RESB`:** the DEV Board stands in for a Main Board, so it owns reset. The Teensy drives `RESB` onto the bus through U3, a level shifter that only sends signals out, so there's no `RESB` pull-up, and nothing else may pull `RESB` low. Connect a DEV only to a passive backplane, never to the COB Backplane Pro, whose reset circuit would pull against U3, and don't fit a card that pulls `RESB` low, such as the COB RTC Card.

<!-- End of build notes. -->

## DEV Board

### DEV Board Rev 1.0

Made, tested. **Tested**.

Cart files: [digikey.csv](BOM/DEV%20Board/Rev%201.0/digikey.csv), [mouser.csv](BOM/DEV%20Board/Rev%201.0/mouser.csv), [digikey-through-hole.csv](BOM/DEV%20Board/Rev%201.0/digikey-through-hole.csv), [mouser-through-hole.csv](BOM/DEV%20Board/Rev%201.0/mouser-through-hole.csv)

| Reference | Qty | Value | Part | Digi-Key | Mouser | Other | Pre-soldered |
|---|---|---|---|---|---|---|---|
| BT1 | 1 | CR2032 | TE Connectivity Linx BAT-HLD-001-THM-TR | [343-BAT-HLD-001-THM-TRCT-ND](https://www.digikey.com/en/products/filter?keywords=343-BAT-HLD-001-THM-TRCT-ND) |  |  | |
| C1–C8 | 8 | 100nF | YAGEO CC0805KRX7R9BB104 | [311-1140-1-ND](https://www.digikey.com/en/products/filter?keywords=311-1140-1-ND) | [603-CC805KRX7R9BB104](https://www.mouser.com/ProductDetail/603-CC805KRX7R9BB104) | LCSC [C49678](https://www.lcsc.com/search?q=C49678) | Yes |
| J1 | 1 | VBAT | Pin header 1×2, 2.54 mm, cut from a strip | [S1011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S1011EC-40-ND) | [200-TSW14007TS](https://www.mouser.com/ProductDetail/200-TSW14007TS) |  | |
| J2 | 1 | SPI | Pin header 2×4, 2.54 mm, cut from a strip | [S2011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S2011EC-40-ND) | [649-67996-140HLF](https://www.mouser.com/ProductDetail/649-67996-140HLF) |  | |
| J3 | 1 | PANEL | Pin header 2×4, 2.54 mm, cut from a strip | [S2011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S2011EC-40-ND) | [649-67996-140HLF](https://www.mouser.com/ProductDetail/649-67996-140HLF) |  | |
| J4 | 1 | 6502 Card Connector | TE Connectivity 5-5530843-4 | [A31723-ND](https://www.digikey.com/en/products/filter?keywords=A31723-ND) | [571-5-5530843-4](https://www.mouser.com/ProductDetail/571-5-5530843-4) |  | |
| J5 | 1 | 6502 Bus | Würth Elektronik 61204021621 | [732-5401-ND](https://www.digikey.com/en/products/filter?keywords=732-5401-ND) | [710-61204021621](https://www.mouser.com/ProductDetail/710-61204021621) |  | |
| J6 | 1 | USB Host | G-Switch GT-USB-7010ASV |  |  | LCSC [C2988369](https://www.lcsc.com/search?q=C2988369) | Yes |
| J7 | 1 | USB HOST | Pin header 1×5, 2.54 mm, cut from a strip | [S1011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S1011EC-40-ND) | [200-TSW14007TS](https://www.mouser.com/ProductDetail/200-TSW14007TS) |  | |
| J8 | 1 | RJ45_LED | Taoglas TMJDD111B083FC2A1D | [931-TMJDD111B083FC2A1D-ND](https://www.digikey.com/en/products/filter?keywords=931-TMJDD111B083FC2A1D-ND) | [960-TDD111B083FC2A1D](https://www.mouser.com/ProductDetail/960-TDD111B083FC2A1D) |  | |
| J9 | 1 | ETHERNET | Adam Tech 2PH2-06-UA | [2057-2PH2-06-UA-ND](https://www.digikey.com/en/products/filter?keywords=2057-2PH2-06-UA-ND) | [200-TMM10301TD](https://www.mouser.com/ProductDetail/200-TMM10301TD) |  | |
| R1–R4 | 4 | 10k | YAGEO RC0805FR-0710KL | [311-10.0KCRCT-ND](https://www.digikey.com/en/products/filter?keywords=311-10.0KCRCT-ND) | [603-RC0805FR-0710KL](https://www.mouser.com/ProductDetail/603-RC0805FR-0710KL) | LCSC [C17414](https://www.lcsc.com/search?q=C17414) | Yes |
| R5–R8 | 4 | 1k | YAGEO RC0805FR-071KL | [311-1.00KCRCT-ND](https://www.digikey.com/en/products/filter?keywords=311-1.00KCRCT-ND) | [603-RC0805FR-071KL](https://www.mouser.com/ProductDetail/603-RC0805FR-071KL) | LCSC [C17513](https://www.lcsc.com/search?q=C17513) | Yes |
| R9, R10 | 2 | 56k | YAGEO RC0805FR-0756KL | [311-56.0KCRCT-ND](https://www.digikey.com/en/products/filter?keywords=311-56.0KCRCT-ND) | [603-RC0805FR-0756KL](https://www.mouser.com/ProductDetail/603-RC0805FR-0756KL) | LCSC [C169923](https://www.lcsc.com/search?q=C169923) | Yes |
| R11 | 1 | 150 | YAGEO RC0805FR-07150RL | [311-150CRCT-ND](https://www.digikey.com/en/products/filter?keywords=311-150CRCT-ND) | [603-RC0805FR-07150RL](https://www.mouser.com/ProductDetail/603-RC0805FR-07150RL) | LCSC [C17471](https://www.lcsc.com/search?q=C17471) | Yes |
| SW1 | 1 | Run/Stop | TS-1187A-B-A-B |  |  | LCSC [C318884](https://www.lcsc.com/search?q=C318884) | |
| SW2 | 1 | Step | TS-1187A-B-A-B |  |  | LCSC [C318884](https://www.lcsc.com/search?q=C318884) | |
| SW3 | 1 | Clock | TS-1187A-B-A-B |  |  | LCSC [C318884](https://www.lcsc.com/search?q=C318884) | |
| SW4 | 1 | Reset | TS-1187A-B-A-B |  |  | LCSC [C318884](https://www.lcsc.com/search?q=C318884) | |
| U4 | 1 | Teensy4.1 | Teensy4.1, Teensy41 |  |  | [PJRC](https://www.pjrc.com/store/teensy41.html) | |

Pin headers are cut from strips, and the cart files count whole strips.

- **BT1:** Retainer for a CR2032 cell. BAT-HLD-001-THM, on the tested boards, is obsolete; this is its listed continuation. Digi-Key has none in stock today.
- **J5:** Male box header with a keying notch, for a 40-way ribbon cable.
- **J6:** What LCSC C2988369 is. The footprint is HRO TYPE-C-31-M-12, and JLCPCB placed this part on the made boards.
- **J8:** Replaces WIZnet J1B1211CCD, now discontinued, which older builds used. Its pinout, LEDs and pad positions match this footprint on the datasheet. No board has been built with it yet.
- **J9:** Mouser sells Samtec TMM-103-01-T-D, the same 2 mm 2×3; its Adam Tech listing has a minimum of 10,000.


## DEV Output Board

### DEV Output Board Rev 1.1

Drawn, not made yet. **Proven design**: silkscreen text only.

Cart files: [digikey.csv](BOM/DEV%20Output%20Board/Rev%201.1/digikey.csv), [mouser.csv](BOM/DEV%20Output%20Board/Rev%201.1/mouser.csv)

| Reference | Qty | Value | Part | Digi-Key | Mouser | Other |
|---|---|---|---|---|---|---|
| C1 | 1 | 3.3nF | KYOCERA AVX SR151C332KAR | [478-3175-ND](https://www.digikey.com/en/products/filter?keywords=478-3175-ND) | [581-SR151C332KAR](https://www.mouser.com/ProductDetail/581-SR151C332KAR) |  |
| J1 | 1 | AUDIO | JST Sales America Inc. B2B-XH-A | [455-B2B-XH-A-ND](https://www.digikey.com/en/products/filter?keywords=455-B2B-XH-A-ND) |  |  |
| J2 | 1 | SERIAL | Pin header 1×4, 2.54 mm, cut from a strip | [S1011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S1011EC-40-ND) | [200-TSW14007TS](https://www.mouser.com/ProductDetail/200-TSW14007TS) |  |
| R6 | 1 | 1K | YAGEO MFR-12FTF52-1K | [13-MFR-12FTF52-1KCT-ND](https://www.digikey.com/en/products/filter?keywords=13-MFR-12FTF52-1KCT-ND) | [603-MFR-12FTF52-1K](https://www.mouser.com/ProductDetail/603-MFR-12FTF52-1K) |  |
| U1 | 1 | Waveshare 2.4in LCD Header | Pin header 1×8, 2.54 mm, cut from a strip | [S1011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S1011EC-40-ND) | [200-TSW14007TS](https://www.mouser.com/ProductDetail/200-TSW14007TS) |  |
| U2 | 1 | Teensy4.0 | Teensy4.0, Teensy40 |  |  | [PJRC](https://www.pjrc.com/store/teensy40.html) |
| U3 | 1 | Waveshare 2.4in LCD | Waveshare 2.4in LCD, Waveshare 2.4in LCD |  |  | [Amazon](https://www.amazon.com/dp/B08H24H7KX) |

Pin headers are cut from strips, and the cart files count whole strips.

### DEV Output Board Rev 1.0

Made, tested. **Tested**.

Cart files: [digikey.csv](BOM/DEV%20Output%20Board/Rev%201.0/digikey.csv), [mouser.csv](BOM/DEV%20Output%20Board/Rev%201.0/mouser.csv)

| Reference | Qty | Value | Part | Digi-Key | Mouser | Other |
|---|---|---|---|---|---|---|
| C1 | 1 | 3.3nF | KYOCERA AVX SR151C332KAR | [478-3175-ND](https://www.digikey.com/en/products/filter?keywords=478-3175-ND) | [581-SR151C332KAR](https://www.mouser.com/ProductDetail/581-SR151C332KAR) |  |
| J1 | 1 | SPEAKER | JST Sales America Inc. B2B-XH-A | [455-B2B-XH-A-ND](https://www.digikey.com/en/products/filter?keywords=455-B2B-XH-A-ND) |  |  |
| J2 | 1 | SERIAL | Pin header 1×4, 2.54 mm, cut from a strip | [S1011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S1011EC-40-ND) | [200-TSW14007TS](https://www.mouser.com/ProductDetail/200-TSW14007TS) |  |
| R6 | 1 | 1K | YAGEO MFR-12FTF52-1K | [13-MFR-12FTF52-1KCT-ND](https://www.digikey.com/en/products/filter?keywords=13-MFR-12FTF52-1KCT-ND) | [603-MFR-12FTF52-1K](https://www.mouser.com/ProductDetail/603-MFR-12FTF52-1K) |  |
| U1 | 1 | Waveshare 2.4in LCD Header | Pin header 1×8, 2.54 mm, cut from a strip | [S1011EC-40-ND](https://www.digikey.com/en/products/filter?keywords=S1011EC-40-ND) | [200-TSW14007TS](https://www.mouser.com/ProductDetail/200-TSW14007TS) |  |
| U2 | 1 | Teensy4.0 | Teensy4.0, Teensy40 |  |  | [PJRC](https://www.pjrc.com/store/teensy40.html) |
| U3 | 1 | Waveshare 2.4in LCD | Waveshare 2.4in LCD, Waveshare 2.4in LCD |  |  | [Amazon](https://www.amazon.com/dp/B08H24H7KX) |

Pin headers are cut from strips, and the cart files count whole strips.
