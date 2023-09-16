#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/defines.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "DFRobot_RGBButton.h" // https://github.com/DFRobot/DFRobot_RGBButton

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

class DFRobot_i2c_RGBButton : public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;

  void set_color(uint32_t color) { this->default_color_ = color; }
  void set_button_address(uint8_t address) { this->address_ = address; }
  void set_button_wire(bool secondWire) { this->secondWire_ = secondWire; }
  
  float get_setup_priority() const override;

  void set_button_sensor(binary_sensor::BinarySensor* button_sensor) {
    this->button_sensor_ = button_sensor;
  }

  // Set button color
  void set_button_color(uint8_t r, uint8_t g, uint8_t b);
  void set_button_color(long color);
  void set_button_color(DFRobot_RGBButton::eGeneralRGBValue_t color);
  // Get button color
  unsigned long get_button_color();
  // Get button state
  bool get_button_state();

 protected:
  DFRobot_RGBButton button_;

  CallbackManager<void()> on_press_callback_;
  CallbackManager<void()> on_release_callback_;
  binary_sensor::BinarySensor* button_sensor_{nullptr};
  
  uint8_t address_ {0x23};
  bool secondWire_ {false};
  uint8_t button_color_r_ {255};
  uint8_t button_color_g_ {255}; 
  uint8_t button_color_b_ {255}; 
  bool button_state_ {false};   // current state
  long default_color_ {0xFFFFFF}; // default color
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome