#include "dfrobot_i2c_rgbbutton.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>

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

// A module is considered gone after this many failed transfers in a row ...
static const uint8_t MAX_ERRORS = 3;
// ... and is then looked for at this interval.
static const uint32_t PROBE_INTERVAL_MS = 5000;

// Scheduler names (per component)
static const char *const TIMER_PROBE = "probe";
static const char *const TIMER_REFRESH = "refresh";
static const char *const TIMER_LONG_PRESS = "long_press";
static const char *const TIMER_CLICKS = "clicks";

static void color_to_rgb(uint32_t color, uint8_t *rgb) {
  rgb[0] = (color >> 16) & 0xFF;
  rgb[1] = (color >> 8) & 0xFF;
  rgb[2] = color & 0xFF;
}

static uint32_t rgb_to_color(uint8_t red, uint8_t green, uint8_t blue) {
  return (uint32_t(red) << 16) | (uint32_t(green) << 8) | blue;
}

// ---------------------------------------------------------------------------------------------
// Setup and connection
// ---------------------------------------------------------------------------------------------

void DFRobotI2CRGBButton::setup() {
  color_to_rgb(this->color_, this->color_rgb_);
  if (this->interrupt_pin_ != nullptr) {
    // INT is high while the button is pressed: no I2C polling needed.
    this->stop_poller();
    this->interrupt_pin_->setup();
  }

  switch (this->probe_()) {
    case PROBE_WRONG_DEVICE:
      this->mark_failed(LOG_STR("Unexpected product ID"));
      return;
    case PROBE_NO_RESPONSE:
      ESP_LOGE(TAG, "No device responds at address 0x%02X", this->address_);
      this->scan_addresses_();
      this->go_offline_("not found at startup");
      break;
    case PROBE_OK:
      this->go_online_();
      break;
  }

  if (this->refresh_interval_ != SCHEDULER_DONT_RUN && this->refresh_interval_ > 0) {
    // Rewrite the color now and then: a module that lost power for a moment comes back dark.
    this->set_interval(TIMER_REFRESH, this->refresh_interval_, [this]() {
      this->written_valid_ = false;
      this->output_();
    });
  }
  this->update_loop_();
}

DFRobotI2CRGBButton::ProbeResult DFRobotI2CRGBButton::probe_() {
  uint8_t buf[2];
  if (this->read_register(REG_PID_MSB, buf, sizeof(buf)) != i2c::ERROR_OK)
    return PROBE_NO_RESPONSE;
  const uint16_t product_id = (uint16_t(buf[0]) << 8) | buf[1];
  if (product_id != PRODUCT_ID) {
    ESP_LOGE(TAG, "Device at 0x%02X has product ID 0x%04X, expected 0x%04X (not an RGB button?)", this->address_,
             product_id, PRODUCT_ID);
    return PROBE_WRONG_DEVICE;
  }
  return PROBE_OK;
}

void DFRobotI2CRGBButton::scan_addresses_() {
  // Help with a wrong `address:` / DIP switch setting: look for RGB buttons on all possible addresses.
  const uint8_t configured = this->address_;
  bool found = false;
  for (uint8_t address = FIRST_ADDRESS; address <= LAST_ADDRESS; address++) {
    if (address == configured)
      continue;
    this->set_i2c_address(address);
    uint8_t buf[2];
    if (this->read_register(REG_PID_MSB, buf, sizeof(buf)) == i2c::ERROR_OK &&
        ((uint16_t(buf[0]) << 8) | buf[1]) == PRODUCT_ID) {
      const int dip = address - FIRST_ADDRESS;
      ESP_LOGW(TAG, "Found an RGB button at address 0x%02X (DIP switch %d%d%d)", address, (dip >> 2) & 1,
               (dip >> 1) & 1, dip & 1);
      found = true;
    }
  }
  this->set_i2c_address(configured);
  if (!found)
    ESP_LOGW(TAG, "No RGB button found on addresses 0x%02X-0x%02X", FIRST_ADDRESS, LAST_ADDRESS);
}

