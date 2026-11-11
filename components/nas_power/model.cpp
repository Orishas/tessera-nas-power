#include "model.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace esphome::nas_power {

static bool is(const char *text, const char *word) { return strcmp(text, word) == 0; }
static bool starts_with(const char *text, const char *prefix) { return strncmp(text, prefix, strlen(prefix)) == 0; }

bool is_known(const char *state) {
  return state != nullptr && *state != 0 && !is(state, "unavailable") && !is(state, "unknown");
}

bool is_running(const char *state) { return state != nullptr && (is(state, "on") || is(state, "home")); }

bool parse_number(const char *state, float &value) {
  if (state == nullptr || *state == 0) return false;
  char *end = nullptr;
  const float v = strtof(state, &end);
  if (end == state || *end != 0 || !std::isfinite(v)) return false;
  value = v;
  return true;
}

float to_watts(float value, const char *unit) {
  if (unit == nullptr) return value;
  if (is(unit, "kW")) return value * 1000.0f;
  if (is(unit, "MW")) return value * 1000000.0f;
  if (is(unit, "mW")) return value / 1000.0f;
  return value;
}

int shown_watts(float watts) { return watts <= 0 ? 0 : int(std::lround(watts)); }

Control control_kind(const char *entity) {
  if (entity == nullptr) return Control::NONE;
  if (starts_with(entity, "script.")) return Control::SCRIPT;
  if (starts_with(entity, "switch.")) return Control::SWITCH;
  if (starts_with(entity, "input_boolean.")) return Control::INPUT_BOOLEAN;
  return Control::NONE;
}

const char *control_action(Control kind, bool start) {
  switch (kind) {
    case Control::SCRIPT: return "script.turn_on";
    case Control::SWITCH: return start ? "switch.turn_on" : "switch.turn_off";
    case Control::INPUT_BOOLEAN: return start ? "input_boolean.turn_on" : "input_boolean.turn_off";
    case Control::NONE: break;
  }
  return nullptr;
}

Phase phase(const Device &d) {
  if (!d.known) return Phase::UNKNOWN;
  const bool draws = d.has_watts && d.watts >= d.idle_w;
  // Its script runs while it is still there, or still draws power: it shuts down. A start counted here is not that.
  if (d.busy && !d.starting && (d.on || draws)) return Phase::STOPPING;
  if (d.on) return Phase::ON;
  if (d.starting || draws) return Phase::STARTING;
  return Phase::OFF;
}

const char *phase_key(Phase p) {
  switch (p) {
    case Phase::UNKNOWN: return "unavailable";
    case Phase::OFF: return "off";
    case Phase::STARTING: return "starting";
    case Phase::ON: return "running";
    case Phase::STOPPING: return "stopping";
  }
  return "off";
}

bool in_transit(Phase p) { return p == Phase::STARTING || p == Phase::STOPPING; }

Tap tap(const Device &d, bool can_act) {
  if (!can_act || d.busy) return Tap::NONE;
  switch (phase(d)) {
    case Phase::OFF: return Tap::START;
    case Phase::ON: return Tap::ASK;
    default: return Tap::NONE;
  }
}

bool blink_lit(uint32_t now_ms) { return now_ms / 1000 % 2 == 0; }

void resample(const uint8_t *src, int sw, int sh, uint8_t *dst, int dw, int dh) {
  for (int y = 0; y < dh; ++y) {
    const float y0 = float(y) * sh / dh, y1 = float(y + 1) * sh / dh;
    for (int x = 0; x < dw; ++x) {
      const float x0 = float(x) * sw / dw, x1 = float(x + 1) * sw / dw;
      float sum = 0, area = 0;
      for (int sy = int(y0); sy < int(std::ceil(y1)) && sy < sh; ++sy)
        for (int sx = int(x0); sx < int(std::ceil(x1)) && sx < sw; ++sx) {
          const float w = (std::min(x1, float(sx + 1)) - std::max(x0, float(sx))) *
                          (std::min(y1, float(sy + 1)) - std::max(y0, float(sy)));
          sum += w * src[sy * sw + sx];
          area += w;
        }
      dst[y * dw + x] = area > 0 ? uint8_t(sum / area + 0.5f) : 0;
    }
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
