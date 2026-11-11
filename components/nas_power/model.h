#pragma once
// The plugin's decisions, without LVGL and without Tessera, so the host tests (tests/run.sh) can check them: what the
// device does now, what a tap does then, which action reaches it, and how its power reads.
#include <cstdint>

namespace esphome::nas_power {

// UNKNOWN: Home Assistant has no state for the device (unavailable, unknown), so nothing here may guess.
enum class Phase { UNKNOWN, OFF, STARTING, ON, STOPPING };
enum class Tap { NONE, START, ASK };
// What starts the device and shuts it down, by its entity's domain.
enum class Control { NONE, SCRIPT, SWITCH, INPUT_BOOLEAN };

// How long a tap keeps "Starting" up while nothing else says the device boots, in milliseconds.
static constexpr uint32_t STARTING_MS = 300000;

// All that is known of the device at one moment.
struct Device {
  bool known = false;      // its entity has a state
  bool on = false;         // it answers
  bool has_watts = false;  // a power sensor reads a number
  float watts = 0;         // that number, in watts
  float idle_w = 5;        // under this it counts as off
  bool busy = false;       // its script runs
  bool starting = false;   // a tap here started it (Starting)
};

// A state that is a state: not none, "unavailable" or "unknown".
bool is_known(const char *state);
// A state that means the device is there: a ping or a switch that is on, a tracker that is home.
bool is_running(const char *state);
// A sensor's state as a number; false for none, "unknown", "unavailable" or anything that is not a number.
bool parse_number(const char *state, float &value);
// A power in its sensor's unit as watts: mW, W, kW and MW; any other unit, or none, is taken as watts.
float to_watts(float value, const char *unit);
// The power as shown: whole watts, never below zero.
int shown_watts(float watts);

// The kind of the control entity, NONE for none or another domain.
Control control_kind(const char *entity);
// The Home Assistant action that starts (`start`) or shuts down the device: a script is run for both, so it does the
// right one itself; a switch is turned on and off. nullptr for NONE.
const char *control_action(Control kind, bool start);

// What the device does.
Phase phase(const Device &d);
// The key of the screen's word for it (translations/, part "screen").
const char *phase_key(Phase p);
// Whether it is on its way: the icon blinks.
bool in_transit(Phase p);
// What a tap does: start it when it is off, ask before shutting it down; nothing while it is on its way, while its
// script still runs (a second run would be refused, or undo the first) or while there is nothing to act on.
Tap tap(const Device &d, bool can_act);
// The blink of a device on its way: lit in every other second.
bool blink_lit(uint32_t now_ms);

// A mask of `sw` x `sh` pixels made `dw` x `dh` into dst: each pixel the mean of the source pixels it covers. LVGL
// then draws it as it is, without a transform of its own.
void resample(const uint8_t *src, int sw, int sh, uint8_t *dst, int dw, int dh);

// From a tap that started the device until it answers, at most STARTING_MS.
struct Starting {
  uint32_t since_ms = 0;
  bool on = false;
  void begin(uint32_t now_ms);
  bool active(uint32_t now_ms, bool device_on);   // ends itself
};

}  // namespace esphome::nas_power