void DFRobotI2CRGBButton::go_online_() {
  this->cancel_interval(TIMER_PROBE);
  this->online_ = true;
  this->error_count_ = 0;
  this->status_clear_warning();
  ESP_LOGI(TAG, "Module at 0x%02X connected", this->address_);

  // The module may have been reset: always write the color again.
  this->written_valid_ = false;
  this->output_();
  if (this->interrupt_pin_ != nullptr) {
    this->publish_button_(this->interrupt_pin_->digital_read());
  } else {
    this->read_button_();
  }
#ifdef USE_BINARY_SENSOR
  if (this->connected_sensor_ != nullptr)
    this->connected_sensor_->publish_state(true);
#endif
  this->update_loop_();
}

void DFRobotI2CRGBButton::go_offline_(const char *reason) {
  const bool was_online = this->online_;
  this->online_ = false;
  this->written_valid_ = false;
  if (was_online)
    ESP_LOGW(TAG, "Module at 0x%02X lost (%s), looking for it every %" PRIu32 " s", this->address_, reason,
             PROBE_INTERVAL_MS / 1000);
  this->status_set_warning(LOG_STR("Module not reachable"));
#ifdef USE_BINARY_SENSOR
  if (this->connected_sensor_ != nullptr)
    this->connected_sensor_->publish_state(false);
#endif
  // Nobody can press a button that is gone: report it released (not as a click), drop pending clicks.
  this->long_press_fired_ = true;
  this->publish_button_(false);
  this->cancel_timeout(TIMER_LONG_PRESS);
  this->cancel_timeout(TIMER_CLICKS);
  this->click_count_ = 0;

  this->set_interval(TIMER_PROBE, PROBE_INTERVAL_MS, [this]() {
    switch (this->probe_()) {
      case PROBE_OK:
        this->go_online_();
        break;
      case PROBE_WRONG_DEVICE:
        this->cancel_interval(TIMER_PROBE);
        this->mark_failed(LOG_STR("Unexpected product ID"));
        break;
      case PROBE_NO_RESPONSE:
        break;
    }
  });
  this->update_loop_();
}

void DFRobotI2CRGBButton::communication_error_() {
  if (!this->online_)
    return;
  if (++this->error_count_ >= MAX_ERRORS) {
    this->go_offline_("no answer");
    return;
  }
  if (this->error_count_ == 1)
    ESP_LOGW(TAG, "I2C communication with 0x%02X failed", this->address_);
  this->status_set_warning(LOG_STR("I2C communication failed"));
}

void DFRobotI2CRGBButton::communication_ok_() {
  if (this->error_count_ == 0)
    return;
  this->error_count_ = 0;
  this->status_clear_warning();
}

void DFRobotI2CRGBButton::update_loop_() {
  // loop() is only needed for the INT pin and for blinking.
  if (this->online_ && (this->interrupt_pin_ != nullptr || this->blinking_)) {
    this->enable_loop();
  } else {
    this->disable_loop();
  }
}

void DFRobotI2CRGBButton::loop() {
  if (!this->online_)
    return;
  if (this->interrupt_pin_ != nullptr)
    this->publish_button_(this->interrupt_pin_->digital_read());
  if (this->blinking_)
    this->output_();
}

void DFRobotI2CRGBButton::update() {
  if (!this->online_)
    return;
  this->read_button_();
}

void DFRobotI2CRGBButton::dump_config() {
  ESP_LOGCONFIG(TAG, "DFRobot I2C RGB Button:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  The device at this address is not an RGB button!");
    return;
  }
  ESP_LOGCONFIG(TAG, "  Connected: %s", YESNO(this->online_));
  if (this->interrupt_pin_ != nullptr) {
    LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  } else {
    LOG_UPDATE_INTERVAL(this);
  }
  ESP_LOGCONFIG(TAG,
                "  Color: 0x%06" PRIX32 "\n"
                "  Brightness: %.0f%%\n"
                "  Blink interval: %" PRIu32 " ms",
                this->color_, this->brightness_ * 100.0f, this->blink_interval_);
  if (this->click_detection_) {
    ESP_LOGCONFIG(TAG,
                  "  Long press time: %" PRIu32 " ms\n"
                  "  Multi click time: %" PRIu32 " ms",
                  this->long_press_time_, this->multi_click_time_);
  }
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Button", this->button_sensor_);
  LOG_BINARY_SENSOR("  ", "Connected", this->connected_sensor_);
