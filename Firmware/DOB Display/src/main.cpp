#include <Arduino.h>
#include <SPI.h>
#include <ILI9341_t3n.h>
#include <DOBDisplay.h>

void processSerialByte(uint8_t b);
void dispatch(uint8_t type, uint8_t a, uint8_t b);
void render();
void setPixel(uint16_t x, uint16_t y, uint8_t colorIndex);
void renderGraphicsI();
void renderGraphicsII();
void renderText();
void renderMulticolor();
void renderSprites();
void sidISR();
void sidReset();
void sidWrite(uint8_t reg, uint8_t val);
void info();

#define HWSERIAL Serial1

uint8_t  vram[16384];
uint8_t  registers[8];
uint16_t vramAddr = 0;

uint16_t palette[256];
uint8_t  framebuffer[CANVAS_W * CANVAS_H];

uint8_t  serialBuffer[32768];

// AV packet accumulator
uint8_t  pktBuf[4];
uint8_t  pktLen = 0;

time_t   lastRenderTime = 0;
uint8_t  targetFrameTime = 17;  // ~60 FPS

//
// SID AUDIO STATE
//

#define SID_SAMPLE_RATE  44100
#define SID_CLOCK        985248  // PAL 6581

// Attack times in ms, indexed by 4-bit attack nibble
static const uint16_t SID_ATTACK_MS[16]  = {
    2,    8,   16,   24,   38,   56,   68,   80,
  100,  250,  500,  800, 1000, 3000, 5000, 8000
};

// Decay/release times in ms, indexed by 4-bit nibble
static const uint16_t SID_DECAY_MS[16]   = {
    6,   24,   48,   72,  114,  168,  204,  240,
  300,  750, 1500, 2400, 3000, 9000, 15000, 24000
};

struct SIDVoice {
  uint32_t phase;           // 32-bit DDS phase accumulator
  uint32_t phaseInc;        // pre-computed increment per sample
  uint32_t envAccum;        // Q8.24 envelope accumulator (>>24 = level 0-255)
  uint32_t envAttackRate;   // Q8.24 increment per sample during attack
  uint32_t envDecayRate;    // Q8.24 decrement per sample during decay
  uint32_t envReleaseRate;  // Q8.24 decrement per sample during release
  uint8_t  envSustain;      // sustain level 0-255
  uint8_t  envState;        // 0=idle 1=attack 2=decay 3=sustain 4=release
  uint8_t  gatePrev;        // previous gate bit for edge detection
  uint16_t lfsr;            // noise LFSR
  uint8_t  noiseTrigger;    // previous noise clock bit
};

uint8_t     sid[32];
SIDVoice    sidVoice[3];
IntervalTimer sidTimer;

ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RESET);

//
// MAIN
//

