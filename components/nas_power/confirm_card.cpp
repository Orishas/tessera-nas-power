#include "confirm_card.h"
#include "model.h"
#include <algorithm>
#include <string>

namespace esphome::nas_power {

using tessera::Font;
namespace ui = tessera::ui;

ConfirmCard::Key ConfirmCard::make_key(lv_obj_t *parent, int x, int width, const char *text_key) {
  Key key;
  key.x = x;
  key.width = width;
  key.block = ui::block(parent, theme::TRACK);
  lv_obj_set_style_pad_all(key.block, 0, 0);
  lv_obj_remove_flag(key.block, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(key.block, x, keys_y_);
  lv_obj_set_size(key.block, width, key_h_);
  key.word = ui::label(key.block, Font::BODY_LARGE, theme::INK);
  lv_obj_set_style_text_align(key.word, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(key.word, width);
  lv_obj_set_pos(key.word, 0, (key_h_ - ui::line_height(Font::BODY_LARGE)) / 2);
  ui::set_text(key.word, plugin_->text(text_key));
  return key;
}

// The question over the middle of the room, the two keys under it: each half the width.
void ConfirmCard::open(const tessera::CardContext &c) {
  control_ = roomy::text(std::string(c.entity ? c.entity : ""));
  const int gap = ui::px(12), lh = ui::line_height(Font::BODY_LARGE);
  question_ = ui::text(c.parent, Font::BODY_LARGE, theme::INK);
  lv_obj_set_width(question_, c.width);
  lv_obj_set_style_text_align(question_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_height(question_, 2 * lh);
  lv_obj_set_pos(question_, 0, std::max(0, c.height / 2 - 2 * lh - gap));
  ui::set_text(question_, plugin_->text("shutdown_q"));
  key_h_ = std::max(ui::mm(12), std::min(c.height / 3, ui::mm(20)));
  keys_y_ = c.height / 2 + gap;
  const int half = (c.width - gap) / 2;
  cancel_ = make_key(c.parent, 0, half, "cancel");
  shutdown_ = make_key(c.parent, half + gap, c.width - half - gap, "shutdown");
  paint();
}

// The tile's data: the device went away while the question stood (someone else shut it down). The key would now run
// the script on a device that is off, and start it.
void ConfirmCard::on_state(JsonObjectConst data) {
  const char *state = data["state"] | "";
  if (*state != 0 && !is_running(state)) tessera::close_card();
}

void ConfirmCard::paint() {
  if (!question_) return;
  ui::set_color(question_, theme::INK);
  ui::set_fill(cancel_.block, tessera::Tone::NORMAL, theme::TRACK);
  ui::set_color(cancel_.word, theme::INK);
  ui::set_fill(shutdown_.block, tessera::Tone::ALERT, theme::ACCENT);
  ui::set_color(shutdown_.word, theme::ON_ACCENT);
}

void ConfirmCard::on_tap() {
  const tessera::Point p = tessera::tap_point();
  if (p.y < keys_y_ || p.y >= keys_y_ + key_h_) return;
  const auto hit = [&p](const Key &key) { return p.x >= key.x && p.x < key.x + key.width; };
  if (hit(cancel_)) {
    tessera::close_card();
    return;
  }
  if (!hit(shutdown_)) return;   // between the two
  const std::string control = roomy::plain(control_);
  const char *action = control_action(control_kind(control.c_str()), false);
  if (action == nullptr) return;
  tessera::action(action, control);
  tessera::close_card();
}

}  // namespace esphome::nas_power
