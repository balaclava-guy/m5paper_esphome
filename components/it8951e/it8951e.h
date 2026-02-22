#pragma once

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include "it8951.h"

namespace esphome {
namespace it8951e {

enum it8951eModel {
  M5EPD = 0,
  it8951eModelsEND  // MUST be last
};

// Waveform update modes.  [oai_citation:10‡juju.nz](https://juju.nz/src/michaelh/pipish/src/branch/main/components/it8951e/it8951e.h)
enum update_mode_e {
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

struct IT8951DevInfo_s {
  uint16_t usPanelW;
  uint16_t usPanelH;
  uint16_t usImgBufAddrL;
  uint16_t usImgBufAddrH;
  char usFWVersion[16];
  char usLUTVersion[16];
};

struct IT8951Dev_s {
  IT8951DevInfo_s devInfo;
  display::DisplayType displayType;
};

class IT8951ESensor : public display::DisplayBuffer,
                      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                            spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_10MHZ> {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  void set_reset_pin(GPIOPin *pin) { reset_pin_ = pin; }
  void set_busy_pin(GPIOPin *pin) { busy_pin_ = pin; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
  void set_reset_duration(uint32_t ms) { reset_duration_ms_ = ms; }
  void set_model(it8951eModel model) { model_ = model; }

  // Clear IT8951 internal memory buffer and optionally INIT refresh.
  void clear(bool init_refresh);

  // Explicit refresh with requested update mode.
  // If full is true, refresh entire panel.
  void update_with_mode(update_mode_e mode, bool full);

  int get_width_internal() override;
  int get_height_internal() override;

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;

 private:
  static const char *const TAG;

  IT8951Dev_s devs_[it8951eModel::it8951eModelsEND] = {
      // M5EPD (M5Paper)
      {
          // devInfo
          {
              960,    // W
              540,    // H
              0x36E0, // ImgBufAddrL (common M5Paper value)
              0x0012, // ImgBufAddrH
              "",     // FW
              "",     // LUT
          },
          // displayType
          display::DisplayType::DISPLAY_TYPE_GRAYSCALE,
      },
  };

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  bool reversed_{false};
  uint32_t reset_duration_ms_{100};
  it8951eModel model_{it8951eModel::M5EPD};

  // Dirty tracking (simple bounding box from origin).
  // This matches the common approach in this component lineage.  [oai_citation:11‡juju.nz](https://juju.nz/src/michaelh/pipish/src/branch/main/components/it8951e/it8951e.cpp)
  uint16_t max_x_{0};
  uint16_t max_y_{0};

  // IT8951 load image state
  uint16_t endian_{IT8951_LDIMG_B_ENDIAN};
  uint16_t pix_bpp_{IT8951_4BPP};

  void reset_();
  void wait_busy_(uint32_t timeout_ms);
  void check_busy_(uint32_t timeout_ms);

  void write_u16_(uint16_t value);
  uint16_t read_u16_();

  void write_command_(uint16_t cmd);
  void write_word_(uint16_t data);
  void write_reg_(uint16_t addr, uint16_t data);
  void set_target_memory_addr_(uint16_t addr_l, uint16_t addr_h);

  void write_args_(uint16_t cmd, const uint16_t *args, uint16_t length);
  void set_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void update_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, update_mode_e mode);

  void write_buffer_to_display_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *gram);

  uint32_t buffer_length_() const;
  void clamp_region_(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h) const;
};

template<typename... Ts> class ClearAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void play(Ts... x) override { this->parent_->clear(true); }
};

template<typename... Ts> class UpdateAction : public Action<Ts...>, public Parented<IT8951ESensor> {
 public:
  void set_mode(update_mode_e mode) { this->mode_ = mode; }
  void set_full(bool full) { this->full_ = full; }

  void play(Ts... x) override { this->parent_->update_with_mode(this->mode_, this->full_); }

 private:
  update_mode_e mode_{update_mode_e::UPDATE_MODE_GC16};
  bool full_{false};
};

}  // namespace it8951e
}  // namespace esphome

