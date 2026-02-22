#include "it8951e.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "esphome/core/gpio.h"
#include "esphome/core/log.h"

namespace esphome {
namespace it8951e {

static const char *const TAG = "it8951e.display";
static constexpr size_t SPI_STREAM_CHUNK = 512;
static constexpr uint32_t SPI_YIELD_BYTES = 4096;

void IT8951ESensor::write_two_byte16_(uint16_t type, uint16_t cmd) {
  this->wait_busy_();
  this->enable();

  this->write_byte16(type);
  this->wait_busy_();
  this->write_byte16(cmd);

  this->disable();
}

uint16_t IT8951ESensor::read_word_() {
  this->wait_busy_();
  this->enable();
  this->write_byte16(0x1000);
  this->wait_busy_();

  // Dummy read.
  this->write_byte16(0x0000);
  this->wait_busy_();

  uint8_t recv[2];
  this->read_array(recv, sizeof(recv));
  const uint16_t word = encode_uint16(recv[0], recv[1]);

  this->disable();
  return word;
}

void IT8951ESensor::read_words_(void *buf, uint32_t words) {
  ExternalRAMAllocator<uint16_t> allocator(ExternalRAMAllocator<uint16_t>::ALLOW_FAILURE);
  uint16_t *buffer = allocator.allocate(words);
  if (buffer == nullptr) {
    ESP_LOGE(TAG, "Read failed: could not allocate %u words", static_cast<unsigned int>(words));
    return;
  }

  this->wait_busy_();
  this->enable();
  this->write_byte16(0x1000);
  this->wait_busy_();

  // Dummy read.
  this->write_byte16(0x0000);
  this->wait_busy_();

  std::array<uint8_t, 64> recv_chunk{};
  uint32_t offset = 0;
  while (offset < words) {
    const uint32_t chunk_words = std::min<uint32_t>(32, words - offset);
    this->read_array(recv_chunk.data(), chunk_words * 2);
    for (uint32_t i = 0; i < chunk_words; i++) {
      buffer[offset + i] = encode_uint16(recv_chunk[i * 2], recv_chunk[i * 2 + 1]);
    }
    offset += chunk_words;
    if ((offset & 0x7F) == 0) {
      delay(1);
    }
  }

  this->disable();

  memcpy(buf, buffer, words * sizeof(uint16_t));
  allocator.deallocate(buffer, words);
}

void IT8951ESensor::write_command_(uint16_t cmd) { this->write_two_byte16_(0x6000, cmd); }

void IT8951ESensor::write_word_(uint16_t cmd) { this->write_two_byte16_(0x0000, cmd); }

void IT8951ESensor::write_reg_(uint16_t addr, uint16_t data) {
  this->write_command_(IT8951_TCON_REG_WR);
  this->wait_busy_();
  this->enable();
  this->write_byte16(0x0000);
  this->wait_busy_();
  this->write_byte16(addr);
  this->wait_busy_();
  this->write_byte16(data);
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
  if (x == 0 && y == 0 && w == this->get_width_internal() && h == this->get_height_internal()) {
    // Full-screen load.
    const uint16_t args[1] = {static_cast<uint16_t>((this->endian_type_ << 8) | (this->pixel_bpp_ << 4))};
    this->write_args_(IT8951_TCON_LD_IMG, args, 1);
  } else {
    // Area load.
    const uint16_t args[5] = {
        static_cast<uint16_t>((this->endian_type_ << 8) | (this->pixel_bpp_ << 4)),
        x,
        y,
        w,
        h,
    };
    this->write_args_(IT8951_TCON_LD_IMG_AREA, args, 5);
  }
}

void IT8951ESensor::wait_busy_(uint32_t timeout) {
  const uint32_t start_time = millis();
  while (true) {
    if (this->busy_pin_->digital_read()) {
      break;
    }
    if (millis() - start_time > timeout) {
      ESP_LOGE(TAG, "BUSY pin timeout");
      break;
    }
    delay(1);
  }
}

void IT8951ESensor::check_busy_(uint32_t timeout) {
  const uint32_t start_time = millis();
  while (true) {
    this->write_command_(IT8951_TCON_REG_RD);
    this->write_word_(IT8951_LUTAFSR);
    const uint16_t status = this->read_word_();
    if (status == 0) {
      break;
    }
    if (millis() - start_time > timeout) {
      ESP_LOGE(TAG, "LUT busy timeout: %u", static_cast<unsigned int>(status));
      break;
    }
    delay(1);
  }
}

void IT8951ESensor::update_area_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, UpdateMode mode) {
  if (mode == UPDATE_MODE_NONE) {
    return;
  }

  this->check_busy_();

  const uint16_t args[7] = {
      x,
      y,
      w,
      h,
      static_cast<uint16_t>(mode),
      this->devices_[this->model_].dev_info.image_buffer_addr_l,
      this->devices_[this->model_].dev_info.image_buffer_addr_h,
  };
  this->write_args_(IT8951_I80_CMD_DPY_BUF_AREA, args, 7);
}

void IT8951ESensor::reset_() {
  this->reset_pin_->digital_write(true);
  this->reset_pin_->digital_write(false);
  delay(this->reset_duration_);
  this->reset_pin_->digital_write(true);
  delay(100);
}

uint32_t IT8951ESensor::get_buffer_length_() const {
  // 4bpp packed: 2 pixels per byte.
  return static_cast<uint32_t>(this->panel_width_) * static_cast<uint32_t>(this->panel_height_) / 2;
}

void IT8951ESensor::get_device_info_(IT8951DevInfo *info) {
  this->write_command_(IT8951_I80_CMD_GET_DEV_INFO);
  this->read_words_(info, sizeof(IT8951DevInfo) / 2);
}

uint16_t IT8951ESensor::get_vcom_() {
  this->write_command_(IT8951_I80_CMD_VCOM);
  this->write_word_(0x0000);
  const uint16_t vcom = this->read_word_();
  ESP_LOGI(TAG, "VCOM = %.02fV", static_cast<float>(vcom) / 1000.0f);
  return vcom;
}

void IT8951ESensor::set_vcom_(uint16_t vcom) {
  this->write_command_(IT8951_I80_CMD_VCOM);
  this->write_word_(0x0001);
  this->write_word_(vcom);
}

void IT8951ESensor::mark_full_dirty_() {
  this->min_x_ = 0;
  this->min_y_ = 0;
  this->max_x_ = this->get_width_internal() - 1;
  this->max_y_ = this->get_height_internal() - 1;
}

void IT8951ESensor::setup() {
  ESP_LOGCONFIG(TAG, "Init starting");

  this->set_model(this->model_);
  this->spi_setup();

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->pin_mode(gpio::FLAG_OUTPUT);
    this->reset_();
  }
  this->busy_pin_->pin_mode(gpio::FLAG_INPUT);

