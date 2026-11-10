#pragma once
// The tile's decisions, without LVGL and without Tessera, so the host tests (tests/run.sh) can check them: what the
// device does now, what a tap does then, and how its power reads.
#include <cstdint>

namespace esphome::nas_power {

enum class Phase { OFF, STARTING, ON, STOPPING };
enum class Tap { NONE, START, ASK };

// How long a tap keeps "Starting ..." up while nothing else says the device boots, in milliseconds.
static constexpr uint32_t STARTING_MS = 300000;

// A state that means the device is there: a ping or a switch that is on, a tracker that is home.
bool is_running(const char *state);
// A sensor's state as a number; false for none, "unknown", "unavailable" or anything that is not a number.
bool parse_watts(const char *state, float &watts);
// The power as shown: whole, never below zero.
int shown_watts(float watts);

// What the device does. `on`: it answers. `watts` against `idle_w`: it draws power (only with `has_watts`). `busy`: its
// script runs. `starting`: a tap here started it (Starting).
Phase phase(bool on, bool has_watts, float watts, float idle_w, bool busy, bool starting);
// What a tap does: start it when it is off, ask before shutting it down, nothing while it is on its way.
Tap tap(Phase p, bool can_act);
// The blink of a device on its way: lit in every other second.
bool blink_lit(uint32_t now_ms);

// A square mask of `from` pixels a side made `side` pixels a side into dst: each pixel the mean of the source pixels
// it covers. LVGL then draws it as it is, without a transform of its own.
void resample(const uint8_t *src, int from, int side, uint8_t *dst);

// From a tap that started the device until it answers, at most STARTING_MS.
struct Starting {
  uint32_t since_ms = 0;
  bool on = false;
  void begin(uint32_t now_ms);
  bool active(uint32_t now_ms, bool device_on);   // ends itself
};

}  // namespace esphome::nas_power
