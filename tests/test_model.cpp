// Host tests of the model: sh tests/run.sh
#include "model.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace esphome::nas_power;

// Unlike assert, this also checks in a build with NDEBUG.
#define CHECK(what) \
  do { \
    if (!(what)) { \
      printf("%s:%d: failed: %s\n", __FILE__, __LINE__, #what); \
      exit(1); \
    } \
  } while (0)

// A device Home Assistant knows, with a power sensor that reads `watts` (idle below 5 W).
static Device device(bool on, float watts, bool busy = false, bool starting = false) {
  Device d;
  d.known = true;
  d.on = on;
  d.has_watts = true;
  d.watts = watts;
  d.busy = busy;
  d.starting = starting;
  return d;
}
// The same without a power sensor.
static Device bare(bool on, bool busy = false, bool starting = false) {
  Device d = device(on, 0, busy, starting);
  d.has_watts = false;
  return d;
}

static void test_states() {
  CHECK(is_running("on"));
  CHECK(is_running("home"));
  CHECK(!is_running("off"));
  CHECK(!is_running("not_home"));
  CHECK(!is_running("unavailable"));
  CHECK(!is_running(""));
  CHECK(!is_running(nullptr));

  CHECK(is_known("on"));
  CHECK(is_known("off"));
  CHECK(is_known("not_home"));
  CHECK(!is_known("unavailable"));
  CHECK(!is_known("unknown"));
  CHECK(!is_known(""));
  CHECK(!is_known(nullptr));
}

static void test_power() {
  float w = -1;
  CHECK(parse_number("42.4", w) && w > 42.3f && w < 42.5f);
  CHECK(parse_number("0", w) && w == 0);
  CHECK(!parse_number("", w));
  CHECK(!parse_number("unavailable", w));
  CHECK(!parse_number("unknown", w));
  CHECK(!parse_number(nullptr, w));
  CHECK(!parse_number("12abc", w));
  CHECK(!parse_number("nan", w));

  CHECK(to_watts(42, "W") == 42);
  CHECK(to_watts(0.5f, "kW") == 500);
  CHECK(to_watts(2, "MW") == 2000000);
  CHECK(to_watts(1500, "mW") == 1.5f);
  CHECK(to_watts(42, "") == 42);
  CHECK(to_watts(42, nullptr) == 42);
  CHECK(to_watts(42, "VA") == 42);

  CHECK(shown_watts(42.4f) == 42);
  CHECK(shown_watts(42.5f) == 43);
  CHECK(shown_watts(0.2f) == 0);
  CHECK(shown_watts(-3.0f) == 0);
}

static void test_control() {
  CHECK(control_kind("script.nas") == Control::SCRIPT);
  CHECK(control_kind("switch.server") == Control::SWITCH);
  CHECK(control_kind("input_boolean.nas") == Control::INPUT_BOOLEAN);
  CHECK(control_kind("light.lamp") == Control::NONE);
  CHECK(control_kind("script") == Control::NONE);
  CHECK(control_kind("") == Control::NONE);
  CHECK(control_kind(nullptr) == Control::NONE);

  // A script is run for both; a switch is turned on and off.
  CHECK(strcmp(control_action(Control::SCRIPT, true), "script.turn_on") == 0);
  CHECK(strcmp(control_action(Control::SCRIPT, false), "script.turn_on") == 0);
  CHECK(strcmp(control_action(Control::SWITCH, true), "switch.turn_on") == 0);
  CHECK(strcmp(control_action(Control::SWITCH, false), "switch.turn_off") == 0);
  CHECK(strcmp(control_action(Control::INPUT_BOOLEAN, true), "input_boolean.turn_on") == 0);
  CHECK(strcmp(control_action(Control::INPUT_BOOLEAN, false), "input_boolean.turn_off") == 0);
  CHECK(control_action(Control::NONE, true) == nullptr);
  CHECK(control_action(Control::NONE, false) == nullptr);
}

static void test_phase() {
  CHECK(phase(device(false, 0)) == Phase::OFF);
  CHECK(phase(device(true, 40)) == Phase::ON);
  // It draws power but does not answer yet: it boots, whoever started it.
  CHECK(phase(device(false, 60)) == Phase::STARTING);
  // Exactly the idle power draws; a little under does not.
  CHECK(phase(device(false, 5)) == Phase::STARTING);
  CHECK(phase(device(false, 4.9f)) == Phase::OFF);
  // Tapped, and nothing says more yet.
  CHECK(phase(device(false, 0, false, true)) == Phase::STARTING);
  CHECK(phase(bare(false, false, true)) == Phase::STARTING);
  // Its script runs while it is still there: it shuts down. Also once it stopped answering but still draws power.
  CHECK(phase(device(true, 40, true)) == Phase::STOPPING);
  CHECK(phase(device(false, 30, true)) == Phase::STOPPING);
  CHECK(phase(bare(true, true)) == Phase::STOPPING);
  // The script still runs (it waits to cut the power) but the device is off.
  CHECK(phase(device(false, 2, true)) == Phase::OFF);
  // A script that only starts it: busy with nothing there yet is not a shutdown.
  CHECK(phase(bare(false, true, true)) == Phase::STARTING);
  // Without a power sensor: the state alone.
  CHECK(phase(bare(false)) == Phase::OFF);
  CHECK(phase(bare(true)) == Phase::ON);
  // On wins over a start still counted.
  CHECK(phase(device(true, 40, false, true)) == Phase::ON);
  // Home Assistant has no state for it: nothing else is believed, not its power either.
  Device gone = device(false, 60, true, true);
  gone.known = false;
  CHECK(phase(gone) == Phase::UNKNOWN);
  CHECK(phase(Device{}) == Phase::UNKNOWN);

  CHECK(in_transit(Phase::STARTING) && in_transit(Phase::STOPPING));
  CHECK(!in_transit(Phase::OFF) && !in_transit(Phase::ON) && !in_transit(Phase::UNKNOWN));

  CHECK(strcmp(phase_key(Phase::UNKNOWN), "unavailable") == 0);
  CHECK(strcmp(phase_key(Phase::OFF), "off") == 0);
  CHECK(strcmp(phase_key(Phase::STARTING), "starting") == 0);
  CHECK(strcmp(phase_key(Phase::ON), "running") == 0);
  CHECK(strcmp(phase_key(Phase::STOPPING), "stopping") == 0);
}

static void test_tap() {
  CHECK(tap(device(false, 0), true) == Tap::START);
  CHECK(tap(device(true, 40), true) == Tap::ASK);
  CHECK(tap(device(false, 60), true) == Tap::NONE);        // starting
  CHECK(tap(device(true, 40, true), true) == Tap::NONE);   // shutting down
  // Nothing to act on: no entity, or no script or switch.
  CHECK(tap(device(false, 0), false) == Tap::NONE);
  CHECK(tap(device(true, 40), false) == Tap::NONE);
  // The script still runs after the device went off (it waits to cut the power): a tap would run it again.
  CHECK(phase(device(false, 2, true)) == Phase::OFF);
  CHECK(tap(device(false, 2, true), true) == Tap::NONE);
  // Its state is not known: a tap could start what runs, or shut down what nobody asked about.
  Device gone = device(false, 0);
  gone.known = false;
  CHECK(tap(gone, true) == Tap::NONE);
}

static void test_starting() {
  Starting s;
  CHECK(!s.active(1000, false));
  s.begin(1000);
  CHECK(s.active(1001, false));
  CHECK(s.active(1000 + STARTING_MS - 1, false));
  CHECK(!s.active(1000 + STARTING_MS, false));
  s.begin(5000);
  CHECK(!s.active(6000, true));   // it is there
  CHECK(!s.active(7000, false));  // and stays ended
  // Across the wrap of the millisecond clock.
  s.begin(0xFFFFFF00u);
  CHECK(s.active(0x00000100u, false));
}

static void test_blink() {
  CHECK(blink_lit(0));
  CHECK(blink_lit(999));
  CHECK(!blink_lit(1000));
  CHECK(blink_lit(2000));
}

static void test_resample() {
  // A mask that is full stays full at any size, an empty one empty.
  uint8_t full[16], empty[16] = {}, out[64];
  for (uint8_t &b : full) b = 255;
  resample(full, 4, 4, out, 2, 2);
  for (int i = 0; i < 4; ++i) CHECK(out[i] == 255);
  resample(full, 4, 4, out, 8, 8);
  for (int i = 0; i < 64; ++i) CHECK(out[i] == 255);
  resample(empty, 4, 4, out, 3, 3);
  for (int i = 0; i < 9; ++i) CHECK(out[i] == 0);
  // Half the size: each pixel is the mean of the four it covers.
  const uint8_t src[16] = {0, 255, 0, 0, 255, 0, 0, 0, 100, 100, 200, 200, 100, 100, 200, 200};
  resample(src, 4, 4, out, 2, 2);
  CHECK(out[0] == 128 && out[1] == 0 && out[2] == 100 && out[3] == 200);
  // The same size: the same mask.
  resample(src, 4, 4, out, 4, 4);
  for (int i = 0; i < 16; ++i) CHECK(out[i] == src[i]);
  // A mask that is wider than high, as the icon is: 4 x 2 made 2 x 1, each pixel the mean of its four.
  const uint8_t wide[8] = {0, 0, 200, 200, 0, 0, 100, 100};
  resample(wide, 4, 2, out, 2, 1);
  CHECK(out[0] == 0 && out[1] == 150);
  // And only narrower: 4 x 2 made 2 x 2 keeps the rows apart.
  resample(wide, 4, 2, out, 2, 2);
  CHECK(out[0] == 0 && out[1] == 200 && out[2] == 0 && out[3] == 100);
}

int main() {
  test_states();
  test_power();
  test_control();
  test_phase();
  test_tap();
  test_starting();
  test_blink();
  test_resample();
  printf("all model tests passed\n");
  return 0;
}
