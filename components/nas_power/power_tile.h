#pragma once
#include "esphome/components/smart_display/plugin_api.h"
#include "model.h"
#include <string>

namespace esphome::nas_power {

// A device that takes a while to start and must be shut down with care, such as a NAS: its icon, its name, what it
// does now and the power it draws. A tap starts it; while it runs, a tap asks before shutting it down. The decisions
// are model.h's.
class PowerTile : public tessera::Tile {
 public:
  explicit PowerTile(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void create(const tessera::TileContext &c) override;
  void on_state(JsonObjectConst data) override;
  void on_tick(uint32_t epoch) override;
  void on_theme() override { paint(); }
  void on_tap() override;

 private:
  int make_icon(lv_obj_t *parent);   // returns its width
  std::string title() const;         // the name given in the editor, or the entity's
  void paint();

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, tile_ = -1;
  int text_x_ = 0, top_ = 0;   // where the name starts: its left edge and its line
  tessera::Text entity_, control_, own_name_, name_;
  bool has_power_ = false;     // a power sensor is chosen
  Device device_;
  Starting starting_;
  Phase shown_ = Phase::UNKNOWN;
  bool lit_ = true;
  lv_obj_t *icon_{}, *title_{}, *status_{}, *power_{};
  tessera::List<uint8_t> mask_;   // the icon at this tile's size
  lv_image_dsc_t image_{};
};

}  // namespace esphome::nas_power
