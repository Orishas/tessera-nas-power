// Host tests of the model: sh tests/run.sh
#include "model.h"
#include <cassert>
#include <cstdio>

using namespace esphome::nas_power;

static void test_is_running() {
  assert(is_running("on"));
  assert(is_running("home"));
  assert(!is_running("off"));
  assert(!is_running("not_home"));
  assert(!is_running("unavailable"));
  assert(!is_running(""));
  assert(!is_running(nullptr));
}

static void test_parse_watts() {
  float w = -1;
  assert(parse_watts("42.4", w) && w > 42.3f && w < 42.5f);
  assert(parse_watts("0", w) && w == 0);
  assert(!parse_watts("", w));
  assert(!parse_watts("unavailable", w));
  assert(!parse_watts("unknown", w));
  assert(!parse_watts(nullptr, w));
  assert(!parse_watts("12abc", w));
}

static void test_shown_watts() {
  assert(shown_watts(42.4f) == 42);
  assert(shown_watts(42.5f) == 43);
  assert(shown_watts(0.2f) == 0);
  assert(shown_watts(-3.0f) == 0);
}

static void test_phase() {
  // on, has_watts, watts, idle, busy, starting
  assert(phase(false, true, 0, 5, false, false) == Phase::OFF);
  assert(phase(true, true, 40, 5, false, false) == Phase::ON);
  // It draws power but does not answer yet: it boots, whoever started it.
  assert(phase(false, true, 60, 5, false, false) == Phase::STARTING);
  // Tapped, and nothing says more yet.
  assert(phase(false, true, 0, 5, false, true) == Phase::STARTING);
  assert(phase(false, false, 0, 5, false, true) == Phase::STARTING);
  // Its script runs while it is still there: it shuts down. Also once it stopped answering but still draws power.
  assert(phase(true, true, 40, 5, true, false) == Phase::STOPPING);
  assert(phase(false, true, 30, 5, true, false) == Phase::STOPPING);
  // The script still runs (it waits to cut the power) but the device is off.
  assert(phase(false, true, 2, 5, true, false) == Phase::OFF);
  // A script that only starts it: busy with nothing there yet is not a shutdown.
  assert(phase(false, false, 0, 5, true, true) == Phase::STARTING);
  // Without a power sensor: the state alone.
  assert(phase(false, false, 0, 5, false, false) == Phase::OFF);
  assert(phase(true, false, 0, 5, false, false) == Phase::ON);
  // On wins over a start still counted.
  assert(phase(true, true, 40, 5, false, true) == Phase::ON);
}

static void test_tap() {
  assert(tap(Phase::OFF, true) == Tap::START);
  assert(tap(Phase::ON, true) == Tap::ASK);
  assert(tap(Phase::STARTING, true) == Tap::NONE);
  assert(tap(Phase::STOPPING, true) == Tap::NONE);
  assert(tap(Phase::OFF, false) == Tap::NONE);
  assert(tap(Phase::ON, false) == Tap::NONE);
}

static void test_starting() {
  Starting s;
  assert(!s.active(1000, false));
  s.begin(1000);
  assert(s.active(1001, false));
  assert(s.active(1000 + STARTING_MS - 1, false));
  assert(!s.active(1000 + STARTING_MS, false));
  s.begin(5000);
  assert(!s.active(6000, true));   // it is there
  assert(!s.active(7000, false));  // and stays ended
  // Across the wrap of the millisecond clock.
  s.begin(0xFFFFFF00u);
  assert(s.active(0x00000100u, false));
}

static void test_blink() {
  assert(blink_lit(0));
  assert(blink_lit(999));
  assert(!blink_lit(1000));
  assert(blink_lit(2000));
}

static void test_resample() {
  // A mask that is full stays full at any size, an empty one empty.
  uint8_t full[16], empty[16] = {}, out[64];
  for (uint8_t &b : full) b = 255;
  resample(full, 4, 2, out);
  for (int i = 0; i < 4; ++i) assert(out[i] == 255);
  resample(full, 4, 8, out);
  for (int i = 0; i < 64; ++i) assert(out[i] == 255);
  resample(empty, 4, 3, out);
  for (int i = 0; i < 9; ++i) assert(out[i] == 0);
  // Half the size: each pixel is the mean of the four it covers.
  const uint8_t src[16] = {0, 255, 0, 0, 255, 0, 0, 0, 100, 100, 200, 200, 100, 100, 200, 200};
  resample(src, 4, 2, out);
  assert(out[0] == 128 && out[1] == 0 && out[2] == 100 && out[3] == 200);
  // The same size: the same mask.
  resample(src, 4, 4, out);
  for (int i = 0; i < 16; ++i) assert(out[i] == src[i]);
}

int main() {
  test_resample();
  test_is_running();
  test_parse_watts();
  test_shown_watts();
  test_phase();
  test_tap();
  test_starting();
  test_blink();
  printf("all model tests passed\n");
  return 0;
}
