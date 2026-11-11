#pragma once
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::nas_power {

// "Shut down now?": Cancel on the left, Shut down on the right. Opened by the tile while its device runs; the card's
// title is the device's name. It closes itself when the device stops answering, so its key never starts what is off.
class ConfirmCard : public tessera::Card {
 public:
  explicit ConfirmCard(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void open(const tessera::CardContext &c) override;
  void on_state(JsonObjectConst data) override;
  void on_theme() override { paint(); }
  void on_tap() override;

 private:
  struct Key {
    lv_obj_t *block{}, *word{};
    int x = 0, width = 0;
  };
  Key make_key(lv_obj_t *parent, int x, int width, const char *text_key);
  void paint();

  const tessera::Plugin *plugin_;
  tessera::Text control_;   // the script or switch that shuts the device down
  int keys_y_ = 0, key_h_ = 0;
  lv_obj_t *question_{};
  Key cancel_, shutdown_;
};

}  // namespace esphome::nas_power
