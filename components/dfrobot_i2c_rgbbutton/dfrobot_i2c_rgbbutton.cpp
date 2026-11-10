#include "dfrobot_i2c_rgbbutton.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>

#include "esphome/core/log.h"

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

static const char *const TAG = "dfrobot_i2c_rgbbutton";

static const uint16_t PRODUCT_ID = 0x43DF;

// Register map (from the DFRobot_RGBButton library)
static const uint8_t REG_RED = 0x01;  // followed by green (0x02) and blue (0x03)
static const uint8_t REG_BUTTON = 0x04;
static const uint8_t REG_PID_MSB = 0x09;  // followed by PID LSB (0x0A)

// The address is selected with the DIP switch on the module: 0x23 + switch value (0..7)
static const uint8_t FIRST_ADDRESS = 0x23;
static const uint8_t LAST_ADDRESS = 0x2A;

void DFRobotI2CRGBButton::setup() {
  uint16_t product_id;
  if (!this->read_product_id_(&product_id)) {
    ESP_LOGE(TAG, "No device responds at address 0x%02X", this->address_);
    this->scan_addresses_();
    this->mark_failed(LOG_STR("No device found, check wiring and I2C address"));
    return;
  }
  if (product_id != PRODUCT_ID) {
    ESP_LOGE(TAG, "Device at 0x%02X has product ID 0x%04X, expected 0x%04X (not an RGB button?)", this->address_,
             product_id, PRODUCT_ID);
    this->mark_failed(LOG_STR("Unexpected product ID"));
    return;
  }

  if (this->interrupt_pin_ != nullptr) {
    // INT is high while the button is pressed: no I2C polling needed.
    this->stop_poller();
    this->interrupt_pin_->setup();
    this->publish_button_(this->interrupt_pin_->digital_read());
  } else {
    this->disable_loop();
    this->read_button_();
  }

#ifdef USE_LIGHT
  // With a light, its restore_mode / initial_state determines the color at boot.
  if (this->light_state_ != nullptr)
    return;
#endif
  this->write_color((this->default_color_ >> 16) & 0xFF, (this->default_color_ >> 8) & 0xFF,
                    this->default_color_ & 0xFF, true);
}

void DFRobotI2CRGBButton::loop() {
  // Only active with an interrupt pin (disabled in setup() otherwise).
  this->publish_button_(this->interrupt_pin_->digital_read());
}

void DFRobotI2CRGBButton::update() { this->read_button_(); }

void DFRobotI2CRGBButton::dump_config() {
  ESP_LOGCONFIG(TAG, "DFRobot I2C RGB Button:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication with the module failed!");
    return;
  }
  if (this->interrupt_pin_ != nullptr) {
    LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  } else {
    LOG_UPDATE_INTERVAL(this);
  }
  ESP_LOGCONFIG(TAG, "  Color: 0x%06" PRIX32, this->get_button_color_hex());
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Button", this->button_sensor_);
#endif
}

// ---- Color ----

