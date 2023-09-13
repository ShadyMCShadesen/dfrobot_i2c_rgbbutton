#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/defines.h"
#include "esphome/components/i2c/i2c.h"

#include "DFRobot_RGBButton.h" // https://github.com/DFRobot/DFRobot_RGBButton

namespace esphome {
namespace dfrobot_i2c_button {

class DFRobot_i2c_RGBButton : public binary_sensor::BinarySensor, public PollingComponent {
 public:
  void set_writer(std::function<void(DFRobot_i2c_RGBButton &)> &&writer) { this->writer_ = std::move(writer); }
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;
  void set_color(DFRobot_RGBButton::eGeneralRGBValue_t color) { this->default_color_ = color; }
  void set_button_address(uint8_t address) { this->address_ = address; }
  void set_button_wire(bool secondWire) { this->secondWire_ = secondWire; }
  void update() override;

  // Update button state (instant)
  void interrupt();
  // Set button color
  void set_button_color(uint8_t r, uint8_t g, uint8_t b);
  void set_button_color(DFRobot_RGBButton::eGeneralRGBValue_t color);
  // Get button color
  uint8_t get_button_color();
  // Get button state
  bool get_button_state();

 protected:
  void command_(uint8_t value);
  void call_writer() { this->writer_(*this); }

  std::function<void(DFRobot_i2c_RGBButton &)> writer_;

  DFRobot_RGBButton button_;
  
  uint8_t address_ {0x23};
  bool secondWire_ {false};
  uint8_t button_color_r_ {255};
  uint8_t button_color_g_ {255}; 
  uint8_t button_color_b_ {255}; 
  bool button_state_ {false};   // current state
  DFRobot_RGBButton::eGeneralRGBValue_t default_color_ {DFRobot_RGBButton::eWhite}; // default color
};

}  // namespace dfrobot_i2c_button
}  // namespace esphome