#endif
}

// ---------------------------------------------------------------------------------------------
// Color
// ---------------------------------------------------------------------------------------------

void DFRobotI2CRGBButton::set_color(uint32_t color) { this->apply_color_(color & 0xFFFFFF, false); }

void DFRobotI2CRGBButton::set_color(uint8_t red, uint8_t green, uint8_t blue) {
  this->apply_color_(rgb_to_color(red, green, blue), false);
}

void DFRobotI2CRGBButton::set_button_color(uint8_t red, uint8_t green, uint8_t blue, bool force) {
  this->apply_color_(rgb_to_color(red, green, blue), force);
}

void DFRobotI2CRGBButton::set_button_color(uint32_t color, bool force) {
  this->apply_color_(color & 0xFFFFFF, force);
}

void DFRobotI2CRGBButton::blink(uint32_t color, uint32_t alternate) {
  this->store_color_(color & 0xFFFFFF);
  this->alternate_color_ = alternate & 0xFFFFFF;
  if (!this->blinking_) {
    this->blinking_ = true;
    this->update_loop_();
  }
  this->output_();
}

void DFRobotI2CRGBButton::set_brightness(float brightness) {
  this->brightness_ = clamp(brightness, 0.0f, 1.0f);
  this->output_();
}

void DFRobotI2CRGBButton::apply_color_(uint32_t color, bool force) {
  if (this->blinking_) {
    this->blinking_ = false;
    this->update_loop_();
  }
#ifdef USE_LIGHT
  if (this->light_state_ != nullptr) {
    // Route through the light so its state (and Home Assistant) stays in sync. The light
    // platform defaults to gamma_correct 1.0, so the module receives exactly these values.
    uint8_t rgb[3];
    color_to_rgb(color, rgb);
    const uint8_t max_value = std::max({rgb[0], rgb[1], rgb[2]});
    auto call = this->light_state_->make_call();
    call.set_transition_length(0);
    if (max_value == 0) {
      call.set_state(false);
    } else {
      call.set_state(true);
      call.set_brightness(max_value / 255.0f);
      call.set_rgb(rgb[0] / float(max_value), rgb[1] / float(max_value), rgb[2] / float(max_value));
    }
    call.perform();
    if (force)
      this->written_valid_ = false;
    return;
  }
#endif
  this->store_color_(color);
  if (force)
    this->written_valid_ = false;
  this->output_();
}

void DFRobotI2CRGBButton::write_color(uint8_t red, uint8_t green, uint8_t blue, bool force) {
  if (this->blinking_) {
    this->blinking_ = false;
    this->update_loop_();
  }
  this->store_color_(rgb_to_color(red, green, blue));
  if (force)
    this->written_valid_ = false;
  this->output_();
}

void DFRobotI2CRGBButton::store_color_(uint32_t color) {
  this->color_ = color;
  color_to_rgb(color, this->color_rgb_);
}

bool DFRobotI2CRGBButton::blink_phase_() const {
  if (this->blink_interval_ == 0)
    return false;
  return (millis() / this->blink_interval_) & 1;
}

void DFRobotI2CRGBButton::output_() {
  if (!this->online_ || this->is_failed())
    return;
  const uint32_t color = (this->blinking_ && this->blink_phase_()) ? this->alternate_color_ : this->color_;
  uint8_t rgb[3];
  color_to_rgb(color, rgb);
  if (this->brightness_ < 1.0f) {
    for (auto &channel : rgb)
      channel = static_cast<uint8_t>(std::lround(channel * this->brightness_));
  }
  const uint32_t value = rgb_to_color(rgb[0], rgb[1], rgb[2]);
  if (this->written_valid_ && value == this->written_)
    return;

  if (this->write_register(REG_RED, rgb, sizeof(rgb)) != i2c::ERROR_OK) {
    this->written_valid_ = false;  // retried with the next update / blink step / refresh
    this->communication_error_();
    return;
  }
  this->written_ = value;
  this->written_valid_ = true;
  ESP_LOGV(TAG, "Color 0x%06" PRIX32, value);
  this->communication_ok_();
}

