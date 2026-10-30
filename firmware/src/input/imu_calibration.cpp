#include "imu_calibration.h"
#include <math.h>

namespace input {

void ImuCorrector::apply(Vec3& a, Vec3& g, uint32_t t) {
  a = {(a.x - cal_.accOffsetG.x) / cal_.accScale.x,
       (a.y - cal_.accOffsetG.y) / cal_.accScale.y,
       (a.z - cal_.accOffsetG.z) / cal_.accScale.z};
  const Vec3 raw = g;
  Vec3& b = cal_.gyroBiasDps;
  g = {raw.x - b.x, raw.y - b.y, raw.z - b.z};

  const float gyro = sqrtf(g.x * g.x + g.y * g.y + g.z * g.z);
  const float dev = fabsf(sqrtf(a.x * a.x + a.y * a.y + a.z * a.z) - 1.0f);
  if (gyro < trk_.stillGyroDps && dev < trk_.stillAccelG) {
    if (!stillSince_) stillSince_ = t ? t : 1;
  } else {
    stillSince_ = 0;
  }
  learning_ = stillSince_ && t - stillSince_ >= trk_.stillMs;
  if (learning_) {
    b.x += trk_.rate * (raw.x - b.x);
    b.y += trk_.rate * (raw.y - b.y);
    b.z += trk_.rate * (raw.z - b.z);
  }
}

}  // namespace input
