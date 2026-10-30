#include "imu_gestures.h"
#include <math.h>

namespace input {

static inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static inline float len(Vec3 v) { return sqrtf(dot(v, v)); }
static const float RAD2DEG = 57.2957795f;

Vec3 ImuGestures::unit(Vec3 v) {
  float l = len(v);
  if (l < 1e-6f) return {0, 0, 1};
  return {v.x / l, v.y / l, v.z / l};
}

Vec3 ImuGestures::frontFromFaceDown(Vec3 up, Vec3 r) {
  up = unit(up);
  Vec3 f = {-r.x, -r.y, -r.z};
  float d = dot(f, up);  // remove any part along up (Gram-Schmidt)
  return unit({f.x - d * up.x, f.y - d * up.y, f.z - d * up.z});
}

void ImuGestures::setConfig(const GestureConfig& c) {
  cfg_ = c;
  cfg_.up = unit(c.up);
  cfg_.front = frontFromFaceDown(cfg_.up, {-c.front.x, -c.front.y, -c.front.z});
  right_ = unit(cross(cfg_.up, cfg_.front));  // x = y cross z
}

void ImuGestures::push(Event e) {
  if (qLen_ < 4) queue_[(qHead_ + qLen_++) % 4] = e;  // drop if 4 are already waiting
}

Event ImuGestures::next() {
  if (!qLen_) return Event::None;
  Event e = queue_[qHead_];
  qHead_ = (qHead_ + 1) % 4; qLen_--;
  return e;
}

void ImuGestures::update(const ImuSample& s) {
  const uint32_t t = s.t_ms;
  const Vec3 a = s.accel_g;
  const float mag = len(a);
  const float gyro = len(s.gyro_dps);
  const float dev = fabsf(mag - 1.0f);
  const float aU = dot(a, cfg_.up), aF = dot(a, cfg_.front), aR = dot(a, right_);

  st_.accelMag = mag;
  st_.rollDeg = atan2f(-aR, aU) * RAD2DEG;   // + = top of the dino tilted to its right
  st_.pitchDeg = atan2f(aF, aU) * RAD2DEG;   // + = leaning back
  st_.still = gyro < cfg_.stillGyroDps && dev < cfg_.stillAccelG;
  if (!started_) { started_ = true; stillSince_ = t; }

  auto fire = [&](Event e) { push(e); };

  // ---- shake: several big |accel| peaks close together
  if (t >= shakeCooldownUntil_) {
    bool above = mag > cfg_.shakePeakG;
    if (above && !aboveShake_) {
      if (shakeCount_ == 0 || t - shakeStart_ > cfg_.shakeWindowMs) { shakeStart_ = t; shakeCount_ = 0; }
      if (++shakeCount_ >= cfg_.shakePeaks) {
        fire(Event::Shake);
        shakeCount_ = 0;
        shakeCooldownUntil_ = t + cfg_.shakeCooldownMs;
      }
    }
    aboveShake_ = above;
  }

  // ---- still / resting
  if (st_.still) { if (!stillSince_) stillSince_ = t; }
  else stillSince_ = 0;
  const uint32_t stillFor = stillSince_ ? t - stillSince_ : 0;

  // ---- pick up / put down
  if (!st_.held) {
    if (stillFor >= cfg_.restMs) st_.resting = true;
    if (st_.resting && !liftAt_ && dev > cfg_.liftAccelG) liftAt_ = t;
    if (liftAt_) {
      if (st_.still) liftAt_ = 0;  // just a bump on the desk
      else if (t - liftAt_ >= cfg_.liftConfirmMs) {
        liftAt_ = 0; st_.held = true; st_.resting = false;
        fire(Event::PickUp);
      }
    }
  } else if (stillFor >= cfg_.putDownMs) {
    st_.held = false; st_.resting = true;
    fire(Event::PutDown);
  }

  // ---- flip face-down / back up (only judged when |accel| is close to 1 g)
  if (dev < 0.3f) {
    if (!st_.faceDown) {
      if (aF < cfg_.faceDownDot) {
        if (!downSince_) downSince_ = t;
        if (t - downSince_ >= cfg_.flipHoldMs) { st_.faceDown = true; downSince_ = 0; fire(Event::FlipDown); }
      } else downSince_ = 0;
    } else {
      if (aF > cfg_.faceUpDot) {
        if (!upSince_) upSince_ = t;
        if (t - upSince_ >= 300) { st_.faceDown = false; upSince_ = 0; fire(Event::FlipUp); }
      } else upSince_ = 0;
    }
  }

  // ---- tilt left / right (only while roughly upright, not face-down)
  const bool upright = !st_.faceDown && aU > 0.5f && dev < 0.3f;
  const float r = st_.rollDeg;
  int8_t dir = 0;
  if (upright && r > cfg_.tiltOnDeg) dir = 1;
  else if (upright && r < -cfg_.tiltOnDeg) dir = -1;
  if (tiltLatched_) {
    if (!upright || fabsf(r) < cfg_.tiltOffDeg) { tiltLatched_ = false; tiltDir_ = 0; tiltSince_ = 0; }
    else if (cfg_.tiltRepeatMs && dir == tiltDir_ && t - tiltLastFire_ >= cfg_.tiltRepeatMs) {
      tiltLastFire_ = t;
      fire(tiltDir_ > 0 ? Event::TiltRight : Event::TiltLeft);
    }
  } else if (dir != 0) {
    if (dir != tiltDir_) { tiltDir_ = dir; tiltSince_ = t; }
    if (t - tiltSince_ >= cfg_.tiltHoldMs) {
      tiltLatched_ = true; tiltLastFire_ = t;
      fire(dir > 0 ? Event::TiltRight : Event::TiltLeft);
    }
  } else { tiltDir_ = 0; tiltSince_ = 0; }
}

}  // namespace input
