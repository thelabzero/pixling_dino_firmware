#include "battery_level.h"

namespace power {

// Typical single-cell Li-ion resting voltage vs charge left. [Estimate: common published curve]
static const uint16_t MV[] = {3270, 3610, 3690, 3710, 3730, 3750, 3770, 3790, 3800, 3820, 3840,
                              3850, 3870, 3910, 3950, 3980, 4020, 4080, 4110, 4150, 4200};
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