  this->write_command_(IT8951_TCON_SYS_RUN);
  this->write_reg_(IT8951_I80CPCR, 0x0001);

  // M5Paper default target.
  const uint16_t vcom = this->get_vcom_();
  if (vcom != 2300) {
    this->set_vcom_(2300);
    this->get_vcom_();
  }

  this->init_internal_(this->get_buffer_length_());
  if (this->buffer_ != nullptr) {
    memset(this->buffer_, 0xFF, this->get_buffer_length_());
  }
  this->mark_full_dirty_();

  if (this->sleep_when_done_) {
    this->write_command_(IT8951_TCON_SLEEP);
  }

  ESP_LOGCONFIG(TAG, "Init done");
}

void IT8951ESensor::write_buffer_to_display_(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *gram) {
  this->endian_type_ = IT8951_LDIMG_B_ENDIAN;
  this->pixel_bpp_ = IT8951_4BPP;

  this->set_target_memory_addr_(this->devices_[this->model_].dev_info.image_buffer_addr_l,
                                this->devices_[this->model_].dev_info.image_buffer_addr_h);
  this->set_area_(x, y, w, h);

  const uint16_t byte_width = this->panel_width_ >> 1;

  this->enable();
  this->write_byte16(0x0000);

  std::array<uint8_t, SPI_STREAM_CHUNK> row_chunk{};
  const uint32_t row_bytes = w >> 1;
  const uint32_t x_byte_offset = x >> 1;
  uint32_t transferred_bytes = 0;

  for (uint32_t row = 0; row < h; row++) {
    const uint8_t *row_ptr = gram + (static_cast<uint32_t>(y + row) * byte_width) + x_byte_offset;
    uint32_t remaining = row_bytes;

    while (remaining > 0) {
      const size_t chunk = std::min<size_t>(remaining, row_chunk.size());
      if (this->reversed_) {
        this->write_array(row_ptr, chunk);
      } else {
        for (size_t i = 0; i < chunk; i++) {
          row_chunk[i] = static_cast<uint8_t>(0xFF - row_ptr[i]);
        }
        this->write_array(row_chunk.data(), chunk);
      }

      row_ptr += chunk;
      remaining -= static_cast<uint32_t>(chunk);
      transferred_bytes += static_cast<uint32_t>(chunk);
      if ((transferred_bytes % SPI_YIELD_BYTES) == 0) {
        delay(1);
      }
    }
  }

  this->disable();
  this->write_command_(IT8951_TCON_LD_IMG_END);
}

