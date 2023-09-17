#include "esphome/core/log.h"

#include "dfrobot_i2c_rgbbutton.h"

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

static const char *TAG = "dfrobot_i2c_rgbbutton";

float DFRobot_i2c_RGBButton::get_setup_priority() const { return setup_priority::IO; } // for ESPHome

void DFRobot_i2c_RGBButton::setup() { // triggers at startup
  ESP_LOGI(TAG, "Setting up DFRobot_i2c_RGBButton...");
  if (this->secondaryBus_) // check if second i2c bus is selected or not
    this->button_ = DFRobot_RGBButton(&Wire1, this->address_);
  else
    this->button_ = DFRobot_RGBButton(&Wire, this->address_);

  while(!this->button_.begin()){ // initialize connection
    ESP_LOGI(TAG, "NO devices found with address %d!", this->address_);
    delay(1000);
  }

  this->set_button_color(this->default_color_); // set default button color
  ESP_LOGI(TAG, "Device connected successfully!");
}

void DFRobot_i2c_RGBButton::loop() { } // triggers every clock cycle

void DFRobot_i2c_RGBButton::update() { // triggers on update_interval
  this->get_button_state(); // get button state
}

void DFRobot_i2c_RGBButton::set_button_color(uint8_t r, uint8_t g, uint8_t b) { // set button color via r, g, b values
  if (r == this->button_color_r_ && g == this->button_color_g_ && b == this->button_color_b_) { // checks if the color is already applied
    ESP_LOGD(TAG, "Color hasn't changed, no changes applied.");
    return;
  }

  // set color and store the rgb values
  this->button_.setRGBColor(r, g, b);
  this->button_color_r_ = r;
  this->button_color_g_ = g;
  this->button_color_b_ = b;

  ESP_LOGD(TAG, "New color set:");
  ESP_LOGD(TAG, "R: %d; G: %d, B:%d", this->button_color_r_, this->button_color_g_, this->button_color_b_);
}

void DFRobot_i2c_RGBButton::set_button_color(unsigned long color) {
  // convert hex to RGB
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  if (r == this->button_color_r_ && g == this->button_color_g_ && b == this->button_color_b_) { // checks if the color is already applied
    ESP_LOGD(TAG, "Color hasn't changed, no changes applied.");
    return;
  }

  // set color and store the rgb values
  this->button_.setRGBColor(r, g, b);
  this->button_color_r_ = r;
  this->button_color_g_ = g;
  this->button_color_b_ = b;

  ESP_LOGD(TAG, "New color set:");
  ESP_LOGD(TAG, "R: %d; G: %d, B:%d", this->button_color_r_, this->button_color_g_, this->button_color_b_);
}

void DFRobot_i2c_RGBButton::set_button_color(DFRobot_RGBButton::eGeneralRGBValue_t color) {
  // convert hex to RGB
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  if (r == this->button_color_r_ && g == this->button_color_g_ && b == this->button_color_b_) { // checks if the color is already applied
    ESP_LOGD(TAG, "Color hasn't changed, no changes applied.");
    return;
  }

  // set color and store the rgb values
  this->button_.setRGBColor(color);
  this->button_color_r_ = r;
  this->button_color_g_ = g;
  this->button_color_b_ = b;

  ESP_LOGD(TAG, "New color set:");
  ESP_LOGD(TAG, "R: %d; G: %d, B:%d", this->button_color_r_, this->button_color_g_, this->button_color_b_);
}

uint8_t* DFRobot_i2c_RGBButton::get_button_color_rgb() { // get button color as array (R, G, B)
  static uint8_t rgb[3] = {this->button_color_r_, this->button_color_g_, this->button_color_b_};
  return rgb;
}

unsigned long DFRobot_i2c_RGBButton::get_button_color_hex() { // get button color as hex value (unsigned long)
  return ((this->button_color_r_ & 0xff) << 16) + ((this->button_color_g_ & 0xff) << 8) + (this->button_color_b_ & 0xff);
}

bool DFRobot_i2c_RGBButton::get_button_state() { // get button state
  this->button_state_ = this->button_.getButtonStatus();

  // publish binary sensor state
  if (this->button_sensor_ != nullptr) { // checks if binary_sensor is setup in configuration
    if (!this->button_sensor_->has_state() || (this->button_sensor_->state != this->button_state_)) { // checks if button's last state has no or another state
      if (this->button_state_) { // button pressed
        ESP_LOGD(TAG, "Button pressed!");
        this->button_sensor_->publish_state(this->button_state_);
        this->on_press_callback_.call();
      } else { // button released
        ESP_LOGD(TAG, "Button released!");
        this->button_sensor_->publish_state(this->button_state_);
        this->on_release_callback_.call();
      }
    }
  }

  return this->button_state_;
}

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
