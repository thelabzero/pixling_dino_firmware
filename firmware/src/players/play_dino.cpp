// Display check for the dino (plan D3/D4 step 1): plays every Face / Life / Utility clip of the design sheet
// in a loop, drawn live by the face rig (src/face, src/scenes/dino_clips.*), and reports timing on this chip.
//   pio run -e play_dino -t upload && pio device monitor -e play_dino
// Holds use the design's preview timing, capped at 3 s. Serial, one letter:
//   n next clip   p previous clip   r repeat this clip on/off   ? list clips   s stats now
//
// Pipelined like the real firmware will be: while frame i is held on screen, frame i+1 is rendered into a
// second buffer. A frame is "late" when its render does not finish inside the hold (the next frame then
// appears late). One clip is kept in RAM at a time, plus the next clip while the last frame is shown.
// Runs on the default loop stack (8 KB): buildDino() is split per clip for that (see dino_clips.cpp).
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include "config.h"
#include "scenes/dino_clips.h"

static Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
static std::vector<scenes::Clip> cur, nxt;  // clip on screen, clip after it (built early at the end of cur)
static face::Grid bufA, bufB;
static face::Grid *shown = &bufA, *prep = &bufB;
static int curIdx = 0;       // clip index of `cur`
static size_t frameIdx = 0;  // frame of `cur` on screen
static uint32_t shownAt = 0, holdMs = 0;
static bool repeatClip = false;
// stats for the clip on screen
static uint32_t worstUs, sumUs, nFrames, nLate, worstDispUs;

static inline int wrap(int i) { return (i + scenes::DINO_CLIP_COUNT) % scenes::DINO_CLIP_COUNT; }

static void blit(const face::Grid& g) {
  uint8_t* buf = display.getBuffer();
  for (int page = 0; page < face::H / 8; page++)
    for (int x = 0; x < face::W; x++) {
      uint8_t b = 0;
      for (int bit = 0; bit < 8; bit++)
        if (g.d[(page * 8 + bit) * face::W + x]) b |= 1 << bit;
      buf[page * face::W + x] = b;
    }
}

static uint32_t holdFor(const scenes::Frame& f) {
  uint32_t h = f.designMs();
  return h > 3000 ? 3000 : h;
}

static void build(std::vector<scenes::Clip>& into, int idx) {
  into.clear();
  into.shrink_to_fit();
  scenes::buildDino(into, "10:24", "SAM", scenes::DINO_CLIP_KEYS[idx]);
  if (into.empty()) Serial.printf("clip %s not built (keys out of sync?)\n", scenes::DINO_CLIP_KEYS[idx]);
}

static void report() {
  if (!nFrames || cur.empty()) return;
  Serial.printf("%-12s %2u frames: render avg %lu us, worst %lu us, display() worst %lu us, late %lu, heap %u, stack left %u B\n",
                cur[0].key, (unsigned)cur[0].frames.size(), (unsigned long)(sumUs / nFrames), (unsigned long)worstUs,
                (unsigned long)worstDispUs, (unsigned long)nLate, (unsigned)ESP.getFreeHeap(), (unsigned)uxTaskGetStackHighWaterMark(NULL));
  worstUs = sumUs = nFrames = nLate = worstDispUs = 0;
}

// Render the frame after the one on screen into `prep` (building the next clip first if we are at the end of this one).
static void prepareNext() {
  const scenes::Frame* f;
  if (frameIdx + 1 < cur[0].frames.size()) f = &cur[0].frames[frameIdx + 1];
  else if (repeatClip) f = &cur[0].frames[0];
  else {
    if (nxt.empty()) build(nxt, wrap(curIdx + 1));
    if (nxt.empty()) return;
    f = &nxt[0].frames[0];
  }
  uint32_t t0 = micros();
  scenes::renderFrame(*f, *prep);
  uint32_t us = micros() - t0;
  if (us > worstUs) worstUs = us;
  sumUs += us; nFrames++;
  if (us / 1000 > holdMs) nLate++;  // the render did not fit inside this hold
}

static void present() {  // show `shown`, then start preparing the next one
  uint32_t t0 = micros();
  blit(*shown);
  display.display();
  uint32_t d = micros() - t0;
  if (d > worstDispUs) worstDispUs = d;
  shownAt = millis() ? millis() : 1;
  holdMs = holdFor(cur[0].frames[frameIdx]);
  prepareNext();
}

static void startClip(int idx) {
  curIdx = wrap(idx);
  build(cur, curIdx);
  nxt.clear(); nxt.shrink_to_fit();
  if (cur.empty()) return;
  Serial.printf("[%d/%d] %s (%s): %u frames, free heap %u\n", curIdx + 1, scenes::DINO_CLIP_COUNT, cur[0].name, cur[0].key,
                (unsigned)cur[0].frames.size(), (unsigned)ESP.getFreeHeap());
  frameIdx = 0;
  scenes::renderFrame(cur[0].frames[0], *shown);
  present();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_FREQ_HZ);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed - check wiring/address");
    for (;;) delay(1000);
  }
  Serial.printf("dino player: %d clips. n next, p previous, r repeat on/off, s stats, ? list\n", scenes::DINO_CLIP_COUNT);
  startClip(0);
}

void loop() {
  int target = curIdx;
  bool jump = false;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == 'n') { target = curIdx + 1; jump = true; }
    else if (c == 'p') { target = curIdx - 1; jump = true; }
    else if (c == 'r') { repeatClip = !repeatClip; Serial.printf("repeat clip %s\n", repeatClip ? "on" : "off"); }
    else if (c == 's') report();
    else if (c == '?') for (int i = 0; i < scenes::DINO_CLIP_COUNT; i++) Serial.printf("  %2d %s\n", i + 1, scenes::DINO_CLIP_KEYS[i]);
  }
  if (jump) { report(); startClip(target); return; }
  if (cur.empty() || millis() - shownAt < holdMs) { delay(1); return; }

  // hold over: show what was prepared
  face::Grid* t = shown; shown = prep; prep = t;
  if (++frameIdx >= cur[0].frames.size()) {
    report();
    if (repeatClip) frameIdx = 0;
    else {
      cur.swap(nxt);  // nxt was built in prepareNext()
      nxt.clear(); nxt.shrink_to_fit();
      curIdx = wrap(curIdx + 1);
      frameIdx = 0;
      Serial.printf("[%d/%d] %s (%s): %u frames, free heap %u\n", curIdx + 1, scenes::DINO_CLIP_COUNT, cur[0].name, cur[0].key,
                    (unsigned)cur[0].frames.size(), (unsigned)ESP.getFreeHeap());
    }
  }
  present();
}