void IT8951ESensor::write_display_internal_(UpdateMode mode, bool force_full, bool honor_auto_full_mode) {
  if (mode == UPDATE_MODE_NONE || this->buffer_ == nullptr) {
    return;
  }

  if (this->sleep_when_done_) {
    this->write_command_(IT8951_TCON_SYS_RUN);
  }

  if (honor_auto_full_mode && !force_full && this->full_update_every_ > 0) {
    this->partial_update_counter_++;
    if (this->partial_update_counter_ >= this->full_update_every_) {
      this->partial_update_counter_ = 0;
      force_full = true;
      mode = UPDATE_MODE_GC16;
    }
  }

  int x1 = this->min_x_;
  int y1 = this->min_y_;
  int x2 = this->max_x_;
  int y2 = this->max_y_;

  if (force_full) {
    x1 = 0;
    y1 = 0;
    x2 = this->get_width_internal() - 1;
    y2 = this->get_height_internal() - 1;
  } else {
    if (x1 > x2 || y1 > y2) {
      if (this->sleep_when_done_) {
        this->write_command_(IT8951_TCON_SLEEP);
      }
      return;
    }
    x1 &= ~0x3;
    y1 &= ~0x3;
    x2 = std::min(this->get_width_internal() - 1, x2 | 0x3);
    y2 = std::min(this->get_height_internal() - 1, y2 | 0x3);
  }

  const uint16_t width = static_cast<uint16_t>(x2 - x1 + 1);
  const uint16_t height = static_cast<uint16_t>(y2 - y1 + 1);

  this->write_buffer_to_display_(static_cast<uint16_t>(x1), static_cast<uint16_t>(y1), width, height, this->buffer_);
  this->update_area_(static_cast<uint16_t>(x1), static_cast<uint16_t>(y1), width, height, mode);

  this->max_x_ = 0;
  this->max_y_ = 0;
  this->min_x_ = this->get_width_internal();
  this->min_y_ = this->get_height_internal();

  if (this->sleep_when_done_) {
    this->write_command_(IT8951_TCON_SLEEP);
  }
}

void IT8951ESensor::write_display(UpdateMode mode) { this->write_display_internal_(mode, false, true); }

void IT8951ESensor::write_display(UpdateMode mode, bool full) { this->write_display_internal_(mode, full, false); }

void IT8951ESensor::clear(bool init) {
  if (this->buffer_ != nullptr) {
    memset(this->buffer_, 0xFF, this->get_buffer_length_());
  }

  if (this->sleep_when_done_) {
    this->write_command_(IT8951_TCON_SYS_RUN);
  }

  this->endian_type_ = IT8951_LDIMG_L_ENDIAN;
  this->pixel_bpp_ = IT8951_4BPP;

  this->set_target_memory_addr_(this->devices_[this->model_].dev_info.image_buffer_addr_l,
                                this->devices_[this->model_].dev_info.image_buffer_addr_h);
  this->set_area_(0, 0, this->get_width_internal(), this->get_height_internal());

  this->enable();
  this->write_byte16(0x0000);

  std::array<uint8_t, SPI_STREAM_CHUNK> fill_chunk{};
  fill_chunk.fill(0xFF);
  uint32_t remaining = this->get_buffer_length_();
  uint32_t transferred_bytes = 0;

  while (remaining > 0) {
    const size_t chunk = std::min<size_t>(remaining, fill_chunk.size());
    this->write_array(fill_chunk.data(), chunk);
    remaining -= static_cast<uint32_t>(chunk);
    transferred_bytes += static_cast<uint32_t>(chunk);
    if ((transferred_bytes % SPI_YIELD_BYTES) == 0) {
      delay(1);
    }
  }
  this->disable();
  this->write_command_(IT8951_TCON_LD_IMG_END);

  if (init) {
    this->update_area_(0, 0, this->get_width_internal(), this->get_height_internal(), UPDATE_MODE_INIT);
  }

  this->max_x_ = 0;
  this->max_y_ = 0;
  this->min_x_ = this->get_width_internal();
  this->min_y_ = this->get_height_internal();

  if (this->sleep_when_done_) {
    this->write_command_(IT8951_TCON_SLEEP);
  }
}

