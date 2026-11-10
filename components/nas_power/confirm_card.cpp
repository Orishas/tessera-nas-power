#include "confirm_card.h"
#include <algorithm>
#include <string>

namespace esphome::nas_power {

using tessera::Font;
namespace ui = tessera::ui;

static lv_obj_t *button(lv_obj_t *parent, lv_obj_t **word) {
  lv_obj_t *b = ui::block(parent, theme::TRACK);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  *word = ui::label(b, Font::BODY_LARGE, theme::INK);
  lv_obj_set_style_text_align(*word, LV_TEXT_ALIGN_CENTER, 0);
  return b;
}

void ConfirmCard::open(const tessera::CardContext &c) {
  entity_ = roomy::text(std::string(c.entity ? c.entity : ""));
  width_ = c.width;
  height_ = c.height;
  gap_ = ui::px(12);
  question_ = ui::text(c.parent, Font::BODY_LARGE, theme::INK);
  lv_obj_set_width(question_, width_);
  lv_obj_set_style_text_align(question_, LV_TEXT_ALIGN_CENTER, 0);
  const int lh = ui::line_height(Font::BODY_LARGE);
  lv_obj_set_height(question_, 2 * lh);
  lv_obj_set_pos(question_, 0, std::max(0, height_ / 2 - 2 * lh - gap_));
  ui::set_text(question_, plugin_->text("shutdown_q"));
  key_w_ = (width_ - gap_) / 2;
  key_h_ = std::max(ui::mm(12), std::min(height_ / 3, ui::mm(20)));
  keys_y_ = height_ / 2 + gap_;
  cancel_ = button(c.parent, &cancel_word_);
  off_ = button(c.parent, &off_word_);
  lv_obj_t *words[2] = {cancel_word_, off_word_};
  lv_obj_t *blocks[2] = {cancel_, off_};
  for (int i = 0; i < 2; ++i) {
    lv_obj_set_pos(blocks[i], i ? key_w_ + gap_ : 0, keys_y_);
    lv_obj_set_size(blocks[i], i ? width_ - key_w_ - gap_ : key_w_, key_h_);
    lv_obj_set_width(words[i], i ? width_ - key_w_ - gap_ : key_w_);
    lv_obj_set_pos(words[i], 0, (key_h_ - lh) / 2);
  }
  ui::set_text(cancel_word_, plugin_->text("cancel"));
  ui::set_text(off_word_, plugin_->text("shutdown"));
  paint();
}

void ConfirmCard::paint() {
  if (!question_) return;
  ui::set_color(question_, theme::INK);
  ui::set_fill(cancel_, tessera::Tone::NORMAL, theme::TRACK);
  ui::set_color(cancel_word_, theme::INK);
  ui::set_fill(off_, tessera::Tone::ALERT, theme::ACCENT);
  ui::set_color(off_word_, theme::ON_ACCENT);
}

void ConfirmCard::on_tap() {
  const tessera::Point p = tessera::tap_point();
  if (p.x < 0 || p.y < keys_y_ || p.y >= keys_y_ + key_h_) return;
  if (p.x < key_w_) {
    tessera::close_card();
    return;
  }
  if (p.x < key_w_ + gap_) return;   // between the two
  const std::string e = roomy::plain(entity_);
  if (e.empty()) return;
  // A script shuts the device down when it runs while the device is on; a switch is turned off.
  const char *service = e.rfind("script.", 0) == 0 ? "script.turn_on"
                        : e.rfind("input_boolean.", 0) == 0 ? "input_boolean.turn_off" : "switch.turn_off";
  tessera::action(service, e);
  tessera::close_card();
}

}  // namespace esphome::nas_power
