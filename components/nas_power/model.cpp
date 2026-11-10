#include "model.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace esphome::nas_power {

bool is_running(const char *state) {
  if (state == nullptr) return false;
  return strcmp(state, "on") == 0 || strcmp(state, "home") == 0;
}

bool parse_watts(const char *state, float &watts) {
  if (state == nullptr || *state == 0) return false;
  char *end = nullptr;
  const float v = strtof(state, &end);
  if (end == state || *end != 0 || !std::isfinite(v)) return false;
  watts = v;
  return true;
}

int shown_watts(float watts) { return watts <= 0 ? 0 : int(std::lround(watts)); }

Phase phase(bool on, bool has_watts, float watts, float idle_w, bool busy, bool starting) {
  const bool draws = has_watts && watts >= idle_w;
  // Its script runs while it is still there, or still draws power: it shuts down. A start counted here is not that.
  if (busy && !starting && (on || draws)) return Phase::STOPPING;
  if (on) return Phase::ON;
  if (starting || draws) return Phase::STARTING;
  return Phase::OFF;
}

Tap tap(Phase p, bool can_act) {
  if (!can_act) return Tap::NONE;
  if (p == Phase::OFF) return Tap::START;
  if (p == Phase::ON) return Tap::ASK;
  return Tap::NONE;
}

bool blink_lit(uint32_t now_ms) { return now_ms / 1000 % 2 == 0; }

void resample(const uint8_t *src, int from, int side, uint8_t *dst) {
  for (int y = 0; y < side; ++y)
    for (int x = 0; x < side; ++x) {
      const float x0 = float(x) * from / side, x1 = float(x + 1) * from / side;
      const float y0 = float(y) * from / side, y1 = float(y + 1) * from / side;
      float sum = 0, area = 0;
      for (int sy = int(y0); sy < int(std::ceil(y1)) && sy < from; ++sy)
        for (int sx = int(x0); sx < int(std::ceil(x1)) && sx < from; ++sx) {
          const float w = (std::min(x1, float(sx + 1)) - std::max(x0, float(sx))) *
                          (std::min(y1, float(sy + 1)) - std::max(y0, float(sy)));
          sum += w * src[sy * from + sx];
          area += w;
        }
      dst[y * side + x] = area > 0 ? uint8_t(sum / area + 0.5f) : 0;
    }
}

void Starting::begin(uint32_t now_ms) {
  since_ms = now_ms;
  on = true;
}

bool Starting::active(uint32_t now_ms, bool device_on) {
  if (on && (device_on || now_ms - since_ms >= STARTING_MS)) on = false;
  return on;
}

}  // namespace esphome::nas_power
