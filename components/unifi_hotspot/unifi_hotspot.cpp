#include "unifi_hotspot.h"
#include <algorithm>
#include <string>

namespace esphome::unifi_hotspot {

using tessera::Font;
namespace ui = tessera::ui;

static const char *const PRESS = "button.press";
// How long a key shows that its press was sent, in seconds.
static const uint32_t SENT_SECONDS = 3;
// Icons of Tessera's set for keys too narrow for their words: refresh-auto and plus.
static const uint32_t ICON_REFRESH = 0xF18F2;
static const uint32_t ICON_PLUS = 0xF0415;

enum Key : uint8_t { REFRESH = 0, CREATE = 1, KEYS = 2 };

// The tile: the QR code of the guest network on the left (on top when the tile is narrow), beside it the network's
// name, the voucher code large, how long the voucher is valid, and two keys that press the integration's buttons.
// The QR code is the tile's own image entity, shown by name (camera:<entity>, plugin API 0.17). On a board without
// pictures (the CYD) the words and keys take the whole card. On a card too small for words and keys, or with the option
// QR code only, the QR code fills the card alone.
class HotspotTile : public tessera::Tile {
 public:
  explicit HotspotTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    entity_ = c.entity;
    buttons_[REFRESH] = c.options["refresh"] | "";
    buttons_[CREATE] = c.options["create"] | "";
    show_wlan_ = c.options["show_wlan"] | true;
    show_duration_ = c.options["show_duration"] | true;
    const bool qr_only = c.options["qr_only"] | false;

    const int w = c.width, h = c.height;
    const int gap = ui::px(12), small = ui::px(4);
    const int bh = ui::line_height(Font::BODY);
    bool keys = !buttons_[REFRESH].empty() || !buttons_[CREATE].empty();
    key_h_ = std::max(ui::mm(8), ui::line_height(Font::BODY_LARGE) + ui::px(10));
    const int min_qr = ui::mm(15);

    // The words and keys as one block, its code in the largest font that fits.
    code_font_ = Font::VALUE;
    auto block = [&]() {
      return (show_wlan_ ? bh + small : 0) + ui::line_height(code_font_) + (show_duration_ ? small + bh : 0) +
             (keys ? gap + key_h_ : 0);
    };
    // What does not fit goes, the least important first; false when not even the code and the keys fit.
    auto shrink = [&](int room) {
      if (block() > room) code_font_ = Font::HEADLINE;
      if (block() > room) show_duration_ = false;
      if (block() > room) show_wlan_ = false;
      if (block() > room) code_font_ = Font::TITLE;
      return block() <= room;
    };

    // The QR code alone, as large as the card.
    const bool pictures = ui::pictures();
    auto qr_alone = [&]() {
      const int side = std::min(w, h);
      place_ = ui::picture(c.parent);
      lv_obj_set_size(place_, side, side);
      lv_obj_set_pos(place_, (w - side) / 2, (h - side) / 2);
    };
    if (pictures && qr_only) return qr_alone();

    // Where the QR code goes: beside the words on a wide card, above them on a tall one, nowhere when it would be
    // smaller than a phone can read.
    int x0 = 0, cw = w, top = 0, room = h;
    bool centred = true;
    if (pictures) {
      if (w >= h * 6 / 5) {
        // Beside: as high as the card, but leave the words at least 55 % of the width.
        const int side = std::min(h, w * 45 / 100);
        if (side >= min_qr) {
          place_ = ui::picture(c.parent);
          lv_obj_set_size(place_, side, side);
          lv_obj_set_pos(place_, 0, (h - side) / 2);
          x0 = side + gap;
          cw = w - x0;
          centred = false;
        }
      } else {
        // Above: what the code and the keys leave of the height.
        const Font keep = code_font_;
        show_wlan_ = show_duration_ = false;
        code_font_ = Font::HEADLINE;
        const int under = block() + gap;
        code_font_ = keep;
        show_wlan_ = c.options["show_wlan"] | true;
        show_duration_ = c.options["show_duration"] | true;
        // The QR code is never wider than the card.
        const int side = std::min(w, h - under);
        // A small square card (1x1) has no room for both: the QR code alone, which is what a guest needs.
        if (side < min_qr && !entity_.empty()) return qr_alone();
        if (side >= min_qr) {
          place_ = ui::picture(c.parent);
          lv_obj_set_size(place_, side, side);
          lv_obj_set_pos(place_, (w - side) / 2, 0);
          top = side + gap;
          room = h - top;
        }
      }
    }
    if (!shrink(room)) {
      keys = false;   // without room for the keys, the code alone
      shrink(room);
    }

