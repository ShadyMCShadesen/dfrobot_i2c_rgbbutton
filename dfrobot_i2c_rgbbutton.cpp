#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/i2c/i2c.h"

#include "dfrobot_i2c_rgbbutton.h"
#include "DFRobot_RGBButton.h" // https://github.com/DFRobot/DFRobot_RGBButton

namespace esphome {
namespace dfrobot_i2c_button {

static const char *TAG = "dfrobot_i2c_button";

float DFRobot_i2c_RGBButton::get_setup_priority() const { return setup_priority::HARDWARE; }

void DFRobot_i2c_RGBButton::setup() {
  if (this->secondWire_) // check if second i2c bus is selected or not
    this->button_ = DFRobot_i2c_RGBButton::DFRobot_RGBButton(&Wire1, this->address_);
  else
    this->button_ = DFRobot_i2c_RGBButton::DFRobot_RGBButton(&Wire, this->address_);

  while(!this->button_.begin()){ // initialize connection
    ESP_LOGD("dfrobot_i2c_button", "NO Deivces !");
    delay(1000);
  } 
  
  this->set_button_color(this->default_color_); // set default button color
  ESP_LOGD("dfrobot_i2c_button", "Device connected successfully !");
}

void DFRobot_i2c_RGBButton::loop() {
  // idk
}

void DFRobot_i2c_RGBButton::update() {
  this->get_button_state(); // get button state on update interval
}

void DFRobot_i2c_RGBButton::interrupt() {
  this->update();
  ESP_LOGD(TAG, "Interrupt called!");
}

void DFRobot_i2c_RGBButton::set_button_color(uint8_t r, uint8_t g, uint8_t b) {
  this->button_.setRGBColor(r, g, b);
  this->button_color_r_ = r;
  this->button_color_g_ = g;
  this->button_color_b_ = b;

  ESP_LOGD(TAG, "New color set!");
}

void DFRobot_i2c_RGBButton::set_button_color(DFRobot_RGBButton::eGeneralRGBValue_t color) {
  this->button_.setRGBColor(color);
  this->button_color_r_ = (color >> 16) & 0xFF;
  this->button_color_g_ = (color >> 8) & 0xFF;
  this->button_color_b_ = color & 0xFF;

  ESP_LOGD(TAG, "New color set!");
}

uint8_t DFRobot_i2c_RGBButton::get_button_color() {
  return 0x00; // work in progress - need to return color value
}

bool DFRobot_i2c_RGBButton::get_button_state() {
  this->button_state_ = this->button_.getButtonStatus();
  this->publish_state(this->button_state_);
  return this->button_state_;
}

}  // namespace ht16k33_alpha
}  // namespace esphome

