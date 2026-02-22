#include "it8951e.h"
#include "esphome/core/log.h"

namespace esphome {
namespace it8951e {

static const char *const TAG = "it8951e";

void IT8951EDisplay::setup() {
  ESP_LOGI(TAG, "Setting up IT8951E display");

  if (cs_pin_ != nullptr) cs_pin_->setup();
  if (reset_pin_ != nullptr) reset_pin_->setup();
  if (busy_pin_ != nullptr) busy_pin_->setup();

  // TODO: initialize IT8951E controller here using your existing driver code
}

void IT8951EDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "IT8951E Display");
  LOG_PIN("  CS Pin: ", cs_pin_);
  LOG_PIN("  Reset Pin: ", reset_pin_);
  LOG_PIN("  Busy Pin: ", busy_pin_);
  ESP_LOGCONFIG(TAG, "  Rotation: %d", rotation_);
  ESP_LOGCONFIG(TAG, "  Reversed: %s", reversed_ ? "true" : "false");
}

void IT8951EDisplay::update() {
  // This renders the lambda into the framebuffer.
  this->do_update_();
}

void IT8951EDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  // TODO: draw into your backing buffer. This stub just ignores pixels.
  (void) x;
  (void) y;
  (void) color;
}

void IT8951EDisplay::clear_panel_() {
  ESP_LOGD(TAG, "Clear panel requested");
  // TODO: clear local buffer + trigger a clean refresh if your controller supports it.
}

void IT8951EDisplay::refresh_panel_(UpdateMode mode, bool full) {
  ESP_LOGD(TAG, "Refresh panel requested mode=%u full=%s", (unsigned) mode, full ? "true" : "false");

  // TODO: call the real IT8951E waveform update method here.
  // full=true should refresh the entire panel; full=false may allow partial if supported.

  // If NONE, do nothing by design.
  if (mode == NONE) return;
}

}  // namespace it8951e
}  // namespace esphome