void IT8951ESensor::update() {
  if (!this->is_ready()) {
    return;
  }
  this->do_update_();
  this->write_display(UPDATE_MODE_DU);
}

void IT8951ESensor::update_slow() {
  if (!this->is_ready()) {
    return;
  }
  this->do_update_();
  this->write_display(UPDATE_MODE_GC16);
}

void IT8951ESensor::fill(Color color) {
  if (this->buffer_ == nullptr) {
    return;
  }
  const uint8_t gray = color.raw_32 & 0x0F;
  const uint8_t packed = static_cast<uint8_t>((gray << 4) | gray);
  memset(this->buffer_, packed, this->get_buffer_length_());
  this->mark_full_dirty_();
}

void HOT IT8951ESensor::draw_pixel_at(int x, int y, Color color) {
  if (!Display::get_clipping().inside(x, y)) {
    return;
  }

  switch (this->rotation_) {
    case display::DISPLAY_ROTATION_0_DEGREES:
      break;
    case display::DISPLAY_ROTATION_90_DEGREES:
      std::swap(x, y);
      x = this->panel_width_ - x - 1;
      break;
    case display::DISPLAY_ROTATION_180_DEGREES:
      x = this->panel_width_ - x - 1;
      y = this->panel_height_ - y - 1;
      break;
    case display::DISPLAY_ROTATION_270_DEGREES:
      std::swap(x, y);
      y = this->panel_height_ - y - 1;
      break;
  }

  this->draw_absolute_pixel_internal(x, y, color);
}

void HOT IT8951ESensor::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || y < 0 || this->buffer_ == nullptr || x >= this->panel_width_ || y >= this->panel_height_) {
    return;
  }

  if (x > this->max_x_) {
    this->max_x_ = x;
  }
  if (y > this->max_y_) {
    this->max_y_ = y;
  }
  if (x < this->min_x_) {
    this->min_x_ = x;
  }
  if (y < this->min_y_) {
    this->min_y_ = y;
  }

  const uint8_t gray = color.raw_32 & 0x0F;
  const uint16_t byte_width = this->panel_width_ >> 1;
  const uint32_t index = static_cast<uint32_t>(y) * byte_width + (static_cast<uint32_t>(x) >> 1);

  uint8_t &buf = this->buffer_[index];
  if ((x & 0x1) != 0) {
    if ((buf & 0x0F) == gray) {
      return;
    }
    buf = static_cast<uint8_t>((buf & 0xF0) | gray);
  } else {
    const uint8_t upper = static_cast<uint8_t>(gray << 4);
    if ((buf & 0xF0) == upper) {
      return;
    }
    buf = static_cast<uint8_t>((buf & 0x0F) | upper);
  }
}

void IT8951ESensor::set_model(it8951eModel model) {
  this->model_ = model;
  this->panel_width_ = this->devices_[model].dev_info.panel_width;
  this->panel_height_ = this->devices_[model].dev_info.panel_height;
  this->max_x_ = 0;
  this->max_y_ = 0;
  this->min_x_ = this->panel_width_;
  this->min_y_ = this->panel_height_;
}

void IT8951ESensor::dump_config() {
  LOG_DISPLAY("", "IT8951E", this);
  LOG_PIN("  Busy pin: ", this->busy_pin_);
  LOG_PIN("  Reset pin: ", this->reset_pin_);

  switch (this->model_) {
    case it8951eModel::M5EPD:
      ESP_LOGCONFIG(TAG, "  Model: M5EPD");
      break;
    default:
      ESP_LOGCONFIG(TAG, "  Model: unknown");
      break;
  }

  ESP_LOGCONFIG(TAG, "  Panel: %d x %d", this->panel_width_, this->panel_height_);
  ESP_LOGCONFIG(TAG, "  Reversed: %s", YESNO(this->reversed_));
  ESP_LOGCONFIG(TAG, "  Sleep when done: %s", YESNO(this->sleep_when_done_));
  ESP_LOGCONFIG(TAG, "  Full update every: %u", static_cast<unsigned int>(this->full_update_every_));
  ESP_LOGCONFIG(TAG, "  Reset duration: %u ms", static_cast<unsigned int>(this->reset_duration_));
  ESP_LOGCONFIG(TAG, "  LUT: %s", this->devices_[this->model_].dev_info.lut_version);
  ESP_LOGCONFIG(TAG, "  FW: %s", this->devices_[this->model_].dev_info.fw_version);
}

}  // namespace it8951e
}  // namespace esphome
