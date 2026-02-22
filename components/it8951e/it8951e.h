#pragma once

#include "esphome/core/component.h"
#include "esphome/core/version.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace it8951e {

enum it8951eModel {
  M5EPD = 0,
  it8951eModelsEND  // MUST be last
};

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2023, 12, 0)
class IT8951ESensor : public display::DisplayBuffer,
#else
class IT8951ESensor : public PollingComponent, public display::DisplayBuffer,
#endif  // VERSION_CODE(2023, 12, 0)
                      public spi::SPIDevice {
 public:
  float get_loop_priority() const override { return 0.0f; }
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  /* ---------------------------------------- Refresh mode description ----------------------------------------
   INIT
   The initialization (INIT) mode is used to completely erase the display and leave it in the white state.
   ...
   (kept as-is)
  */

  struct IT8951DevInfo_s {
    int usPanelW;
    int usPanelH;
    uint16_t usImgBufAddrL;
    uint16_t usImgBufAddrH;
    char usFWVersion[16];
    char usLUTVersion[16];
  };

  struct IT8951Dev_s {
    struct IT8951DevInfo_s devInfo;
    display::DisplayType displayType;
  };

  enum update_mode_e {
    UPDATE_MODE_INIT = 0,
    UPDATE_MODE_DU = 1,
    UPDATE_MODE_GC16 = 2,
    UPDATE_MODE_GL16 = 3,
    UPDATE_MODE_GLR16 = 4,
    UPDATE_MODE_GLD16 = 5,
    UPDATE_MODE_DU4 = 6,
    UPDATE_MODE_A2 = 7,
    UPDATE_MODE_NONE = 8
  };

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
  display::DisplayType get_display_type() override { return IT8951DevAll[this->model_].displayType; }
  void clear(bool init);
  void fill(Color color) override;
  void draw_pixel_at(int x, int y, Color color) override;
  void write_display(update_mode_e mode);

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return usPanelW_; }
  int get_height_internal() override { return usPanelH_; }
  uint32_t get_buffer_length_();

 private:
  struct IT8951Dev_s IT8951DevAll[it8951eModel::it8951eModelsEND]{
      // it8951eModel::M5EPD
      960,  // .devInfo.usPanelW
      540,  // .devInfo.usPanelH
      0x36E0,  // .devInfo.usImgBufAddrL
      0x0012,  // .devInfo.usImgBufAddrH
      "",  // .devInfo.usFWVersion
      "",  // .devInfo.usLUTVersion
      display::DisplayType::DISPLAY_TYPE_GRAYSCALE  // .displayType
  };

  void get_device_info(struct IT8951DevInfo_s *info);

  int max_x = 0;
  int max_y = 0;
  int min_x = 960;
  int min_y = 540;

  uint16_t m_endian_type = 0;
  uint16_t m_pix_bpp = 0;
  uint8_t _it8951_rotation = 0;

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  int usPanelW_{0};
  int usPanelH_{0};

  bool reversed_{false};
  uint32_t reset_duration_{100};

  bool sleep_when_done_{true};
  uint32_t full_update_every_{60};
  uint32_t partial_update_{0};

  enum it8951eModel model_{it8951eModel::M5EPD};

  void reset(void);

  void wait_busy(uint32_t timeout = 1000);
  void check_busy(uint32_t timeout = 1000);

  uint16_t get_vcom();
  void set_vcom(uint16_t vcom);

  uint16_t read_word();
  void read_words(void *buf, uint32_t length);
  void write_two_byte16(uint16_t type, uint16_t cmd);
  void write_command(uint16_t cmd);
  void write_word(uint16_t cmd);
  void write_reg(uint16_t addr, uint16_t data);
  void set_target_memory_addr(uint16_t tar_addrL, uint16_t tar_addrH);
  void write_args(uint16_t cmd, uint16_t *args, uint16_t length);
  void set_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void update_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h, update_mode_e mode);
  void write_buffer_to_display(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *gram);
};

template <typename... Ts> class ClearAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->clear(true); }
};

template <typename... Ts> class UpdateSlowAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->update_slow(); }
};

template <typename... Ts> class DrawAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->write_display(IT8951ESensor::UPDATE_MODE_DU); }
};

// New: DU4
template <typename... Ts> class UpdateFastAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->write_display(IT8951ESensor::UPDATE_MODE_DU4); }
};

// New: A2
template <typename... Ts> class UpdateA2Action : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->write_display(IT8951ESensor::UPDATE_MODE_A2); }
};

// New: GL16
template <typename... Ts> class UpdateGL16Action : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(const Ts &... x) override { this->parent_->write_display(IT8951ESensor::UPDATE_MODE_GL16); }
};

}  // namespace it8951e
}  // namespace esphome
