// Screen: the SSD1306 OLED. Plays the dino clip for what the brain shows (machine.view(), built live by
// scenes/dino_live.*). On the home screen, a HomeScene module (the dino's day, modules/life.*) decides the
// clip instead, and may dim the screen at night. Tells whoever chose the clip when a one-shot clip has finished.
//
// Pipelined: while a frame is held on screen, the next one is rendered into a second buffer (a frame takes
// 8-216 ms to draw on the chip, a hold can be as short as 60 ms; found on hardware, docs/debug-play-dino.md).
#pragma once
#include <Adafruit_SSD1306.h>
#include "../app/module.h"
#include "../face/face.h"
#include "../scenes/dino_live.h"

class Screen : public Module {
 public:
  Screen();
  const char* name() const override { return "screen"; }
  bool begin(App& app) override;
  void loop(App& app, uint32_t now) override;
  void printInfo(Print& out) const override;

 private:
  Adafruit_SSD1306 display_;
  scenes::LiveClip clip_;
  utility::View playing_;      // the view the current clip was built from
  bool home_ = false;          // current clip comes from the HomeScene
  uint32_t homeSerial_ = 0;
  size_t frame_ = 0;           // frame on screen
  int prepared_ = -1;          // index of the frame already rendered into *prep_ (-1 = none)
  uint32_t frameAt_ = 0;
  bool finished_ = false;
  int contrast_ = -1;
  face::Grid bufA_, bufB_;     // 8 KB each: the Screen must not live on the stack
  face::Grid *shown_ = &bufA_, *prep_ = &bufB_;
  uint32_t lastRenderUs_ = 0, worstRenderUs_ = 0;
  void update(App& app, uint32_t now);
  void advance(App& app, uint32_t now);
  void build(App& app, const utility::View& v);
  void brightness(App& app);
  void show(size_t i, uint32_t now);   // render frame i now (blocks for one render), show it, then prepare the next
  int nextIndex() const;               // frame after the one on screen, or -1 when a one-shot clip is over
  void prepareNext();
  void present(const face::Grid& g);   // pack to the SSD1306 page layout and send
};