    width_ = cw;
    const lv_text_align_t align = centred ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT;
    int y = top + std::max(0, (room - block()) / 2);
    auto line = [&](Font font, theme::Role role, int height) {
      lv_obj_t *l = ui::label(c.parent, font, role);
      lv_obj_set_width(l, cw);
      lv_obj_set_pos(l, x0, y);
      lv_obj_set_style_text_align(l, align, 0);
      y += height;
      return l;
    };
    if (show_wlan_) wlan_ = line(Font::BODY, theme::MUTED, bh + small);
    code_ = line(code_font_, theme::INK, ui::line_height(code_font_));
    if (show_duration_) {
      y += small;
      duration_ = line(Font::BODY, theme::MUTED, bh);
    }

    // The keys: side by side across the column, each half of it, or the whole of it alone; never wider than a key
    // needs on a large card.
    if (keys) {
      y += gap;
      key_y_ = y;
      const bool both = !buttons_[REFRESH].empty() && !buttons_[CREATE].empty();
      const int most = ui::mm(45);
      const int kw = std::min(both ? (cw - gap) / 2 : cw, most);
      const int row = both ? 2 * kw + gap : kw;
      int x = centred ? x0 + (cw - row) / 2 : x0;
      for (int k = 0; k < KEYS; ++k) {
        if (buttons_[k].empty()) continue;
        key_[k] = ui::block(c.parent, k == CREATE ? theme::ACCENT : theme::RAISED);
        lv_obj_remove_flag(key_[k], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(key_[k], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(key_[k], 0, 0);
        lv_obj_set_style_radius(key_[k], key_h_ / 2, 0);
        lv_obj_set_size(key_[k], kw, key_h_);
        lv_obj_set_pos(key_[k], x, key_y_);
        key_label_[k] = ui::label(key_[k], Font::BODY_LARGE, k == CREATE ? theme::ON_ACCENT : theme::INK);
        lv_obj_set_width(key_label_[k], kw - ui::px(8));
        lv_obj_set_style_text_align(key_label_[k], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(key_label_[k]);
        key_x_[k] = x;
        key_w_[k] = kw;
        x += kw + gap;
      }
    }
    paint();
  }

  void on_state(JsonObjectConst data) override {
    wlan_text_ = tessera::Text(data["attributes"]["wlan_name"] | "");
    JsonObjectConst voucher = data["entities"]["voucher"];
    has_voucher_ = !voucher.isNull();
    const tessera::Text code(voucher["state"] | "");
    // A new code: the key that asked for it has done its work.
    if (has_voucher_ && !(code == code_text_)) sent_[CREATE] = sent_[REFRESH] = 0;
    code_text_ = code;
    duration_text_ = tessera::Text(voucher["attributes"]["duration"] | "");
    // The voucher sensor knows the network's name too, when the QR code is not chosen.
    if (wlan_text_.empty()) wlan_text_ = tessera::Text(voucher["attributes"]["wlan_name"] | "");
    if (place_) {
      const std::string entity = roomy::plain(entity_);
      ui::set_picture(place_, entity.empty() ? std::string() : "camera:" + entity, tessera::Fit::CONTAIN);
    }
    paint();
  }

  void on_tick(uint32_t epoch) override {
    bool changed = false;
    for (int k = 0; k < KEYS; ++k) {
      if (sent_[k] != 0 && epoch >= sent_[k] + SENT_SECONDS) {
        sent_[k] = 0;
        changed = true;
      }
    }
    if (changed) paint();
  }

  void on_tap() override {
    if (!code_) return;
    const tessera::Point p = tessera::tap_point();
    if (p.y < key_y_ - ui::px(8) || p.y > key_y_ + key_h_ + ui::px(8)) return;
    for (int k = 0; k < KEYS; ++k) {
      if (!key_[k] || p.x < key_x_[k] - ui::px(6) || p.x > key_x_[k] + key_w_[k] + ui::px(6)) continue;
      const bool ok = tessera::action(PRESS, roomy::plain(buttons_[k]));
      sent_[k] = ok ? std::max<uint32_t>(tessera::epoch(), 1) : 0;
      failed_[k] = !ok;
      paint();
      return;
    }
  }

  void on_theme() override {
    if (wlan_) ui::set_color(wlan_, theme::MUTED);
    if (duration_) ui::set_color(duration_, theme::MUTED);
    paint();
  }

 private:
  void paint() {
    if (!code_) return;   // the QR code alone
    // The code: as large as the block allows and the column's width takes.
    std::string code = roomy::plain(code_text_);
    const bool none = !has_voucher_ || code.empty() || code == "unavailable" || code == "unknown";
    if (!has_voucher_) code = plugin_->text("choose_voucher");
    else if (none) code = plugin_->text("no_voucher");
    Font font = none ? Font::BODY_LARGE : code_font_;
    if (!none) {
      static const Font STEPS[] = {Font::VALUE, Font::HEADLINE, Font::TITLE, Font::BODY_LARGE};
      bool from = false;
      for (Font f : STEPS) {
        if (f == code_font_) from = true;
        if (!from) continue;
        font = f;
        if (ui::covers(code, f) && ui::text_width(code, f) <= width_) break;
      }
    }
    ui::set_font(code_, font);
    ui::set_color(code_, none ? theme::MUTED : theme::INK);
    ui::set_text(code_, code);

    if (wlan_) ui::set_text(wlan_, roomy::plain(wlan_text_));
    if (duration_) {
      const std::string d = roomy::plain(duration_text_);
      ui::set_text(duration_, d.empty() || none ? std::string() : tessera::fill(plugin_->text("valid_for"), "d", d));
    }

    for (int k = 0; k < KEYS; ++k) {
      if (!key_[k]) continue;
      const theme::Role ground = k == CREATE ? theme::ACCENT : theme::RAISED;
      const theme::Role ink = k == CREATE ? theme::ON_ACCENT : theme::INK;
      ui::set_fill(key_[k], tessera::Tone::NORMAL, ground);
      const char *word = failed_[k] ? "failed" : sent_[k] ? "sent" : k == CREATE ? "create" : "refresh";
      const std::string text = plugin_->text(word);
      // A narrow key shows its icon instead of its word; a sent press is then green, a failed one red.
      if (ui::text_width(text, Font::BODY_LARGE) > key_w_[k] - ui::px(8)) {
        ui::set_font(key_label_[k], Font::ICON_SMALL);
        ui::set_text(key_label_[k], ui::icon(k == CREATE ? ICON_PLUS : ICON_REFRESH));
        ui::set_tone(key_label_[k], failed_[k] ? tessera::Tone::ALERT : sent_[k] ? tessera::Tone::GOOD
                                                                                : tessera::Tone::NORMAL, ink);
      } else {
        ui::set_font(key_label_[k], Font::BODY_LARGE);
        ui::set_text(key_label_[k], text);
        ui::set_color(key_label_[k], ink);
      }
    }
  }

  const tessera::Plugin *plugin_;
  tessera::Text entity_, buttons_[KEYS], wlan_text_, code_text_, duration_text_;   // PSRAM first
  bool show_wlan_ = true, show_duration_ = true, has_voucher_ = false;
  bool failed_[KEYS] = {false, false};
  uint32_t sent_[KEYS] = {0, 0};
  Font code_font_ = Font::VALUE;
  int width_ = 0, key_h_ = 0, key_y_ = -1000, key_x_[KEYS] = {0, 0}, key_w_[KEYS] = {0, 0};
  lv_obj_t *place_{}, *wlan_{}, *code_{}, *duration_{}, *key_[KEYS] = {nullptr, nullptr},
           *key_label_[KEYS] = {nullptr, nullptr};
};

void UnifiHotspot::setup() {
  add_tile("hotspot", [this]() { return new HotspotTile(this); });
}

}  // namespace esphome::unifi_hotspot
