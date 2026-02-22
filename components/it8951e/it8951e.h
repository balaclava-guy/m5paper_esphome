#pragma once

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "it8951.h"

namespace esphome {
namespace it8951e {

enum it8951eModel {
  M5EPD = 0,
  it8951eModelsEND,
};

enum UpdateMode : uint8_t {
  UPDATE_MODE_INIT = 0,
  UPDATE_MODE_DU = 1,
  UPDATE_MODE_GC16 = 2,
  UPDATE_MODE_GL16 = 3,
  UPDATE_MODE_GLR16 = 4,
  UPDATE_MODE_GLD16 = 5,
  UPDATE_MODE_DU4 = 6,
  UPDATE_MODE_A2 = 7,
  UPDATE_MODE_NONE = 8,
};

class IT8951ESensor
    : public display::DisplayBuffer,
      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                            spi::DATA_RATE_20MHZ> {
 public:
  struct IT8951DevInfo {
    int panel_width;
    int panel_height;
    uint16_t image_buffer_addr_l;
    uint16_t image_buffer_addr_h;
    char fw_version[16];
    char lut_version[16];
  };

  struct IT8951Dev {
    IT8951DevInfo dev_info;
    display::DisplayType display_type;
  };

  float get_loop_priority() const override { return 0.0f; }
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  void set_reset_pin(GPIOPin *reset) { this->reset_pin_ = reset; }
  void set_busy_pin(GPIOPin *busy) { this->busy_pin_ = busy; }
  void set_reversed(bool reversed) { this->reversed_ = reversed; }
  void set_reset_duration(uint32_t reset_duration) { this->reset_duration_ = reset_duration; }
  void set_model(it8951eModel model);
  void set_sleep_when_done(bool sleep_when_done) { this->sleep_when_done_ = sleep_when_done; }
  void set_full_update_every(uint32_t full_update_every) { this->full_update_every_ = full_update_every; }

  void setup() override;
  void update() override;
  void update_slow();
  void dump_config() override;

  display::DisplayType get_display_type() override { return this->devices_[this->model_].display_type; }

  void clear(bool init);
  void fill(Color color) override;
  void draw_pixel_at(int x, int y, Color color) override;
  void write_display(UpdateMode mode);
  void write_display(UpdateMode mode, bool full);

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return this->panel_width_; }
  int get_height_internal() override { return this->panel_height_; }
  uint32_t get_buffer_length_() const;

 private:
  IT8951Dev devices_[it8951eModel::it8951eModelsEND]{{
      {
          960,
          540,
          0x36E0,
          0x0012,
          "",
          "",
      },
      display::DisplayType::DISPLAY_TYPE_GRAYSCALE,
  }};

  void write_display_internal_(UpdateMode mode, bool force_full, bool honor_auto_full_mode);
  void mark_full_dirty_();

  void get_device_info_(IT8951DevInfo *info);
  void reset_();
  void wait_busy_(uint32_t timeout = 2000);
  void check_busy_(uint32_t timeout = 6000);
  uint16_t get_vcom_();
  void set_vcom_(uint16_t vcom);

  uint16_t read_word_();
  void read_words_(void *buf, uint32_t words);

  void write_two_byte16_(uint16_t type, uint16_t cmd);
  void write_command_(uint16_t cmd);
  void write_word_(uint16_t cmd);
  void write_reg_(uint16_t addr, uint16_t data);
  void set_target_memory_addr_(uint16_t addr_l, uint16_t addr_h);
  void write_args_(uint16_t cmd, const uint16_t *args, uint16_t length);

  void set_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void update_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, UpdateMode mode);
  void write_buffer_to_display_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *gram);

  int max_x_{0};
  int max_y_{0};
  int min_x_{960};
  int min_y_{540};

  uint16_t endian_type_{IT8951_LDIMG_B_ENDIAN};
  uint16_t pixel_bpp_{IT8951_4BPP};

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  int panel_width_{960};
  int panel_height_{540};
  bool reversed_{false};
  uint32_t reset_duration_{100};
  bool sleep_when_done_{true};
  uint32_t full_update_every_{60};
  uint32_t partial_update_counter_{0};
  it8951eModel model_{it8951eModel::M5EPD};
};

template<typename... Ts> class ClearAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->clear(true); }
};

template<typename... Ts> class UpdateSlowAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->update_slow(); }
};

template<typename... Ts> class DrawAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->write_display(UPDATE_MODE_DU); }
};

template<typename... Ts> class UpdateAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void set_mode(UpdateMode mode) { this->mode_ = mode; }
  void set_full(bool full) { this->full_ = full; }

  void play(const Ts &... x) override { this->parent_->write_display(this->mode_, this->full_); }

 protected:
  UpdateMode mode_{UPDATE_MODE_DU};
  bool full_{false};
};

}  // namespace it8951e
}  // namespace esphome
