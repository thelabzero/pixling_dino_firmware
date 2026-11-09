// Boot-up animation preview: plays the three boot film candidates (src/boot/) on the OLED, one after the other.
//   pio run -e play_boot -t upload && pio device monitor -e play_boot
//
// Serial keys:  1 = A born (pixel, person, Dino, name)   2 = B dino (Dino hops onto the i)
//               3 = C zoom (name, dot, Dino, face, live face)   n next film   p previous film
//               r replay   l loop on/off   a play all three in a row   s stats   ? help
// Touch pad: next film.
//
// Stand-alone on purpose (nothing of the real firmware runs): this only previews the candidates so one can be
// chosen. Film C ends on the REAL face rig (Ex::Idle with its blink), the other frames come from src/boot/boot_film.
// Timing is millis()-driven and non-blocking: the schedule is cumulative, so a slow frame (render + display(),
// printed by `s`) never makes the whole film drift.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include "config.h"
#include "boot/boot_film.h"
#include "face/face.h"

static Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
// Big buffers are static (never on the stack): 3 x 8 KB.
static uint8_t work[boot::PIXELS], outPx[boot::PIXELS];
static face::Grid liveGrid;
static face::FaceO liveOver;

// The live face for film C: the real rig, idle face; variant 1/2 = half / closed lids like the sheet's blink.
static void liveFace(uint8_t* px, int variant) {
  using namespace face;
  liveOver = FaceO();
  if (variant == 1) liveOver.E(EyeO().lt(0.25).lb(0.25));
  else if (variant == 2) liveOver.E(EyeO().lt(0.5).lb(0.5));
  liveGrid.clear();
  faceDraw(liveGrid, resolve(Ex::Idle, liveOver));
  memcpy(px, liveGrid.d, boot::PIXELS);
}

static void blit(const uint8_t* px) {
  uint8_t* buf = display.getBuffer();
  for (int page = 0; page < boot::H / 8; page++)
    for (int x = 0; x < boot::W; x++) {
      uint8_t b = 0;
      for (int bit = 0; bit < 8; bit++)
        if (px[(page * 8 + bit) * boot::W + x]) b |= 1 << bit;
      buf[page * boot::W + x] = b;
    }
}

constexpr uint32_t GAP_MS = 1200;       // dark pause between two plays in loop mode
constexpr uint32_t TOUCH_LOCK_MS = 400;

static int ver = 0, stepIdx = 0;
static bool playing = false, loopMode = true, allMode = false, touchWas = false;
static uint32_t dueAt = 0, filmStart = 0, lastTouch = 0, gapUntil = 0;
// stats of the film on screen
static uint32_t worstRender, worstShow, nLate, sumRender;

static void showStep() {
  const boot::Film& f = boot::film(ver);
  const boot::Step& s = f.steps[stepIdx];
  uint32_t t0 = micros();
  boot::renderStep(s, work, outPx, liveFace);
  uint32_t render = micros() - t0;
  blit(outPx);
  t0 = micros();
  display.display();
  uint32_t show = micros() - t0;
  sumRender += render;
  if (render > worstRender) worstRender = render;
  if (show > worstShow) worstShow = show;
  if ((render + show) / 1000 > s.ms) nLate++;
}

static void stats() {
  const boot::Film& f = boot::film(ver);
  Serial.printf("  %s: %d steps, %lu ms of film, render avg %lu us worst %lu us, display() worst %lu us, steps shown late %lu, heap %u\n",
                f.name, f.n, (unsigned long)boot::totalMs(f), (unsigned long)(sumRender / (f.n ? f.n : 1)), (unsigned long)worstRender,
                (unsigned long)worstShow, (unsigned long)nLate, (unsigned)ESP.getFreeHeap());
}

static void start(int v) {
  ver = (v % boot::V_COUNT + boot::V_COUNT) % boot::V_COUNT;
  stepIdx = 0;
  worstRender = worstShow = nLate = sumRender = 0;
  playing = true;
  filmStart = millis();
  dueAt = filmStart + boot::film(ver).steps[0].ms;
  Serial.printf("[%s] %s (%lu ms)\n", boot::film(ver).key, boot::film(ver).name, (unsigned long)boot::totalMs(boot::film(ver)));
  showStep();
}

static void help() {
  Serial.println("boot preview: 1 A born | 2 B dino | 3 C zoom | n next | p prev | r replay | l loop | a all | s stats | ?");
  for (int i = 0; i < boot::V_COUNT; i++) Serial.printf("  %s  %s  (%lu ms)\n", boot::film(i).key, boot::film(i).name, (unsigned long)boot::totalMs(boot::film(i)));
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_FREQ_HZ);
  pinMode(PIN_TOUCH, INPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed - check wiring/address");
    for (;;) delay(1000);
  }
  display.clearDisplay();
  display.display();
  help();
  start(0);
}

void loop() {
  uint32_t now = millis();
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= '1' && c <= '3') { allMode = false; start(c - '1'); }
    else if (c == 'n') { allMode = false; start(ver + 1); }
    else if (c == 'p') { allMode = false; start(ver - 1); }
    else if (c == 'r') start(ver);
    else if (c == 'l') { loopMode = !loopMode; Serial.printf("loop %s\n", loopMode ? "on" : "off"); }
    else if (c == 'a') { allMode = true; start(0); Serial.println("playing all three in a row"); }
    else if (c == 's') stats();
    else if (c == '?') help();
  }
  bool touched = digitalRead(PIN_TOUCH) == (TOUCH_ACTIVE_HIGH ? HIGH : LOW);
  if (touched && !touchWas && now - lastTouch > TOUCH_LOCK_MS) { lastTouch = now; allMode = false; start(ver + 1); }
  touchWas = touched;

  if (!playing) {  // between plays: dark, then the next one
    if (now >= gapUntil) {
      if (allMode) start(ver + 1);
      else if (loopMode) start(ver);
    }
    return;
  }
  if ((int32_t)(now - dueAt) < 0) { delay(1); return; }
  const boot::Film& f = boot::film(ver);
  if (stepIdx + 1 < f.n) {
    stepIdx++;
    dueAt += f.steps[stepIdx].ms;  // cumulative schedule: no drift
    showStep();
  } else {  // film over
    playing = false;
    stats();
    if (allMode && ver + 1 >= boot::V_COUNT) allMode = false;
    gapUntil = now + GAP_MS;
    if (loopMode || allMode) {
      display.clearDisplay();
      display.display();
    }
  }
}
