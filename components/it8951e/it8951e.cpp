#include "it8951e.h"

#include "esphome/core/application.h"

namespace esphome {
namespace it8951e {

const char *const IT8951ESensor::TAG = "it8951e.display";

void IT8951ESensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up IT8951E...");

  if (this->reset_pin_ == nullptr || this->busy_pin_ == nullptr) {
    ESP_LOGE(TAG, "reset_pin and busy_pin must be set.");
    return;
  }

  this->reset_pin_->setup();
  this->reset_pin_->digital_write(true);

  this->busy_pin_->setup();

  this->spi_setup();

  this->reset_();

  // Allocate framebuffer (4bpp packed as 2 pixels per byte).
  this->init_internal_(this->buffer_length_());
  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate display buffer.");
    return;
  }

  // Clear buffer to white.
  memset(this->buffer_, 0x00, this->buffer_length_());
  this->max_x_ = 0;
  this->max_y_ = 0;

  ESP_LOGCONFIG(TAG, "IT8951E setup complete.");
}

void IT8951ESensor::dump_config() {
  LOG_DISPLAY("", "IT8951E", this);
  ESP_LOGCONFIG(TAG, "  Model: %s", (this->model_ == it8951eModel::M5EPD) ? "M5EPD" : "Unknown");
  ESP_LOGCONFIG(TAG, "  Reversed: %s", this->reversed_ ? "true" : "false");
  ESP_LOGCONFIG(TAG, "  Reset duration: %ums", (unsigned) this->reset_duration_ms_);
}

int IT8951ESensor::get_width_internal() { return this->devs_[this->model_].devInfo.usPanelW; }
int IT8951ESensor::get_height_internal() { return this->devs_[this->model_].devInfo.usPanelH; }

uint32_t IT8951ESensor::buffer_length_() const {
  // 4bpp packed: 2 pixels per byte
  return (uint32_t) this->get_width_internal() * (uint32_t) this->get_height_internal() / 2U;
}

void IT8951ESensor::reset_() {
  this->reset_pin_->digital_write(true);
  delay(10);
  this->reset_pin_->digital_write(false);
  delay(this->reset_duration_ms_);
  this->reset_pin_->digital_write(true);
  delay(100);
}

void IT8951ESensor::wait_busy_(uint32_t timeout_ms) {
  const uint32_t start = millis();
  // Many IT8951 boards expose HRDY/BUSY as "high = ready".
  while (!this->busy_pin_->digital_read()) {
    delay(1);
    if (millis() - start > timeout_ms) {
      ESP_LOGW(TAG, "Busy wait timeout.");
      break;
    }
  }
}

void IT8951ESensor::check_busy_(uint32_t timeout_ms) {
  // A small guard that still logs if not ready.
  const uint32_t start = millis();
  while (!this->busy_pin_->digital_read()) {
    delay(1);
    if (millis() - start > timeout_ms) {
      ESP_LOGW(TAG, "Busy check timeout.");
      break;
    }
  }
}

void IT8951ESensor::write_u16_(uint16_t value) {
  uint8_t buf[2];
  buf[0] = (uint8_t) (value >> 8);
  buf[1] = (uint8_t) (value & 0xFF);
  this->write_array(buf, sizeof(buf));
}

uint16_t IT8951ESensor::read_u16_() {
  uint8_t buf[2]{0, 0};
  this->read_array(buf, sizeof(buf));
  return (uint16_t(buf[0]) << 8) | uint16_t(buf[1]);
}

void IT8951ESensor::write_command_(uint16_t cmd) {
  // IT8951 SPI preamble for "command" is commonly 0x6000 in this component lineage.  [oai_citation:12‡juju.nz](https://juju.nz/src/michaelh/pipish/src/branch/main/components/it8951e/it8951e.cpp)
  this->wait_busy_(200);
  this->enable();
  this->write_u16_(0x6000);
  this->wait_busy_(200);
  this->write_u16_(cmd);
  this->disable();
}

