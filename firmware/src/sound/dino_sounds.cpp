// Dino sounds from a square-wave piezo. A piezo can't roar, so these suggest a small dinosaur:
// gravelly growls (fast pulsing between close pitches), squeaky chirps, a rumbling purr, a yelp.
//
// Step = {start Hz, end Hz, ms, volume 0-100}. Start Hz 0 = rest.
// All sounds at volume 100 (a piezo on 3.3 V is quiet at best). Pitches sit in 1-4 kHz: a small
// piezo is loudest around 2-4 kHz and nearly silent below ~1 kHz (found on hardware).
// "Growl" = a run of short steps with rests: the pulsing is what reads as gravel on a piezo.
// These are first drafts: listen (tools/sound_preview, or the "p" key) and edit the numbers.
#include "dino_sounds.h"

namespace sound {

using utility::Sound;

#define N_(a) (uint8_t)(sizeof(a) / sizeof(a[0]))

// ---- the brain's sounds ----
// Menu select / break done: a short rising squeak "reep!".
static const Step CHIRP[] = {{1800, 3200, 90, 100}, {0, 0, 20, 0}, {3200, 2900, 50, 100}};
// Menu move / stopwatch minute: a short tick.
static const Step TICK[] = {{3000, 3000, 20, 100}};
// Alarm stage 1 (gentle): sleepy rumbling yawn, falling, with a little gravel at the end.
static const Step YAWN[] = {{2400, 1300, 500, 100}, {0, 0, 120, 0}, {1500, 1200, 30, 100}, {0, 0, 15, 0},
                            {1500, 1200, 30, 100}, {0, 0, 15, 0}, {1500, 1200, 30, 100}};
// Alarm stage 2: a proper growl that ends in a roar-ish rise.
static const Step ROAR[] = {{1700, 1300, 30, 100}, {0, 0, 14, 0}, {1700, 1300, 30, 100}, {0, 0, 14, 0},
                            {1700, 1300, 30, 100}, {0, 0, 14, 0}, {1700, 1300, 30, 100}, {0, 0, 14, 0},
                            {1300, 2800, 220, 100}, {2800, 1800, 160, 100}};
// Alarm stage 3: RAWR twice, higher and faster.
static const Step ROAR2[] = {{2000, 1500, 25, 100}, {0, 0, 10, 0}, {2000, 1500, 25, 100}, {0, 0, 10, 0},
                             {2000, 1500, 25, 100}, {0, 0, 10, 0}, {1600, 3800, 150, 100}, {3800, 2200, 150, 100},
                             {0, 0, 100, 0},
                             {2000, 1500, 25, 100}, {0, 0, 10, 0}, {2000, 1500, 25, 100}, {0, 0, 10, 0},
                             {1600, 4000, 160, 100}, {4000, 2400, 200, 100}};
// Snooze: falling grumpy "hmph" in two parts.
static const Step HMPH[] = {{1900, 1400, 140, 100}, {0, 0, 60, 0}, {1700, 1100, 200, 100}};
// Alarm set / focus done: fast warble between two notes.
static const Step TRILL[] = {{2600, 2600, 30, 100}, {3200, 3200, 30, 100}};
// Alarm off: falling two notes.
static const Step OFF[] = {{2400, 2400, 90, 100}, {0, 0, 40, 0}, {1600, 1600, 130, 100}};
// Focus start: a serious low "mm", settling to work.
static const Step SETTLE[] = {{2400, 1500, 220, 100}};
// Good morning (dismiss): purr, then a happy chirp.
static const Step PURR1[] = {{1100, 1150, 20, 100}, {0, 0, 20, 0}};
static const Step MORNING[] = {{1100, 1150, 20, 100}, {0, 0, 20, 0}, {1100, 1150, 20, 100}, {0, 0, 20, 0},
                               {1100, 1150, 20, 100}, {0, 0, 20, 0}, {1100, 1150, 20, 100}, {0, 0, 20, 0},
                               {1100, 1150, 20, 100}, {0, 0, 20, 0}, {1100, 1150, 20, 100}, {0, 0, 120, 0},
                               {2000, 3200, 130, 100}};

// ---- the dino's own reactions (voices) ----
// Pet, shy frame: rumbling purr, a 1.1 kHz tone pulsed ~25 times a second, about 1 s.
// (Pattern.repeat below plays PURR1 30 times.)
// Pet, grin frame / happy: fast warble, shorter than the alarm trill.
static const Step HAPPY_TRILL[] = {{2800, 2800, 30, 100}, {3400, 3400, 30, 100}};
// RAWR frame: a growl, then a short rising bark.
static const Step GROWL[] = {{1500, 1150, 30, 100}, {0, 0, 14, 0}, {1500, 1150, 30, 100}, {0, 0, 14, 0},
                             {1500, 1150, 30, 100}, {0, 0, 14, 0}, {1500, 1150, 30, 100}, {0, 0, 14, 0},
                             {1500, 1150, 30, 100}, {0, 0, 14, 0}, {1200, 3000, 130, 100}};
// Giggle after the RAWR: three quick squeaks.
static const Step GIGGLE[] = {{2800, 3400, 60, 100}, {0, 0, 40, 0}, {2800, 3400, 60, 100}, {0, 0, 40, 0}, {2800, 3600, 80, 100}};
// Shaken / picked up: a sharp falling yelp.
static const Step YELP[] = {{3800, 1800, 130, 100}};
// Dizzy: a wobbling up-down-up.
static const Step DIZZY[] = {{2000, 3000, 120, 100}, {3000, 1800, 120, 100}, {1800, 2800, 120, 100}, {2800, 2000, 140, 100}};
// Sad / hungry / low battery: two falling whimpers.
static const Step WHIMPER[] = {{2400, 1600, 200, 100}, {0, 0, 70, 0}, {2200, 1300, 280, 100}};
// Over-petted: a low grumbling "grrmph".
static const Step GRUMBLE[] = {{1400, 1100, 40, 100}, {0, 0, 20, 0}, {1400, 1100, 40, 100}, {0, 0, 20, 0},
                               {1400, 1000, 40, 100}, {0, 0, 20, 0}, {1300, 900, 160, 100}};
// Charging: three munches.
static const Step CHOMP[] = {{1800, 1800, 40, 100}, {0, 0, 60, 0}, {1800, 1800, 40, 100}, {0, 0, 60, 0}, {1800, 1800, 40, 100}};
// Surprise (bang mark): one high squeak.
static const Step SQUEAK[] = {{3200, 3800, 70, 100}};
// Falling asleep: a slow falling sigh.
static const Step SIGH[] = {{2200, 1200, 700, 100}};

// Test: stepped sweep 500 Hz - 5 kHz, 400 ms each, full volume, to find the piezo's loudest pitch.
static const Step SWEEP[] = {{500, 500, 400, 100},   {750, 750, 400, 100},   {1000, 1000, 400, 100},
                             {1500, 1500, 400, 100}, {2000, 2000, 400, 100}, {2500, 2500, 400, 100},
                             {3000, 3000, 400, 100}, {3500, 3500, 400, 100}, {4000, 4000, 400, 100},
                             {4500, 4500, 400, 100}, {5000, 5000, 400, 100}};

// Indices 0-10 are the brain's sounds (patternFor(Sound) uses them), 11 on are voices.
static const Named ALL[] = {
    {"chirp", {CHIRP, N_(CHIRP), 1, 0}},                     // 0
    {"tick", {TICK, N_(TICK), 1, 0}},                        // 1
    {"yawn (alarm 1)", {YAWN, N_(YAWN), 1, 2500}},           // 2
    {"roar (alarm 2)", {ROAR, N_(ROAR), 1, 1500}},           // 3
    {"roar roar (alarm 3)", {ROAR2, N_(ROAR2), 1, 700}},     // 4
    {"hmph (snooze)", {HMPH, N_(HMPH), 1, 0}},               // 5
    {"purr", {PURR1, N_(PURR1), 30, 0}},                     // 6
    {"trill", {TRILL, N_(TRILL), 5, 0}},                     // 7
    {"alarm off", {OFF, N_(OFF), 1, 0}},                     // 8
    {"settle (focus start)", {SETTLE, N_(SETTLE), 1, 0}},    // 9
    {"good morning (dismiss)", {MORNING, N_(MORNING), 1, 0}},  // 10
    {"voice: happy trill", {HAPPY_TRILL, N_(HAPPY_TRILL), 4, 0}},  // 11
    {"voice: growl (RAWR)", {GROWL, N_(GROWL), 1, 0}},       // 12
    {"voice: giggle", {GIGGLE, N_(GIGGLE), 1, 0}},           // 13
    {"voice: yelp", {YELP, N_(YELP), 1, 0}},                 // 14
    {"voice: dizzy", {DIZZY, N_(DIZZY), 1, 0}},              // 15
    {"voice: whimper", {WHIMPER, N_(WHIMPER), 1, 0}},        // 16
    {"voice: grumble", {GRUMBLE, N_(GRUMBLE), 1, 0}},        // 17
    {"voice: chomp", {CHOMP, N_(CHOMP), 1, 0}},              // 18
    {"voice: squeak", {SQUEAK, N_(SQUEAK), 1, 0}},           // 19
    {"voice: sigh", {SIGH, N_(SIGH), 1, 0}},                 // 20
};

static const Pattern SWEEP_P = {SWEEP, N_(SWEEP), 1, 0};
const Pattern& sweepPattern() { return SWEEP_P; }

const Named* allPatterns(int& count) {
  count = N_(ALL);
  return ALL;
}

Pattern patternFor(Sound s) {
  switch (s) {
    case Sound::AlarmStage1: return ALL[2].pattern;      // loops until the stage changes
    case Sound::AlarmStage2: return ALL[3].pattern;
    case Sound::AlarmStage3: return ALL[4].pattern;
    case Sound::Snooze: return ALL[5].pattern;
    case Sound::Dismiss: return ALL[10].pattern;
    case Sound::AlarmSet: return ALL[7].pattern;
    case Sound::AlarmOff: return ALL[8].pattern;
    case Sound::MenuMove: return ALL[1].pattern;
    case Sound::MenuSelect: return ALL[0].pattern;
    case Sound::FocusStart: return ALL[9].pattern;
    case Sound::FocusDone: return ALL[7].pattern;
    case Sound::BreakDone: return ALL[0].pattern;
    case Sound::StopwatchMinute: return ALL[1].pattern;
    default: return Pattern{nullptr, 0, 1, 0};
  }
}

Pattern patternFor(Voice v) {
  switch (v) {
    case Voice::Purr: return ALL[6].pattern;
    case Voice::Trill: return ALL[11].pattern;
    case Voice::Growl: return ALL[12].pattern;
    case Voice::Giggle: return ALL[13].pattern;
    case Voice::Yelp: return ALL[14].pattern;
    case Voice::Dizzy: return ALL[15].pattern;
    case Voice::Whimper: return ALL[16].pattern;
    case Voice::Grumble: return ALL[17].pattern;
    case Voice::Chomp: return ALL[18].pattern;
    case Voice::Squeak: return ALL[19].pattern;
    case Voice::Sigh: return ALL[20].pattern;
    default: return Pattern{nullptr, 0, 1, 0};
  }
}

}  // namespace sound
