#include "power_tile.h"
#include "icon.h"
#include "esphome/core/hal.h"
#include <algorithm>
#include <cstdio>

namespace esphome::nas_power {

using tessera::Font;
namespace ui = tessera::ui;

void PowerTile::create(const tessera::TileContext &c) {
  width_ = c.width;
  height_ = c.height;
  tile_ = c.tile;
  entity_ = roomy::text(std::string(c.entity ? c.entity : ""));
  own_name_ = roomy::text(std::string(c.name ? c.name : ""));
  control_ = roomy::text(std::string(c.options["control"] | ""));
  has_power_ = (c.options["power"] | "")[0] != 0;
  device_.idle_w = c.options["idle"] | 5.0f;

  // The icon on the left, in the middle of the height; the name over what it does beside it; the power on the right
  // of the name's line.
  const int lh = ui::line_height(Font::BODY);
  text_x_ = make_icon(c.parent) + ui::px(10);
  top_ = (height_ - 2 * lh) / 2;
  title_ = ui::label(c.parent, Font::BODY, theme::INK);
  lv_obj_set_pos(title_, text_x_, top_);
  status_ = ui::label(c.parent, Font::BODY, theme::MUTED);
  lv_obj_set_pos(status_, text_x_, top_ + lh);
  lv_obj_set_width(status_, std::max(0, width_ - text_x_));
  power_ = ui::label(c.parent, Font::BODY, theme::INK_SOFT);
  lv_obj_set_style_text_align(power_, LV_TEXT_ALIGN_RIGHT, 0);
  paint();
}

// The plugin's own NAS icon (icon.h), a mask at this tile's size painted in a theme colour: a little over half the
// tile's height wide, never narrower than an icon of the font and never wider than a finger, the mask itself or a third of the tile.
int PowerTile::make_icon(lv_obj_t *parent) {
  const int ih = ui::line_height(Font::ICON);
  const int most = std::min({ui::mm(10), ICON_WIDTH, height_, width_ / 3});
  const int w = std::max(1, std::min(std::max(ih, height_ * 55 / 100), most));
  const int h = std::max(1, w * ICON_HEIGHT / ICON_WIDTH);
  mask_.assign(size_t(w) * h, 0);
  resample(ICON_NAS, ICON_WIDTH, ICON_HEIGHT, mask_.data(), w, h);
  image_.header.magic = LV_IMAGE_HEADER_MAGIC;
  image_.header.cf = LV_COLOR_FORMAT_A8;
  image_.header.w = w;
  image_.header.h = h;
  image_.header.stride = w;
  image_.data = mask_.data();
  image_.data_size = mask_.size();
  icon_ = lv_image_create(parent);
  lv_obj_remove_flag(icon_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_image_recolor_opa(icon_, LV_OPA_COVER, 0);
  lv_image_set_src(icon_, &image_);
  lv_obj_set_size(icon_, w, h);
  lv_obj_set_pos(icon_, 0, (height_ - h) / 2);
  return w;
}

void PowerTile::on_state(JsonObjectConst data) {
  const char *state = data["state"] | "";
  device_.known = is_known(state);
  device_.on = is_running(state);
  name_ = roomy::text(std::string(data["name"] | ""));
  JsonObjectConst power = data["entities"]["power"], control = data["entities"]["control"];
  float value = 0;
  device_.has_watts = has_power_ && parse_number(power["state"] | "", value);
  device_.watts = to_watts(value, power["attributes"]["unit_of_measurement"] | "W");
  // A script is "on" while it runs; a switch that is on is the device itself, not something on its way.
  device_.busy = control_kind(control_.c_str()) == Control::SCRIPT && is_running(control["state"] | "");
  paint();
}

void PowerTile::on_tick(uint32_t) {
  const uint32_t ms = esphome::millis();
  device_.starting = starting_.active(ms, device_.on);   // ends itself when the device is there, or after too long
  const Phase p = phase(device_);
  // On its way: the icon blinks once a second, painted here (no animation of its own).
  const bool lit = !in_transit(p) || blink_lit(ms);
  if (p != shown_ || lit != lit_) {
    lit_ = lit;
    paint();
  }
}

std::string PowerTile::title() const { return roomy::plain(own_name_.empty() ? name_ : own_name_); }

void PowerTile::paint() {
  if (!icon_) return;
  const int room = std::max(0, width_ - text_x_);
  if (entity_.empty()) {
    lv_obj_set_style_image_recolor(icon_, ui::color(theme::SUBTLE), 0);
    ui::set_text(title_, plugin_->text("choose_device"));
    ui::set_color(title_, theme::MUTED);
    lv_obj_set_width(title_, room);
    ui::set_text(status_, std::string(""));
    ui::set_text(power_, std::string(""));
    return;
  }
  const Phase p = shown_ = phase(device_);
  // The accent says it is there; on its way the icon blinks between the accent and the muted of "off".
  const bool accent = p == Phase::ON || (in_transit(p) && lit_);
  const theme::Role ink = accent ? theme::ACCENT : p == Phase::UNKNOWN ? theme::SUBTLE : theme::MUTED;
  lv_obj_set_style_image_recolor(icon_, ui::color(ink), 0);
  ui::set_text(status_, plugin_->text(phase_key(p)));
  ui::set_color(status_, theme::MUTED);

  // The power beside the name: the name gets what the number leaves.
  std::string power;
  if (device_.has_watts) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d W", shown_watts(device_.watts));
    power = buf;
  }
  const int pw = power.empty() ? 0 : ui::text_width(power, Font::BODY);
  const int gap = power.empty() ? 0 : ui::px(8);
  ui::set_text(power_, power);
  ui::set_color(power_, theme::INK_SOFT);
  lv_obj_set_width(power_, pw);
  lv_obj_set_pos(power_, width_ - pw, top_);
  ui::set_text(title_, title());
  ui::set_color(title_, theme::INK);
  lv_obj_set_width(title_, std::max(0, room - pw - gap));
}

// Off: start it. Running: ask first, on the card. On its way: nothing, a second start or stop would only confuse it.
void PowerTile::on_tap() {
  const std::string control = roomy::plain(control_);
  const Control kind = control_kind(control.c_str());
  switch (tap(device_, !entity_.empty() && kind != Control::NONE)) {
    case Tap::NONE:
      return;
    case Tap::ASK:
      tessera::open_card(plugin_->plugin_id(), "confirm", control, tile_, title());
      return;
    case Tap::START:
      if (!tessera::action(control_action(kind, true), control)) return;
      starting_.begin(esphome::millis());
      device_.starting = true;
      lit_ = true;
      paint();
      return;
  }
}

}  // namespace esphome::nas_power