void IT8951ESensor::write_word_(uint16_t data) {
  // IT8951 SPI preamble for "data" is commonly 0x0000.
  this->wait_busy_(200);
  this->enable();
  this->write_u16_(0x0000);
  this->wait_busy_(200);
  this->write_u16_(data);
  this->disable();
}

void IT8951ESensor::write_reg_(uint16_t addr, uint16_t data) {
  this->write_command_(IT8951_TCON_REG_WR);
  this->wait_busy_(200);
  this->enable();
  this->write_u16_(0x0000);
  this->wait_busy_(200);
  this->write_u16_(addr);
  this->wait_busy_(200);
  this->write_u16_(data);
  this->disable();
}

void IT8951ESensor::set_target_memory_addr_(uint16_t addr_l, uint16_t addr_h) {
  this->write_reg_(IT8951_LISAR + 2, addr_h);
  this->write_reg_(IT8951_LISAR, addr_l);
}

void IT8951ESensor::write_args_(uint16_t cmd, const uint16_t *args, uint16_t length) {
  this->write_command_(cmd);
  for (uint16_t i = 0; i < length; i++) {
    this->write_word_(args[i]);
  }
}

void IT8951ESensor::set_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint16_t args[5];
  args[0] = (uint16_t) ((this->endian_ << 8) | (this->pix_bpp_ << 4));
  args[1] = x;
  args[2] = y;
  args[3] = w;
  args[4] = h;
  this->write_args_(IT8951_TCON_LD_IMG_AREA, args, 5);
}

void IT8951ESensor::update_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, update_mode_e mode) {
  if (mode == update_mode_e::UPDATE_MODE_NONE) {
    return;
  }

  this->clamp_region_(x, y, w, h);

  uint16_t args[7];
  args[0] = x;
  args[1] = y;
  args[2] = w;
  args[3] = h;
  args[4] = (uint16_t) mode;
  args[5] = this->devs_[this->model_].devInfo.usImgBufAddrL;
  args[6] = this->devs_[this->model_].devInfo.usImgBufAddrH;

  this->write_args_(IT8951_I80_CMD_DPY_BUF_AREA, args, 7);
}

void IT8951ESensor::clamp_region_(uint16_t &x, uint16_t &y, uint16_t &w, uint16_t &h) const {
  const uint16_t W = (uint16_t) this->get_width_internal();
  const uint16_t H = (uint16_t) this->get_height_internal();

  // IT8951 partial constraints: width and x often want multiples of 4 pixels.
  x = (uint16_t) (x & 0xFFFC);
  w = (uint16_t) ((w + 3) & 0xFFFC);

  if (x >= W) x = 0;
  if (y >= H) y = 0;
  if (x + w > W) w = (uint16_t) (W - x);
  if (y + h > H) h = (uint16_t) (H - y);

  if (w == 0) w = 4;
  if (h == 0) h = 4;
}

void IT8951ESensor::write_buffer_to_display_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *gram) {
  // This sends the packed 4bpp buffer as 16-bit words (4 pixels per word).
  // Layout: 2 bytes = 4 pixels (each nibble is a pixel).
  this->endian_ = IT8951_LDIMG_B_ENDIAN;
  this->pix_bpp_ = IT8951_4BPP;

  this->set_target_memory_addr_(this->devs_[this->model_].devInfo.usImgBufAddrL,
                                this->devs_[this->model_].devInfo.usImgBufAddrH);

  this->set_area_(x, y, w, h);

  const uint32_t row_bytes = (uint32_t) w / 2U;  // 2 pixels per byte
  const uint32_t total_bytes = row_bytes * (uint32_t) h;

  this->wait_busy_(500);
  this->enable();
  // data preamble
  this->write_u16_(0x0000);

  // Stream as-is. The IT8951 expects words, but SPI writes bytes; controller bridges.
  // Keep chunks modest.
  const uint32_t CHUNK = 1024;
  for (uint32_t pos = 0; pos < total_bytes; pos += CHUNK) {
    const uint32_t n = std::min<uint32_t>(CHUNK, total_bytes - pos);
    this->write_array(gram + pos, n);
  }

  this->disable();
  this->write_command_(IT8951_TCON_LD_IMG_END);
}

