#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

class DFRobot_i2c_RGBButton : public i2c::I2CDevice, public PollingComponent {
 public:
  /**
  * @enum  eGeneralRGBValue_t
  * @brief  RGB values for general seven colors and white & black
  */
  enum class eGeneralRGBValue_t : uint32_t {
   eRed    = 0xFF0000,   /**< Red */
   eOrange = 0xFF7F00,   /**< Orange */
   eYellow = 0xFFFF00,   /**< Yellow */
   eGreen  = 0x00FF00,   /**< Green */
   eCyan   = 0x00FFFF,   /**< Indigo */
   eBlue   = 0x0000FF,   /**< Blue */
   ePurple = 0x8B00FF,   /**< Purple */
   eWhite  = 0xFFFFFF,   /**< White (for turning on LED) */
   eBlack  = 0x000000,   /**< Black (for turning off LED) */
  };

  void setup() override;
  void loop() override;
  void update() override;

  void set_default_color(uint32_t color) { this->default_color_ = color; }

  float get_setup_priority() const override;

  void set_button_sensor(binary_sensor::BinarySensor* button_sensor) {
    this->button_sensor_ = button_sensor;
  }

  // Set button color
  void set_button_color(uint8_t r, uint8_t g, uint8_t b, bool force = false);
  void set_button_color(unsigned long color, bool force = false);
  void set_button_color(eGeneralRGBValue_t color, bool force = false);
  // Get button color
  uint8_t* get_button_color_rgb();
  unsigned long get_button_color_hex();
  // Get button state
  bool get_button_state();

 protected:
  bool begin();
  uint8_t get_i2c_address();
  uint16_t get_pid();
  uint16_t concat_bytes(uint8_t msb, uint8_t lsb);

 private:
  uint8_t deviceAddr_;
  binary_sensor::BinarySensor* button_sensor_{nullptr};

  uint8_t button_color_[3] {0, 0, 0};
  bool button_state_ {false};   // current state
  long default_color_ {0xFFFFFF}; // default color
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
