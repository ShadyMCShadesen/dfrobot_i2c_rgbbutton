#pragma once

#include <cstdint>

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_LIGHT
#include "esphome/components/light/light_state.h"
#endif

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

/// DFRobot Gravity I2C RGB LED button (DFR0991).
///
/// The hub reads the button (over I2C, or from the INT pin if configured) and owns the LED
/// color. The LED can additionally be exposed as an RGB light, the button as a binary sensor.
class DFRobotI2CRGBButton : public PollingComponent, public i2c::I2CDevice {
 public:
  /// Predefined colors (kept for lambdas written against earlier versions).
  enum eGeneralRGBValue_t : uint32_t {
    eRed = 0xFF0000,
    eOrange = 0xFF7F00,
    eYellow = 0xFFFF00,
    eGreen = 0x00FF00,
    eCyan = 0x00FFFF,
    eBlue = 0x0000FF,
    ePurple = 0x8B00FF,
    eWhite = 0xFFFFFF,
    eBlack = 0x000000,
  };

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  // Configuration (called from generated code)
  void set_default_color(uint32_t color) { this->default_color_ = color; }
  void set_interrupt_pin(GPIOPin *pin) { this->interrupt_pin_ = pin; }
#ifdef USE_BINARY_SENSOR
  void set_button_sensor(binary_sensor::BinarySensor *sensor) { this->button_sensor_ = sensor; }
#endif
#ifdef USE_LIGHT
  void set_light_state(light::LightState *state) { this->light_state_ = state; }
#endif

  /// Set the LED color (0xRRGGBB). If an RGB light is configured, the change goes through the
  /// light so that Home Assistant shows the same color.
  void set_color(uint32_t color);
  void set_color(uint8_t red, uint8_t green, uint8_t blue);

  /// Earlier API, still supported. `force` rewrites the color even if it did not change.
  void set_button_color(uint8_t red, uint8_t green, uint8_t blue, bool force = false);
  void set_button_color(uint32_t color, bool force = false);

  /// Last color written to the module, as {R, G, B} or 0xRRGGBB.
  const uint8_t *get_button_color_rgb() const { return this->color_; }
  uint32_t get_button_color_hex() const;
  /// Last known button state (true = pressed). Does not access the bus.
  bool get_button_state() const { return this->button_state_; }

  /// Write the color directly to the module (used by the light output).
  /// Returns false if the I2C write failed.
  bool write_color(uint8_t red, uint8_t green, uint8_t blue, bool force = false);

 protected:
  void apply_color_(uint8_t red, uint8_t green, uint8_t blue, bool force);
  bool read_product_id_(uint16_t *product_id);
  void read_button_();
  void publish_button_(bool pressed);
  void update_status_();
  void scan_addresses_();

  GPIOPin *interrupt_pin_{nullptr};
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *button_sensor_{nullptr};
#endif
#ifdef USE_LIGHT
  light::LightState *light_state_{nullptr};
#endif
  uint32_t default_color_{0x000000};
  uint8_t color_[3]{0, 0, 0};
  bool color_written_{false};
  bool button_state_{false};
  bool button_state_known_{false};
  bool read_failed_{false};
  bool write_failed_{false};
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
