#include "power_tile.h"
#include "icon.h"
#include "esphome/core/hal.h"
#include <algorithm>
#include <cstdio>
#include <string>

namespace esphome::nas_power {

using tessera::Font;
namespace ui = tessera::ui;

static bool starts_with(const std::string &s, const char *prefix) { return s.rfind(prefix, 0) == 0; }

void PowerTile::create(const tessera::TileContext &c) {
  width_ = c.width;
  height_ = c.height;
  tile_ = c.tile;
  entity_ = roomy::text(std::string(c.entity ? c.entity : ""));
  own_name_ = roomy::text(std::string(c.name ? c.name : ""));
  control_ = roomy::text(std::string(c.options["control"] | ""));
  has_power_ = (c.options["power"] | "")[0] != 0;
  idle_w_ = c.options["idle"] | 5.0f;

  // The icon on the left, in the middle of the height; the name over what it does beside it; the power on the right
  // of the name's line.
  const int ih = ui::line_height(Font::ICON), lh = ui::line_height(Font::BODY);
  const int gap = ui::px(10);
  // The plugin's own NAS icon (icon.h), a mask at this tile's size painted in a theme colour: a little over half
  // the height, never smaller than an icon of the font and never larger than a finger.
  const int side = std::max(1, std::min({std::max(ih, height_ * 55 / 100), ui::mm(10), height_, width_ / 3}));
  mask_.assign(size_t(side) * side, 0);
  resample(ICON_NAS, ICON_SIZE, side, mask_.data());
  image_.header.magic = LV_IMAGE_HEADER_MAGIC;
  image_.header.cf = LV_COLOR_FORMAT_A8;
  image_.header.w = side;
  image_.header.h = side;
  image_.header.stride = side;
  image_.data = mask_.data();
  image_.data_size = mask_.size();
  icon_ = lv_image_create(c.parent);
  lv_obj_remove_flag(icon_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_image_recolor_opa(icon_, LV_OPA_COVER, 0);
  lv_image_set_src(icon_, &image_);
  lv_obj_set_size(icon_, side, side);
  lv_obj_set_pos(icon_, 0, (height_ - side) / 2);
  text_x_ = side + gap;
  const int top = top_ = (height_ - 2 * lh) / 2;
  title_ = ui::label(c.parent, Font::BODY, theme::INK);
  lv_obj_set_pos(title_, text_x_, top);
  status_ = ui::label(c.parent, Font::BODY, theme::MUTED);
  lv_obj_set_pos(status_, text_x_, top + lh);
  lv_obj_set_width(status_, std::max(0, width_ - text_x_));
  power_ = ui::label(c.parent, Font::BODY, theme::INK_SOFT);
  lv_obj_set_style_text_align(power_, LV_TEXT_ALIGN_RIGHT, 0);
  paint();
}

void PowerTile::on_state(JsonObjectConst data) {
  on_ = is_running(data["state"] | "");
  name_ = roomy::text(std::string(data["name"] | ""));
  JsonObjectConst e = data["entities"];
  has_watts_ = has_power_ && parse_watts(e["power"]["state"] | "", watts_);
  snprintf(unit_, sizeof(unit_), "%s", e["power"]["attributes"]["unit_of_measurement"] | "W");
  // A script is "on" while it runs; a switch that is on is the device itself, not something on its way.
  busy_ = starts_with(roomy::plain(control_), "script.") && std::string(e["control"]["state"] | "") == "on";
  paint();
}

Phase PowerTile::now() const {
  return phase(on_, has_watts_, watts_, idle_w_, busy_, starting_.on);
}

void PowerTile::on_tick(uint32_t) {
  const uint32_t ms = esphome::millis();
  starting_.active(ms, on_);   // ends itself when the device is there, or after too long
  const Phase p = now();
  // On its way: the icon blinks once a second, painted here (no animation of its own).
  const bool lit = (p == Phase::STARTING || p == Phase::STOPPING) ? blink_lit(ms) : true;
  if (p != shown_ || lit != lit_) {
    lit_ = lit;
    paint();
  }
}

void PowerTile::paint() {
  if (!icon_) return;
  if (entity_.empty()) {
    lv_obj_set_style_image_recolor(icon_, ui::color(theme::SUBTLE), 0);
    ui::set_text(title_, plugin_->text("choose_device"));
    ui::set_color(title_, theme::MUTED);
    lv_obj_set_width(title_, std::max(0, width_ - text_x_));
    ui::set_text(status_, std::string(""));
    ui::set_text(power_, std::string(""));
    return;
  }
  const Phase p = now();
  shown_ = p;
  const char *word = p == Phase::ON ? "running" : p == Phase::STARTING ? "starting" : p == Phase::STOPPING ? "stopping" : "off";
  // The accent says it is there; on its way the icon blinks between the accent and the muted of "off".
  const bool accent = p == Phase::ON || ((p == Phase::STARTING || p == Phase::STOPPING) && lit_);
  lv_obj_set_style_image_recolor(icon_, ui::color(accent ? theme::ACCENT : theme::MUTED), 0);
  ui::set_text(status_, plugin_->text(word));
  ui::set_color(status_, theme::MUTED);

  // The power beside the name: the name gets what the number leaves.
  std::string power;
  if (has_watts_) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%d %s", shown_watts(watts_), unit_);
    power = buf;
  }
  const int pw = power.empty() ? 0 : ui::text_width(power, Font::BODY);
  const int room = std::max(0, width_ - text_x_);
  const int gap = power.empty() ? 0 : ui::px(8);
  ui::set_text(power_, power);
  ui::set_color(power_, theme::INK_SOFT);
  lv_obj_set_width(power_, pw);
  lv_obj_set_pos(power_, width_ - pw, top_);
  ui::set_text(title_, roomy::plain(own_name_.empty() ? name_ : own_name_));
  ui::set_color(title_, theme::INK);
  lv_obj_set_width(title_, std::max(0, room - pw - gap));
}

// Off: start it. Running: ask first, on the card. On its way: nothing, a second start or stop would only confuse it.
void PowerTile::on_tap() {
  const std::string control = roomy::plain(control_);
  const Tap t = tap(now(), !entity_.empty() && !control.empty());
  if (t == Tap::NONE) return;
  if (t == Tap::ASK) {
    tessera::open_card(plugin_->plugin_id(), "confirm", control, tile_, roomy::plain(own_name_.empty() ? name_ : own_name_));
    return;
  }
  const char *service = starts_with(control, "script.") ? "script.turn_on"
                        : starts_with(control, "input_boolean.") ? "input_boolean.turn_on" : "switch.turn_on";
  if (tessera::action(service, control)) {
    starting_.begin(esphome::millis());
    lit_ = true;
    paint();
  }
}

}  // namespace esphome::nas_power
