#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace it8951e {

enum UpdateMode : uint8_t {
  DU = 0,
  DU4 = 1,
  A2 = 2,
  GL16 = 3,
  GC16 = 4,
  INIT = 5,
  NONE = 6,
};

class IT8951EDisplay : public display::DisplayBuffer, public PollingComponent {
 public:
  void set_spi(spi::SPIComponent *spi) { spi_ = spi; }
  void set_cs_pin(GPIOPin *pin) { cs_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { reset_pin_ = pin; }
  void set_busy_pin(GPIOPin *pin) { busy_pin_ = pin; }
  void set_rotation(int rotation) { rotation_ = rotation; }
  void set_reversed(bool reversed) { reversed_ = reversed; }

  void setup() override;
  void update() override;
  void dump_config() override;

  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  void clear_panel_();
  void refresh_panel_(UpdateMode mode, bool full);

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_height_internal() override { return 540; }
  int get_width_internal() override { return 960; }

  spi::SPIComponent *spi_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  int rotation_{0};
  bool reversed_{false};
};

class IT8951EUpdateAction : public Action<TEMPLATABLE_VALUE(UpdateMode), TEMPLATABLE_VALUE(bool)> {
 public:
  explicit IT8951EUpdateAction(IT8951EDisplay *parent) : parent_(parent) {}

  void set_mode(UpdateMode mode) { this->mode_ = mode; }
  void set_full(bool full) { this->full_ = full; }

  void play(TEMPLATABLE_VALUE(UpdateMode) mode, TEMPLATABLE_VALUE(bool) full) override {}

  void play() override {
    if (parent_ == nullptr) return;
    parent_->refresh_panel_(this->mode_, this->full_);
  }

 protected:
  IT8951EDisplay *parent_{nullptr};
  UpdateMode mode_{GC16};
  bool full_{true};
};

class IT8951EClearAction : public Action<> {
 public:
  explicit IT8951EClearAction(IT8951EDisplay *parent) : parent_(parent) {}

  void play() override {
    if (parent_ == nullptr) return;
    parent_->clear_panel_();
  }

 protected:
  IT8951EDisplay *parent_{nullptr};
};

}  // namespace it8951e
}  // namespace esphome

