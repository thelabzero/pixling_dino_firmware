// Dino's voice: which sound pattern plays for each of the brain's sounds, and for the dino's own
// reactions ("voices", used by the life brain). The shapes are in dino_sounds.cpp as plain tables:
// edit them there, listen with tools/sound_preview (renders .wav files), then upload.
#pragma once
#include "tone_player.h"
#include "../utility/machine.h"

namespace sound {

// Pattern for a brain sound (alarm, menu, focus...); n = 0 for none.
Pattern patternFor(utility::Sound s);

// The dino's own reactions, named after the design sheet's Life clips.
enum class Voice : uint8_t {
  None,
  Purr,     // pet: under the shy frame
  Trill,    // pet: on the grin; also happy / squeezed-eyes moments
  Growl,    // RAWR: starts on the RAWR frame
  Giggle,   // RAWR: the giggle after it
  Yelp,     // shaken / picked up: the jolt
  Dizzy,    // shaken: spinning pupils
  Whimper,  // sad, hungry, low battery
  Grumble,  // over-petted
  Chomp,    // charging: munching the leaf
  Squeak,   // surprise (bang mark)
  Sigh,     // falling asleep
  Count
};
Pattern patternFor(Voice v);

// Named patterns, for previews and the "p" key.
struct Named { const char* name; Pattern pattern; };
const Named* allPatterns(int& count);

// Test sweep: 500 Hz to 5 kHz in steps, to find the piezo's loudest pitch (the "r" key).
const Pattern& sweepPattern();

}  // namespace sound
