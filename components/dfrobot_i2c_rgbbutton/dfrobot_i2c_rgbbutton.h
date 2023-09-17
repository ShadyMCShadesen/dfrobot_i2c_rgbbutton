#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#define RGBBUTTON_CONCAT_BYTES(msb, lsb)   (((uint16_t)msb << 8) | (uint16_t)lsb)   ///< Macro combines two 8-bit data into one 16-bit data

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

class DFRobot_i2c_RGBButton : public i2c::I2CDevice, public PollingComponent {
 public:
  /**
  * @enum  eGeneralRGBValue_t
  * @brief  RGB values for general seven colors and white & black
  */
  typedef enum
  {
   eRed    = 0xFF0000,   /**< Red */
   eOrange = 0xFF7F00,   /**< Orange */
   eYellow = 0xFFFF00,   /**< Yellow */
   eGreen  = 0x00FF00,   /**< Green */
   eCyan   = 0x00FFFF,   /**< Indigo */
   eBlue   = 0x0000FF,   /**< Blue */
   ePurple = 0x8B00FF,   /**< Purple */
   eWhite  = 0xFFFFFF,   /**< White (for turning on LED) */
   eBlack  = 0x000000,   /**< Black (for turning off LED) */
  }eGeneralRGBValue_t;

  void setup() override;
  void loop() override;
  void update() override;

  void set_color(uint32_t color) { this->default_color_ = color; }

  float get_setup_priority() const override;

  void set_button_sensor(binary_sensor::BinarySensor* button_sensor) {
    this->button_sensor_ = button_sensor;
  }

  // Set button color
  void set_button_color(uint8_t r, uint8_t g, uint8_t b);
  void set_button_color(unsigned long color);
  void set_button_color(eGeneralRGBValue_t color);
  // Get button color
  uint8_t* get_button_color_rgb();
  unsigned long get_button_color_hex();
  // Get button state
  bool get_button_state();

 protected:
  bool begin(void);
  uint8_t getI2CAddr(void);
  uint16_t getPID(void);

  binary_sensor::BinarySensor* button_sensor_{nullptr};

  uint8_t button_color_[3] {255, 255, 255};
  bool button_state_ {false};   // current state
  long default_color_ {0xFFFFFF}; // default color

 private:
  uint8_t deviceAddr_;
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