void IT8951ESensor::clear(bool init_refresh) {
  // Clear local framebuffer to white (panel white depends on reversed_).
  // 0x00 is white in many 4bpp pipelines; reversed_ flips at draw time.
  memset(this->buffer_, 0x00, this->buffer_length_());
  this->max_x_ = (uint16_t) this->get_width_internal();
  this->max_y_ = (uint16_t) this->get_height_internal();

  // Also clear the panel memory and optionally INIT refresh.
  this->write_command_(IT8951_TCON_SYS_RUN);

  // Write a full-white buffer to the controller memory.
  uint16_t x = 0, y = 0, w = (uint16_t) this->get_width_internal(), h = (uint16_t) this->get_height_internal();
  this->write_buffer_to_display_(x, y, w, h, this->buffer_);

  if (init_refresh) {
    this->update_area_(0, 0, w, h, update_mode_e::UPDATE_MODE_INIT);
  }

  this->write_command_(IT8951_TCON_SLEEP);
}

void IT8951ESensor::update() {
  // Default component.update path uses GC16, matching the lineage baseline.  [oai_citation:13‡juju.nz](https://juju.nz/src/michaelh/pipish/src/branch/main/components/it8951e/it8951e.cpp)
  this->update_with_mode(update_mode_e::UPDATE_MODE_GC16, false);
}

void IT8951ESensor::update_with_mode(update_mode_e mode, bool full) {
  if (this->buffer_ == nullptr) return;

  // Render via display pipeline
  this->do_update_();

  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t w = full ? (uint16_t) this->get_width_internal() : this->max_x_;
  uint16_t h = full ? (uint16_t) this->get_height_internal() : this->max_y_;

  if (w == 0 || h == 0) {
    // Nothing marked dirty; still allow a minimal refresh.
    w = 4;
    h = 4;
  }

  this->write_command_(IT8951_TCON_SYS_RUN);
  this->write_buffer_to_display_(x, y, w, h, this->buffer_);
  this->update_area_(x, y, w, h, mode);
  this->write_command_(IT8951_TCON_SLEEP);

  this->max_x_ = 0;
  this->max_y_ = 0;
}

void IT8951ESensor::draw_absolute_pixel_internal(int x, int y, Color color) {
  const int W = this->get_width_internal();
  const int H = this->get_height_internal();
  if (x < 0 || y < 0 || x >= W || y >= H) return;
  if (this->buffer_ == nullptr) return;

  // Convert ESPHome Color to 4-bit grayscale.
  // Color in ESPHome is RGB; for monochrome dashboards we treat any non-black as white.
  // If you already draw using inverted constants in YAML, this remains consistent.
  uint8_t v = (color.red + color.green + color.blue) / 3;
  uint8_t nibble = (uint8_t) (v >> 4);  // 0..15
  if (this->reversed_) nibble = (uint8_t) (0x0F - nibble);

  const uint32_t byte_width = (uint32_t) W / 2U;
  const uint32_t index = (uint32_t) y * byte_width + (uint32_t) (x >> 1);

  if ((x & 1) == 0) {
    // high nibble
    this->buffer_[index] = (uint8_t) ((this->buffer_[index] & 0x0F) | (nibble << 4));
  } else {
    // low nibble
    this->buffer_[index] = (uint8_t) ((this->buffer_[index] & 0xF0) | (nibble & 0x0F));
  }

  // Dirty tracking from origin to max extent
  if ((uint16_t) (x + 1) > this->max_x_) this->max_x_ = (uint16_t) (x + 1);
  if ((uint16_t) (y + 1) > this->max_y_) this->max_y_ = (uint16_t) (y + 1);
}

}  // namespace it8951e
}  // namespace esphome

