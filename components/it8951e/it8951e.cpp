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

  if (reset_pin_ != nullptr) {
    ESP_LOGD(TAG, "Resetting panel for %u ms", (unsigned) reset_duration_ms_);
    reset_pin_->digital_write(false);
    delay(reset_duration_ms_);
    reset_pin_->digital_write(true);
    delay(10);
  }

  // TODO: integrate your real IT8951E init here
}

void IT8951EDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "IT8951E Display");
  LOG_PIN("  CS Pin: ", cs_pin_);
  LOG_PIN("  Reset Pin: ", reset_pin_);
  LOG_PIN("  Busy Pin: ", busy_pin_);
  ESP_LOGCONFIG(TAG, "  Reset duration: %u ms", (unsigned) reset_duration_ms_);
  ESP_LOGCONFIG(TAG, "  Rotation: %d", rotation_);
  ESP_LOGCONFIG(TAG, "  Reversed: %s", reversed_ ? "true" : "false");
}

void IT8951EDisplay::update() {
  this->do_update_();
}

void IT8951EDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  (void) x;
  (void) y;
  (void) color;
}

void IT8951EDisplay::clear_panel_() {
  ESP_LOGD(TAG, "Clear panel requested");
}

void IT8951EDisplay::refresh_panel_(UpdateMode mode, bool full) {
  ESP_LOGD(TAG, "Refresh panel requested mode=%u full=%s", (unsigned) mode, full ? "true" : "false");
  if (mode == NONE) return;
  (void) full;
}

}  // namespace it8951e
}  // namespace esphome

