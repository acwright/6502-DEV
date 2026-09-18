
#include "SerialCard.h"

// The card modelled is the standard Serial Card with its `CTS EN` jumper at
// ground, which is how every board has been built (6502-COB `09988ca`). On that
// card DCDB and DSRB are tied to ground and DTRB is not connected, so all three
// of the R6551's modem inputs are permanently low — asserted:
//
// - CTSB low never gates the transmitter, so there is no CTS here at all. A
//   variable for a pin soldered to ground would only be dead code to drift.
// - DCDB low never gates the receiver, so DTR alone does (receiverEnabled()).
// - DSRB and DCDB read 0 in the status register, and never change, so the
//   chip's interrupt on a modem-line change can never fire either.
// - DTRB reaching nothing on the card does not stop command bit 0 gating the
//   receiver, the transmitter and the interrupts inside the chip; that is
//   modelled, and is not a mistake.
//
// No card picker, no jumpers, and none of the Serial Card Pro's lines: the DEV
// is one fixed card, and SerialUSB1's own line state is not wired to it. With
// every line at ground this agrees with the reference model, 6502-EMULATOR's
// src/core/IO/ACIA.ts and SerialCard.ts, in all but the two places commented
// below: transmitterEnabled() on TIC 00, and tick() on a byte that arrives
// while the receiver is off.

SerialCard::SerialCard() {
  this->reset();
}

// Whether the receiver is running. The R6551 needs DTR (command bit 0) set —
// "0: disable receiver and all interrupts (DTR high)" — and DCDB low, which it
// always is on this card (see the top of this file).
bool SerialCard::receiverEnabled() {
  return (this->cmd & SC_CMD_DTR) != 0x00;
}

// Whether the transmitter is running.
//
// The chip needs two things for it: DTR set, since with bit 0 clear "the
// transmitter is disabled immediately", and a TIC (command bits 3-2) other
// than 00, because 00 is "RTSB = High, Transmitter Off" on Rockwell's Rev. 4
// sheet and Synertek's — settled on the bench in 2026 against a real R6551.
//
// Only the DTR half is modelled here. TIC 00 raises RTS and nothing more, and
// that is deliberate: this firmware embeds no ROM. It runs whatever 32 KB
// image is in /ROMs on the SD card, so there is no ROM to check the way the
// PicoCalc port can check its built-in BIOS, and the images most likely to be
// on that card are the BIOS 1.6 and 2.0 binaries shipped before the flow
// control was fixed — every one of which raises RTS on itself once the input
// buffer fills and then echoes through a routine that spins on TDRE. Turning
// the transmitter off at TIC 00 would deadlock those on a long paste, where
// today they merely get noisy. Adopt it once the DEV board pins a known ROM;
// the reasoning, and the bench transcript behind it, are in 6502-EMULATOR's
// src/core/IO/ACIA.ts.
bool SerialCard::transmitterEnabled() {
  return (this->cmd & SC_CMD_DTR) != 0x00;
}

uint8_t SerialCard::read(uint16_t address) {
  uint8_t _status;

  switch(address & 0x0003) {
    case 0x00: // Receive Data Register
      // A data read clears RDRF and, per the datasheet, the three receive
      // error flags — "automatically cleared after a read of the Receiver
      // Data Register" — and takes any pending interrupt with them
      this->status &= ~(SC_STATUS_IRQ |
                        SC_STATUS_RX_REG_FULL |
                        SC_STATUS_PARITY_ERROR |
                        SC_STATUS_FRAMING_ERROR |
                        SC_STATUS_OVERRUN);
      return this->rx;
    case 0x01: // Status Register
      // Bits 6 and 5 are the levels on the DSRB and DCDB pins, and both are
      // active low: 0 means ready / carrier present. The standard Serial Card
      // ties both pins to ground, so both always read 0
      _status = (uint8_t)(this->status & ~(SC_STATUS_DSR | SC_STATUS_DCD));
      // A status read clears the interrupt flag and nothing else; the byte
      // returned is the state from before the clear
      this->status &= ~SC_STATUS_IRQ;
      return _status;
    case 0x02: // Command Register
      return this->cmd;
    case 0x03: // Control Register
      return this->ctrl;
    default:
      return 0x00; // Shouldn't happen
  }
}