void DFRobotI2CRGBButton::set_color(uint32_t color) {
  this->apply_color_((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, false);
}

void DFRobotI2CRGBButton::set_color(uint8_t red, uint8_t green, uint8_t blue) {
  this->apply_color_(red, green, blue, false);
}

void DFRobotI2CRGBButton::set_button_color(uint8_t red, uint8_t green, uint8_t blue, bool force) {
  this->apply_color_(red, green, blue, force);
}

void DFRobotI2CRGBButton::set_button_color(uint32_t color, bool force) {
  this->apply_color_((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, force);
}

void DFRobotI2CRGBButton::apply_color_(uint8_t red, uint8_t green, uint8_t blue, bool force) {
#ifdef USE_LIGHT
  if (this->light_state_ != nullptr) {
    // Route through the light so its state (and Home Assistant) stays in sync. The light
    // platform defaults to gamma_correct 1.0, so the module receives exactly these values.
    const uint8_t max_value = std::max({red, green, blue});
    auto call = this->light_state_->make_call();
    call.set_transition_length(0);
    if (max_value == 0) {
      call.set_state(false);
    } else {
      call.set_state(true);
      call.set_brightness(max_value / 255.0f);
      call.set_rgb(red / float(max_value), green / float(max_value), blue / float(max_value));
    }
    call.perform();
    return;
  }
#endif
  this->write_color(red, green, blue, force);
}

bool DFRobotI2CRGBButton::write_color(uint8_t red, uint8_t green, uint8_t blue, bool force) {
  if (this->is_failed())
    return false;
  const uint8_t rgb[3] = {red, green, blue};
  if (!force && this->color_written_ && std::memcmp(rgb, this->color_, sizeof(rgb)) == 0)
    return true;

  if (this->write_register(REG_RED, rgb, sizeof(rgb)) != i2c::ERROR_OK) {
    // Keep the old cached color so the next call retries the write.
    if (!this->write_failed_)
      ESP_LOGW(TAG, "Writing color 0x%02X%02X%02X failed", red, green, blue);
    this->write_failed_ = true;
    this->update_status_();
    return false;
  }
  std::memcpy(this->color_, rgb, sizeof(rgb));
  this->color_written_ = true;
  ESP_LOGV(TAG, "Color set to 0x%02X%02X%02X", red, green, blue);
  if (this->write_failed_) {
    ESP_LOGI(TAG, "Writing the color works again");
    this->write_failed_ = false;
    this->update_status_();
  }
  return true;
}

uint32_t DFRobotI2CRGBButton::get_button_color_hex() const {
  return (uint32_t(this->color_[0]) << 16) | (uint32_t(this->color_[1]) << 8) | this->color_[2];
}

// ---- Button ----

void DFRobotI2CRGBButton::read_button_() {
  uint8_t value;
  if (this->read_register(REG_BUTTON, &value, 1) != i2c::ERROR_OK) {
    if (!this->read_failed_)
      ESP_LOGW(TAG, "Reading the button state failed");
    this->read_failed_ = true;
    this->update_status_();
    return;  // keep the last known state
  }
  if (this->read_failed_) {
    ESP_LOGI(TAG, "Reading the button state works again");
    this->read_failed_ = false;
    this->update_status_();
  }
  this->publish_button_(value != 0);
}

void DFRobotI2CRGBButton::publish_button_(bool pressed) {
  if (this->button_state_known_ && pressed == this->button_state_)
    return;
  this->button_state_ = pressed;
  this->button_state_known_ = true;
#ifdef USE_BINARY_SENSOR
  if (this->button_sensor_ != nullptr)
    this->button_sensor_->publish_state(pressed);
#endif
}

void DFRobotI2CRGBButton::update_status_() {
  if (this->read_failed_ || this->write_failed_) {
    this->status_set_warning(LOG_STR("I2C communication failed"));
  } else {
    this->status_clear_warning();
  }
}

// ---- Detection ----

bool DFRobotI2CRGBButton::read_product_id_(uint16_t *product_id) {
  uint8_t buf[2];
  if (this->read_register(REG_PID_MSB, buf, sizeof(buf)) != i2c::ERROR_OK)
    return false;
  *product_id = (uint16_t(buf[0]) << 8) | buf[1];
  return true;
}

void DFRobotI2CRGBButton::scan_addresses_() {
  // Help with a wrong `address:` / DIP switch setting: look for RGB buttons on all possible addresses.
  const uint8_t configured = this->address_;
  bool found = false;
  for (uint8_t address = FIRST_ADDRESS; address <= LAST_ADDRESS; address++) {
    if (address == configured)
      continue;
    this->set_i2c_address(address);
    uint16_t product_id;
    if (this->read_product_id_(&product_id) && product_id == PRODUCT_ID) {
      ESP_LOGW(TAG, "Found an RGB button at address 0x%02X (DIP switch %d%d%d)", address,
               ((address - FIRST_ADDRESS) >> 2) & 1, ((address - FIRST_ADDRESS) >> 1) & 1, (address - FIRST_ADDRESS) & 1);
      found = true;
    }
  }
  this->set_i2c_address(configured);
  if (!found)
    ESP_LOGW(TAG, "No RGB button found on addresses 0x%02X-0x%02X", FIRST_ADDRESS, LAST_ADDRESS);
}

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