void setup() {
  pinMode(TFT_BL, OUTPUT);
  pinMode(TFT_RESET, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  analogWriteResolution(8);  // 8-bit PWM DAC for audio output
  digitalWrite(TFT_BL, HIGH);
  digitalWrite(TFT_RESET, HIGH);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  // Build palette: indices 0-15 from TMS_PALETTE, rest = black
  for (uint16_t i = 0; i < 256; i++) {
    palette[i] = (i < 16) ? TMS_PALETTE[i] : 0x0000;
  }

  Serial.begin(115200);
  HWSERIAL.begin(6000000);
  HWSERIAL.addMemoryForRead(serialBuffer, sizeof(serialBuffer));

  memset(vram, 0, sizeof(vram));
  memset(registers, 0, sizeof(registers));

  sidReset();

  info();
}

void loop() {
  while (HWSERIAL.available()) processSerialByte(HWSERIAL.read());
  while (Serial.available())   processSerialByte(Serial.read());

  if (millis() - lastRenderTime >= targetFrameTime) {
    render();
    lastRenderTime = millis();
  }
}

//
// AV PACKET PROCESSING
//

void processSerialByte(uint8_t b) {
  if (pktLen == 0 && b != AV_SYNC) return;  // re-align
  pktBuf[pktLen++] = b;

  if (pktLen == 4) {
    dispatch(pktBuf[1], pktBuf[2], pktBuf[3]);
    pktLen = 0;
  }
}

void dispatch(uint8_t type, uint8_t a, uint8_t b) {
  switch (type) {
    case AV_TYPE_VIDEO_DATA:
      vram[vramAddr & 0x3FFF] = b;
      vramAddr = (vramAddr + 1) & 0x3FFF;
      break;
    case AV_TYPE_VIDEO_REG:
      registers[a & 0x07] = b;
      break;
    case AV_TYPE_VIDEO_ADDR:
      vramAddr = ((uint16_t)(a & 0x3F) << 8) | b;
      break;
    case AV_TYPE_SOUND:
      sidWrite(a & 0x1F, b);
      break;
    case AV_TYPE_RESET:
      memset(vram, 0, sizeof(vram));
      memset(registers, 0, sizeof(registers));
      vramAddr = 0;
      sidReset();
      break;
  }
}

//
// RENDER
//

void render() {
  uint8_t m1 = (registers[1] >> 4) & 1;
  uint8_t m2 = (registers[1] >> 3) & 1;
  uint8_t m3 = (registers[0] >> 1) & 1;
  uint8_t backdrop = registers[7] & 0x0F;
  uint8_t displayEnabled = (registers[1] >> 6) & 1;

  memset(framebuffer, backdrop, sizeof(framebuffer));

  // R1 bit 6 clear blanks the active area to the backdrop without disturbing
  // VRAM — the usual way to hide a screen while it is being redrawn
  if (displayEnabled) {
    if      (m1 == 0 && m2 == 0 && m3 == 0) { renderGraphicsI();   renderSprites(); }
    else if (m1 == 0 && m2 == 0 && m3 == 1) { renderGraphicsII();  renderSprites(); }
    else if (m1 == 1 && m2 == 0 && m3 == 0) { renderText(); }
    else if (m1 == 0 && m2 == 1 && m3 == 0) { renderMulticolor();  renderSprites(); }
  }

  tft.writeRect8BPP(0, 0, CANVAS_W, CANVAS_H, framebuffer, palette);
}

void setPixel(uint16_t x, uint16_t y, uint8_t colorIndex) {
  if (x >= ACTIVE_W || y >= ACTIVE_H) return;
  if (colorIndex == 0) return;  // transparent - show backdrop
  framebuffer[(y + BORDER_Y) * CANVAS_W + (x + BORDER_X)] = colorIndex;
}

//
// GRAPHICS I MODE - 32x24 tiles, 8x8 each
//

void renderGraphicsI() {
  uint16_t nameTable    = (registers[2] & 0x0F) << 10;
  uint16_t colorTable   = registers[3] << 6;
  uint16_t patternTable = (registers[4] & 0x07) << 11;

  for (uint8_t tileY = 0; tileY < 24; tileY++) {
    for (uint8_t tileX = 0; tileX < 32; tileX++) {
      uint8_t charCode = vram[(nameTable + tileY * 32 + tileX) & 0x3FFF];

      uint8_t colorByte = vram[(colorTable + (charCode >> 3)) & 0x3FFF];
      uint8_t fg = (colorByte >> 4) & 0x0F;
      uint8_t bg = colorByte & 0x0F;

      for (uint8_t row = 0; row < 8; row++) {
        uint8_t patByte = vram[(patternTable + charCode * 8 + row) & 0x3FFF];
        for (uint8_t col = 0; col < 8; col++) {
          uint8_t bit = (patByte >> (7 - col)) & 1;
          setPixel(tileX * 8 + col, tileY * 8 + row, bit ? fg : bg);
        }
      }
    }
  }
}

//
// GRAPHICS II MODE - 32x24 tiles, 3 sections of 256 patterns
//

void renderGraphicsII() {
  uint16_t nameTable    = (registers[2] & 0x0F) << 10;
  uint16_t colorBase    = (registers[3] & 0x80) << 6;
  uint16_t patternBase  = (registers[4] & 0x04) << 11;

  // R3 and R4 do double duty in Graphics II: R3's low seven bits mask the
  // name-table byte, and R4's low two bits select which of the three thirds of
  // the screen get their own pattern and colour page. A program that clears
  // them expects a single page repeated down the screen, not three.
  uint16_t nameMask = ((registers[3] & 0x7F) << 3) | 0x07;

  for (uint8_t tileY = 0; tileY < 24; tileY++) {
    uint16_t pageOffset   = (uint16_t)(((tileY & 0x18) >> 3) & (registers[4] & 0x03)) << 11;
    uint16_t patternTable = patternBase + pageOffset;
    uint16_t colorTable   = colorBase + (pageOffset & ((registers[3] & 0x60) << 6));

    for (uint8_t tileX = 0; tileX < 32; tileX++) {
      uint16_t charCode = vram[(nameTable + tileY * 32 + tileX) & 0x3FFF] & nameMask;

      for (uint8_t row = 0; row < 8; row++) {
        uint8_t patByte   = vram[(patternTable + charCode * 8 + row) & 0x3FFF];
        uint8_t colorByte = vram[(colorTable   + charCode * 8 + row) & 0x3FFF];
        uint8_t fg = (colorByte >> 4) & 0x0F;
        uint8_t bg = colorByte & 0x0F;

        for (uint8_t col = 0; col < 8; col++) {
          uint8_t bit = (patByte >> (7 - col)) & 1;
          setPixel(tileX * 8 + col, tileY * 8 + row, bit ? fg : bg);
        }
      }
    }
  }
}

//
// TEXT MODE - 40x24 tiles, 6x8 each, centered in 256px
//

void renderText() {
  uint16_t nameTable    = (registers[2] & 0x0F) << 10;
  uint16_t patternTable = (registers[4] & 0x07) << 11;
  uint8_t fg = (registers[7] >> 4) & 0x0F;
  uint8_t bg = registers[7] & 0x0F;
  uint8_t xOffset = 8;  // center 240px (40*6) in 256px

  for (uint8_t tileY = 0; tileY < 24; tileY++) {
    for (uint8_t tileX = 0; tileX < 40; tileX++) {
      uint8_t charCode = vram[(nameTable + tileY * 40 + tileX) & 0x3FFF];

      for (uint8_t row = 0; row < 8; row++) {
        uint8_t patByte = vram[(patternTable + charCode * 8 + row) & 0x3FFF];
        for (uint8_t col = 0; col < 6; col++) {
          uint8_t bit = (patByte >> (7 - col)) & 1;
          setPixel(xOffset + tileX * 6 + col, tileY * 8 + row, bit ? fg : bg);
        }
      }
    }
  }
}

//
// MULTICOLOR MODE - 64x48 fat pixels (4x4 each)
//

void renderMulticolor() {
  uint16_t nameTable    = (registers[2] & 0x0F) << 10;
  uint16_t patternTable = (registers[4] & 0x07) << 11;

  for (uint8_t tileY = 0; tileY < 24; tileY++) {
    for (uint8_t tileX = 0; tileX < 32; tileX++) {
      uint8_t charCode = vram[(nameTable + tileY * 32 + tileX) & 0x3FFF];

      // One pattern byte covers a whole 8x4 half-tile — high nibble the left
      // 4x4 block, low nibble the right. Which of the character's eight bytes
      // a group of scanlines uses steps once every four lines and is offset by
      // the tile row's position within a group of four, which is what lets the
      // same 32 name-table entries paint four different sets of blocks down
      // the screen.
      for (uint8_t half = 0; half < 2; half++) {
        uint8_t byteIndex  = (tileY & 0x03) * 2 + half;
        uint8_t patByte    = vram[(patternTable + charCode * 8 + byteIndex) & 0x3FFF];
        uint8_t leftColor  = (patByte >> 4) & 0x0F;
        uint8_t rightColor = patByte & 0x0F;

        for (uint8_t dy = 0; dy < 4; dy++) {
          uint16_t y = tileY * 8 + half * 4 + dy;
          for (uint8_t dx = 0; dx < 4; dx++) {
            setPixel(tileX * 8 + dx,     y, leftColor);
            setPixel(tileX * 8 + 4 + dx, y, rightColor);
          }
        }
      }
    }
  }
}

//
// SPRITE RENDERING - up to 32 sprites, max 4 visible per scanline
//

void renderSprites() {
  uint16_t spriteAttrTable    = (registers[5] & 0x7F) << 7;
  uint16_t spritePatternTable = (registers[6] & 0x07) << 11;
  uint8_t  size16  = (registers[1] >> 1) & 1;
  uint8_t  magnify = registers[1] & 1;
  uint8_t  spriteSize   = size16 ? 16 : 8;
  uint8_t  pixelSize    = magnify ? 2 : 1;
  uint8_t  spriteSizePx = spriteSize * pixelSize;

  // Which sprite has claimed each pixel of the scanline being built: 0 is
  // free, anything else is colour + 1. A transparent sprite still claims its
  // pixels against lower-priority sprites, which is why this holds colour + 1
  // rather than a plain flag.
  uint8_t rowSpriteBits[ACTIVE_W];

  // Scanline-major, sprites visited in index order, as the real part scans
  // them. Both rules fall out of that: a pixel already claimed by a
  // lower-numbered sprite is not overwritten, and once four sprites have been
  // found on a line the fifth and everything after it is dropped from it.
  for (int16_t y = 0; y < ACTIVE_H; y++) {
    uint8_t spritesShown = 0;

    for (uint8_t i = 0; i < 32; i++) {
      uint16_t attrAddr = spriteAttrTable + i * 4;
      int16_t  yPos     = vram[attrAddr & 0x3FFF];

      if (yPos == 0xD0) break;  // sentinel: no sprites past this one

      // A Y above 0xE0 is negative — a sprite entering from above the screen
      if (yPos > 0xE0) yPos -= 256;
      yPos += 1;  // first visible row is Y + 1

      int16_t patRow = y - yPos;
      if (magnify) patRow >>= 1;
      if (patRow < 0 || patRow >= spriteSize) continue;

      if (spritesShown == 0) memset(rowSpriteBits, 0, sizeof(rowSpriteBits));

      spritesShown++;
      if (spritesShown > 4) break;

      uint8_t colorByte = vram[(attrAddr + 3) & 0x3FFF];
      uint8_t color     = colorByte & 0x0F;
      uint8_t patIdx    = vram[(attrAddr + 2) & 0x3FFF];
      if (size16) patIdx &= 0xFC;

      // Rows 8-15 of a 16x16 sprite fall into quadrant B naturally; the jump
      // to +16 bytes below moves from the left half (A/B) to the right (C/D)
      uint16_t patOffset = spritePatternTable + patIdx * 8 + patRow;
      uint8_t  patByte   = vram[patOffset & 0x3FFF];
      uint8_t  patBit    = 0;

      int16_t xPos = vram[(attrAddr + 1) & 0x3FFF];
      if (colorByte & 0x80) xPos -= 32;  // early clock

      int16_t endX = xPos + spriteSizePx;
      if (endX > ACTIVE_W) endX = ACTIVE_W;

      for (int16_t x = xPos, screenBit = 0; x < endX; x++, screenBit++) {
        if (x >= 0 && (patByte & 0x80)) {
          if (color != 0 && rowSpriteBits[x] < 2) setPixel(x, y, color);
          if (rowSpriteBits[x] == 0) rowSpriteBits[x] = color + 1;
        }

        if (!magnify || (screenBit & 1)) {
          patByte = (patByte << 1) & 0xFF;
          if (++patBit == 8) {
            patBit = 0;
            if (size16) patByte = vram[(patOffset + 16) & 0x3FFF];
          }
        }
      }
    }
  }
}

//
// SID AUDIO ENGINE
//

// Pre-compute per-voice phase increment from frequency registers 0/1 (or 7/8, 14/15)
void sidUpdateFreq(uint8_t v) {
  uint16_t freqReg = sid[v * 7] | ((uint16_t)sid[v * 7 + 1] << 8);
  // Scale SID 24-bit DDS to 32-bit DDS at SID_SAMPLE_RATE:
  //   phaseInc = freqReg * SID_CLOCK * 256 / SID_SAMPLE_RATE
  sidVoice[v].phaseInc = (uint32_t)(freqReg * (SID_CLOCK * 256.0f / SID_SAMPLE_RATE));
}

// Pre-compute ADSR envelope rates from registers 5/6 (or 12/13, 19/20)
void sidUpdateADSR(uint8_t v) {
  uint8_t attackIdx  = (sid[v * 7 + 5] >> 4) & 0x0F;
  uint8_t decayIdx   =  sid[v * 7 + 5]       & 0x0F;
  uint8_t sustainNib = (sid[v * 7 + 6] >> 4) & 0x0F;
  uint8_t releaseIdx =  sid[v * 7 + 6]       & 0x0F;

  float ms2samples = SID_SAMPLE_RATE / 1000.0f;
  float scale      = 255.0f * (float)(1u << 24);

  sidVoice[v].envAttackRate  = (uint32_t)(scale / (SID_ATTACK_MS[attackIdx]  * ms2samples));
  sidVoice[v].envDecayRate   = (uint32_t)(scale / (SID_DECAY_MS[decayIdx]    * ms2samples));
  sidVoice[v].envReleaseRate = (uint32_t)(scale / (SID_DECAY_MS[releaseIdx]  * ms2samples));
  sidVoice[v].envSustain     = sustainNib * 17;
}

// Handle a SID register write — update mirrored state and pre-computed values
void sidWrite(uint8_t reg, uint8_t val) {
  sid[reg] = val;
  uint8_t v = reg / 7;
  if (v >= 3) return;  // regs 0x15-0x18 (filter/volume) are used directly from sid[] in the ISR

  uint8_t r = reg % 7;

  if (r == 0 || r == 1) sidUpdateFreq(v);   // frequency lo/hi
  if (r == 5 || r == 6) sidUpdateADSR(v);   // attack/decay or sustain/release

  if (r == 4) {
    // Control register — detect gate transitions
    uint8_t gate = val & 1;
    if (gate && !sidVoice[v].gatePrev) {
      // Rising edge: restart oscillator and begin attack
      sidVoice[v].phase    = 0;
      sidVoice[v].envAccum = 0;
      sidVoice[v].envState = 1;
    } else if (!gate && sidVoice[v].gatePrev) {
      // Falling edge: begin release from current level
      sidVoice[v].envState = 4;
    }
    sidVoice[v].gatePrev = gate;
  }
}

// Full SID / audio reset
void sidReset() {
  sidTimer.end();
  memset(sid,      0, sizeof(sid));
  memset(sidVoice, 0, sizeof(sidVoice));
  for (uint8_t v = 0; v < 3; v++) sidVoice[v].lfsr = 0xACE1;  // non-zero LFSR seed
  analogWrite(SPEAKER, 0);
  sidTimer.begin(sidISR, 1000000.0f / SID_SAMPLE_RATE);
}

// Synthesize one audio sample and write PWM — called by IntervalTimer at SID_SAMPLE_RATE Hz
FASTRUN void sidISR() {
  uint8_t masterVol = sid[0x18] & 0x0F;
  int32_t mix = 0;

  for (uint8_t v = 0; v < 3; v++) {
    SIDVoice &sv = sidVoice[v];
    uint8_t   ctrl = sid[v * 7 + 4];

    // ---- Advance phase accumulator ----
    sv.phase += sv.phaseInc;

    // ---- Waveform generation (0-255) ----
    uint8_t wave = 0;

    if (ctrl & 0x80) {
      // Noise: advance LFSR on rising edge of phase bit 27 (maps SID bit 19 to 32-bit DDS)
      uint8_t trig = (sv.phase >> 27) & 1;
      if (trig && !sv.noiseTrigger) {
        uint16_t b = ((sv.lfsr >> 0) ^ (sv.lfsr >> 2) ^ (sv.lfsr >> 3) ^ (sv.lfsr >> 5)) & 1;
        sv.lfsr = (sv.lfsr >> 1) | (b << 15);
      }
      sv.noiseTrigger = trig;
      wave = (uint8_t)(sv.lfsr >> 8);

    } else if (ctrl & 0x40) {
      // Pulse: compare upper 12 bits of phase against pulse-width register
      uint16_t pw = sid[v * 7 + 2] | ((uint16_t)(sid[v * 7 + 3] & 0x0F) << 8);
      wave = ((sv.phase >> 20) < pw) ? 255 : 0;

    } else if (ctrl & 0x20) {
      // Sawtooth: phase upper 8 bits directly
      wave = (uint8_t)(sv.phase >> 24);

    } else if (ctrl & 0x10) {
      // Triangle: mirror sawtooth in second half of cycle
      wave = (sv.phase < 0x80000000u)
             ? (uint8_t)(sv.phase  >> 23)
             : (uint8_t)((0xFFFFFFFFu - sv.phase) >> 23);
    }

    // ---- ADSR envelope ----
    switch (sv.envState) {
      case 1: {  // Attack: ramp up to 255
        // The fastest attacks step far enough that the sum can pass UINT32_MAX
        // and wrap below full scale, so treat a wrap as having reached the top.
        uint32_t next = sv.envAccum + sv.envAttackRate;
        if (next < sv.envAccum || next >= 0xFF000000u) {
          sv.envAccum = 0xFF000000u;
          sv.envState = (sv.envSustain < 255) ? 2 : 3;
        } else {
          sv.envAccum = next;
        }
        break;
      }
      case 2:  // Decay: ramp down to sustain level
        if (sv.envAccum <= sv.envDecayRate || (sv.envAccum >> 24) <= sv.envSustain) {
          sv.envAccum = (uint32_t)sv.envSustain << 24;
          sv.envState = 3;
        } else {
          sv.envAccum -= sv.envDecayRate;
        }
        break;
      case 3:  // Sustain: hold
        sv.envAccum = (uint32_t)sv.envSustain << 24;
        break;
      case 4:  // Release: ramp down to 0
        if (sv.envAccum <= sv.envReleaseRate) {
          sv.envAccum = 0;
          sv.envState = 0;
        } else {
          sv.envAccum -= sv.envReleaseRate;
        }
        break;
    }

    // Apply envelope to waveform and accumulate mix
    mix += ((int32_t)wave * (int32_t)(sv.envAccum >> 24)) >> 8;
  }

  // Scale by master volume (0-15), average across 3 voices, output 0-255
  analogWrite(SPEAKER, (uint8_t)((mix * masterVol) / (15 * 3)));
}

//
// INFO
//

void info() {
  Serial.println();
  Serial.println("8eeee8 8eee88 8eeee8     8eeee8 8  8eeee8 8eeee8 8     8eeee8 8    8 ");
  Serial.println("8    8 8    8 8    8     8    8 8  8      8    8 8     8    8 8    8 ");
  Serial.println("8e   8 8    8 8eeee8ee   8e   8 8e 8eeeee 8eeee8 8e    8eeee8 8eeee8 ");
  Serial.println("88   8 8    8 88     8   88   8 88     88 88     88    88   8   88   ");
  Serial.println("88   8 8    8 88     8   88   8 88 e   88 88     88    88   8   88   ");
  Serial.println("88eee8 8eeee8 88eeeee8   88eee8 88 8eee88 88     88eee 88   8   88   ");
  Serial.println();
  Serial.print("DOB Display (TMS9918A) | Version: ");
  Serial.print(VERSION);
  Serial.println();
  Serial.println("--------------------------------------");
  Serial.println("| Created by A.C. Wright (c) 2026   |");
  Serial.println("--------------------------------------");
  Serial.println();
}
