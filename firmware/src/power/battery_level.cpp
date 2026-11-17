#include "battery_level.h"

namespace power {

// Voltage vs charge left, 0..100 % in steps of 5, where 0 % = the low-battery cutoff (3300 mV) and the
// percentage is the share of idle runtime left. [Measured: soak run 2026-11-09, this cell (KP411525), idle on the
// desk, 4 h 23 min, 3.94 V -> 3.30 V) and the run of 2026-11-10 (3.83 V -> 3.30 V). Both started at the blue light, so a
// full cell READS 3.83-3.94 V on this board: the ADC under-reads (a 330k divider is a weak source; assumption: about
// 6-9 %, a multimeter would tell). The curve is in the board's own readings, so 100 % is ~3.96 V here, not 4.2 V.]
static const uint16_t MV[] = {3300, 3440, 3500, 3527, 3556, 3580, 3605, 3620, 3638, 3653, 3676,
                              3693, 3715, 3741, 3760, 3783, 3808, 3844, 3884, 3915, 3960};
static const int N = sizeof(MV) / sizeof(MV[0]);  // 21 points: 0, 5, 10 ... 100 %

int lipoPercent(uint32_t mv) {
  if (mv <= MV[0]) return 0;
  if (mv >= MV[N - 1]) return 100;
  for (int i = 1; i < N; i++)
    if (mv < MV[i]) {  // between point i-1 and i: interpolate
      const float f = (float)(mv - MV[i - 1]) / (float)(MV[i] - MV[i - 1]);
      return (int)((i - 1) * 5 + f * 5 + 0.5f);
    }
  return 100;
}

}  // namespace power
