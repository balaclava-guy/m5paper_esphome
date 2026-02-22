#pragma once

#include "esphome/core/component.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace it8951e {

enum it8951eModel : uint8_t {
  M5EPD = 0,
};

// Keep these names stable because they appear in YAML.
enum it8951eUpdateMode : uint8_t {
  DU = 0,
  DU4 = 1,
  A2 = 2,
  GL16 = 3,
  GC16 = 4,
};

class IT8951ESensor : public PollingComponent,
                      public display::DisplayBuffer,
                      public display::Display,
                      public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                           spi::CLOCK_POLARITY_LOW,
                                           spi::CLOCK_PHASE_LEADING,
                                           spi::DATA_RATE_20MHZ> {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_model(it8951eModel model) { this->model_ = model; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_busy_pin(GPIOPin *pin) { this->busy_pin_ = pin; }
  void set_reversed(bool reversed) { this->reversed_ = reversed; }
  void set_reset_duration(uint32_t reset_duration_ms) { this->reset_duration_ms_ = reset_duration_ms; }
  void set_sleep_when_done(bool sleep_when_done) { this->sleep_when_done_ = sleep_when_done; }
  void set_full_update_every(uint32_t every) { this->full_update_every_ = every; }

  // New: selectable update mode (used by it8951e.update action)
  void set_mode(it8951eUpdateMode mode) { this->pending_mode_ = mode; }

  // Actions already used by your YAML
  void clear(bool init = false);
  void updateslow();
  void draw();

  // DisplayBuffer overrides
  int get_width_internal() override;
  int get_height_internal() override;
  void draw_absolute_pixel_internal(int x, int y, Color color) override;

 protected:
  // SPI helpers / IT8951 protocol helpers
  void hard_reset_();
  bool wait_while_busy_(uint32_t timeout_ms);

  void write_two_byte16(uint16_t type, uint16_t cmd);
  uint16_t read_word();
  void read_words(void *data, uint32_t len);
  void write_reg(uint16_t addr, uint16_t data);

  void write_buffer_to_display(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buffer);

  void update_display_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, it8951eUpdateMode mode);

  // State
  it8951eModel model_{M5EPD};

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};

  bool reversed_{false};
  uint32_t reset_duration_ms_{200};
  bool sleep_when_done_{true};
  uint32_t full_update_every_{60};
  uint32_t update_counter_{0};

  // New: mode requested by the parameterised update action
  it8951eUpdateMode pending_mode_{GC16};

  // Panel / controller info
  uint16_t panel_w_{960};
  uint16_t panel_h_{540};

  // Framebuffer pointers/etc are defined in your .cpp
};

}  // namespace it8951e
}  // namespace esphome