void SerialCard::write(uint16_t address, uint8_t value) {
  switch(address & 0x0003) {
    case 0x00: // Transmit Data Register
      // The byte only waits here; tick() sends it, and only once the
      // transmitter is on, so a write made with DTR clear leaves TDRE clear
      // and the byte where it is until the command register turns the
      // transmitter back on
      this->tx = value;
      this->status &= ~SC_STATUS_TX_REG_EMPTY;
      this->txPending = true;
      break;
    case 0x01: // Programmed Reset
      // Per the datasheet's "Program Reset Operation": clears command bits 4-0
      // — DTR high, receiver, transmitter and interrupts disabled, RTS high,
      // echo mode off — and the overrun bit of the status register. The
      // control register, the other status bits and any byte waiting in the
      // transmit register are left alone, and a pending interrupt is not
      // withdrawn: "if IRQ is low when the reset occurs, it stays low until
      // serviced"
      this->cmd &= 0b11100000;
      this->status &= ~SC_STATUS_OVERRUN;
      break;
    case 0x02: // Command Register
      this->cmd = value;
      break;
    case 0x03: // Control Register
      this->ctrl = value;
      break;
  }
}

uint8_t SerialCard::tick(uint32_t cpuFrequency) {
  // Rate-limit RX polling: check every 64 cycles (~15,625 checks/sec at 1MHz)
  if (++rxPollCounter >= 64) {
    rxPollCounter = 0;

    // Nothing is taken off the link while the receiver is off (DTR clear, the
    // reset state) or while the receive register is still full. The byte waits
    // in the USB stack's buffer instead: on a board it would be lost on the
    // wire, but here it costs nothing to keep, and an XMODEM packet arrives as
    // one burst whose every byte has to survive. Not reading it is also what
    // keeps a real overrun from being invented — the previous code flagged one
    // for a byte it had not consumed and was about to deliver anyway.
    if (this->receiverEnabled() && (this->status & SC_STATUS_RX_REG_FULL) == 0x00) {
      if (SerialUSB1.available()) {
        this->rx = SerialUSB1.read();
        this->status |= SC_STATUS_RX_REG_FULL;

        // Receiver Echo Mode
        if ((this->cmd & SC_CMD_REM) != 0x00) {
          SerialUSB1.write(this->rx);
        }

        // Receiver Interrupt: IRD (bit 1) clear enables it, and DTR is already
        // on or we would not be here
        if ((this->cmd & SC_CMD_IRD) == 0x00) {
          this->status |= SC_STATUS_IRQ;
        }
      }
    }
  }

  // The waiting byte goes out as soon as the transmitter is on (no baud rate
  // delay). With it off the byte stays in the transmit register and TDRE stays
  // clear, which is what makes firmware polling TDRE wait.
  if (txPending && this->transmitterEnabled()) {
    SerialUSB1.write(this->tx);

    this->status |= SC_STATUS_TX_REG_EMPTY;
    txPending = false;

    // Transmit Interrupt: TIC (bits 3-2) = 01, the only one of the four that
    // enables it. IRD is the receiver interrupt's bit and has no say here.
    uint8_t tic = (this->cmd & (SC_CMD_TIC0 | SC_CMD_TIC1)) >> 2;
    if (tic == 0x01) {
      this->status |= SC_STATUS_IRQ;
    }
  }

  // With DTR clear "all interrupts are disabled": the IRQB pin is not driven,
  // whatever the status register happens to be holding
  if ((this->cmd & SC_CMD_DTR) == 0x00) { return 0x00; }

  return this->status & SC_STATUS_IRQ;
}

void SerialCard::reset() {
  SerialUSB1.clear();

  this->tx = 0x00;
  this->rx = 0x00;
  this->cmd = 0x00;
  this->ctrl = 0x00;
  // DSR and DCD are active low and tied to ground on the standard Serial Card,
  // so both bits are 0. read() masks them off on the way out; nothing ever
  // sets them here.
  this->status = SC_STATUS_TX_REG_EMPTY;

  this->txPending = false;
  this->rxPollCounter = 0;
}