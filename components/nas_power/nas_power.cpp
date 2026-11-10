#include "nas_power.h"
#include "confirm_card.h"
#include "power_tile.h"

namespace esphome::nas_power {

void NasPower::setup() {
  add_tile("power", [this]() { return new PowerTile(this); });
  add_card("confirm", [this]() { return new ConfirmCard(this); });
}

}  // namespace esphome::nas_power
