#include "time_sync.h"

namespace timekeeping {

void SyncSchedule::onFailure(uint32_t now) {
  static const uint32_t RETRY_MS[4] = {60000UL, 300000UL, 900000UL, 3600000UL};
  nextAt_ = now + RETRY_MS[failures_ < 3 ? failures_ : 3];
  if (failures_ < 255) failures_++;
}

void formatHHMM(bool valid, int h, int m, char out[6]) {
  if (!valid || h < 0 || h > 23 || m < 0 || m > 59) {
    out[0] = '-'; out[1] = '-'; out[2] = ':'; out[3] = '-'; out[4] = '-'; out[5] = 0;
    return;
  }
  out[0] = (char)('0' + h / 10); out[1] = (char)('0' + h % 10); out[2] = ':';
  out[3] = (char)('0' + m / 10); out[4] = (char)('0' + m % 10); out[5] = 0;
}

}  // namespace timekeeping