// ---------------------------------------------------------------------------------------------
// Button and clicks
// ---------------------------------------------------------------------------------------------

void DFRobotI2CRGBButton::read_button_() {
  uint8_t value;
  if (this->read_register(REG_BUTTON, &value, 1) != i2c::ERROR_OK) {
    this->communication_error_();
    return;  // keep the last known state
  }
  this->communication_ok_();
  if (!this->written_valid_)
    this->output_();  // retry a failed color write
  this->publish_button_(value != 0);
}

void DFRobotI2CRGBButton::publish_button_(bool pressed) {
  if (this->locked_)
    pressed = false;
  const bool was_known = this->button_state_known_;
  if (was_known && pressed == this->button_state_)
    return;
  this->button_state_ = pressed;
  this->button_state_known_ = true;
#ifdef USE_BINARY_SENSOR
  if (this->button_sensor_ != nullptr)
    this->button_sensor_->publish_state(pressed);
#endif
  if (this->click_detection_ && was_known) {
    if (pressed) {
      this->on_press_();
    } else {
      this->on_release_();
    }
  }
}

void DFRobotI2CRGBButton::lock() {
  if (this->locked_)
    return;
  this->locked_ = true;
  ESP_LOGD(TAG, "Locked");
  this->long_press_fired_ = true;  // releasing by locking is not a click
  this->publish_button_(false);
  this->cancel_timeout(TIMER_LONG_PRESS);
  this->cancel_timeout(TIMER_CLICKS);
  this->click_count_ = 0;
}

void DFRobotI2CRGBButton::unlock() {
  if (!this->locked_)
    return;
  this->locked_ = false;
  ESP_LOGD(TAG, "Unlocked");
}

void DFRobotI2CRGBButton::on_press_() {
  this->cancel_timeout(TIMER_CLICKS);
  this->long_press_fired_ = false;
  this->set_timeout(TIMER_LONG_PRESS, this->long_press_time_, [this]() {
    this->long_press_fired_ = true;
    if (this->click_count_ > 0)
      this->finish_clicks_();  // clicks before the long press
    this->report_long_press_();
  });
}

void DFRobotI2CRGBButton::on_release_() {
  this->cancel_timeout(TIMER_LONG_PRESS);
  if (this->long_press_fired_)
    return;
  this->click_count_++;
  if (!this->multi_click_ || this->click_count_ >= 3) {
    this->finish_clicks_();
  } else {
    this->set_timeout(TIMER_CLICKS, this->multi_click_time_, [this]() { this->finish_clicks_(); });
  }
}

void DFRobotI2CRGBButton::finish_clicks_() {
  this->cancel_timeout(TIMER_CLICKS);
  const uint8_t count = this->click_count_;
  this->click_count_ = 0;
  if (count > 0)
    this->report_clicks_(count);
}

void DFRobotI2CRGBButton::report_clicks_(uint8_t count) {
  const char *type;
  if (count == 1) {
    type = "click";
    this->click_callback_.call();
  } else if (count == 2) {
    type = "double_click";
    this->double_click_callback_.call();
  } else {
    type = "triple_click";
    this->triple_click_callback_.call();
  }
  ESP_LOGD(TAG, "0x%02X: %s", this->address_, type);
#ifdef USE_EVENT
  if (this->click_event_ != nullptr)
    this->click_event_->trigger(type);
#endif
}

void DFRobotI2CRGBButton::report_long_press_() {
  ESP_LOGD(TAG, "0x%02X: long_press", this->address_);
  this->long_press_callback_.call();
#ifdef USE_EVENT
  if (this->click_event_ != nullptr)
    this->click_event_->trigger("long_press");
#endif
}

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
