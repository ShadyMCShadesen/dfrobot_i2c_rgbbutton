#pragma once

#include <cstdint>

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/i2c/i2c.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_EVENT
#include "esphome/components/event/event.h"
#endif
#ifdef USE_LIGHT
#include "esphome/components/light/light_state.h"
#endif

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

/// DFRobot Gravity I2C RGB LED button (DFR0991).
///
/// The hub reads the button (over I2C, or from the INT pin if configured), detects clicks and
/// owns the LED color (with blinking and brightness). A module that is missing or loses power is
/// detected and set up again when it is back.
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

  // ---- Configuration (called from generated code) ----
  void set_default_color(uint32_t color) { this->color_ = color; }
  void set_interrupt_pin(GPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_blink_interval(uint32_t ms) { this->blink_interval_ = ms; }
  void set_refresh_interval(uint32_t ms) { this->refresh_interval_ = ms; }
  void set_long_press_time(uint32_t ms) { this->long_press_time_ = ms; }
  void set_multi_click_time(uint32_t ms) { this->multi_click_time_ = ms; }
  /// Wait for a second/third click before reporting a click (only needed for double/triple clicks).
  void set_multi_click(bool enabled) { this->multi_click_ = enabled; }
  void set_click_detection(bool enabled) { this->click_detection_ = enabled; }
#ifdef USE_BINARY_SENSOR
  void set_button_sensor(binary_sensor::BinarySensor *sensor) { this->button_sensor_ = sensor; }
  void set_connected_sensor(binary_sensor::BinarySensor *sensor) { this->connected_sensor_ = sensor; }
#endif
#ifdef USE_EVENT
  void set_click_event(event::Event *event) { this->click_event_ = event; }
#endif
#ifdef USE_LIGHT
  void set_light_state(light::LightState *state) { this->light_state_ = state; }
#endif

  // ---- Color ----

  /// Set a steady color (0xRRGGBB); ends blinking. If an RGB light is configured, the change goes
  /// through the light so that Home Assistant shows the same color.
  void set_color(uint32_t color);
  void set_color(uint8_t red, uint8_t green, uint8_t blue);
  /// Alternate between `color` and `alternate` every blink_interval. All buttons with the same
  /// blink_interval blink in the same phase (`color` first). Not shown by the light entity.
  void blink(uint32_t color, uint32_t alternate = 0x000000);
  bool is_blinking() const { return this->blinking_; }
  /// Brightness 0.0-1.0, applied to every color written to this module.
  void set_brightness(float brightness);
  float get_brightness() const { return this->brightness_; }

  /// Earlier API, still supported. `force` rewrites the color even if it did not change.
  void set_button_color(uint8_t red, uint8_t green, uint8_t blue, bool force = false);
  void set_button_color(uint32_t color, bool force = false);

  /// The color set last (not scaled by brightness), as {R, G, B} or 0xRRGGBB.
  const uint8_t *get_button_color_rgb() const { return this->color_rgb_; }
  uint32_t get_button_color_hex() const { return this->color_; }

  /// Called by the light output: shows the light's color (ends blinking).
  void write_color(uint8_t red, uint8_t green, uint8_t blue, bool force = false);

  // ---- Button ----

  /// Last known button state (true = pressed). Does not access the bus.
  bool get_button_state() const { return this->button_state_; }
  /// While locked, presses are ignored: the binary sensor stays off and no clicks are reported.
  void lock();
  void unlock();
  bool is_locked() const { return this->locked_; }
  /// True while the module answers on the bus.
  bool is_connected() const { return this->online_; }

  template<typename F> void add_on_click_callback(F &&callback) { this->click_callback_.add(std::forward<F>(callback)); }
  template<typename F> void add_on_double_click_callback(F &&callback) {
    this->double_click_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_triple_click_callback(F &&callback) {
    this->triple_click_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_long_press_callback(F &&callback) {
    this->long_press_callback_.add(std::forward<F>(callback));
  }

 protected:
  enum ProbeResult : uint8_t { PROBE_OK, PROBE_NO_RESPONSE, PROBE_WRONG_DEVICE };

  ProbeResult probe_();
  void scan_addresses_();
  void go_online_();
  void go_offline_(const char *reason);
  void communication_error_();
  void communication_ok_();
  void update_loop_();

  void apply_color_(uint32_t color, bool force);
  void store_color_(uint32_t color);
  void output_();
  bool blink_phase_() const;

  void read_button_();
  void publish_button_(bool pressed);
  void on_press_();
  void on_release_();
  void finish_clicks_();
  void report_clicks_(uint8_t count);
  void report_long_press_();

  GPIOPin *interrupt_pin_{nullptr};
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *button_sensor_{nullptr};
  binary_sensor::BinarySensor *connected_sensor_{nullptr};
#endif
#ifdef USE_EVENT
  event::Event *click_event_{nullptr};
#endif
#ifdef USE_LIGHT
  light::LightState *light_state_{nullptr};
#endif

  // Configuration
  uint32_t blink_interval_{500};
  uint32_t refresh_interval_{60000};
  uint32_t long_press_time_{1000};
  uint32_t multi_click_time_{400};
  bool multi_click_{false};
  bool click_detection_{false};

  // Color
  uint32_t color_{0x000000};  // steady color, or first blink color
  uint32_t alternate_color_{0x000000};
  uint8_t color_rgb_[3]{0, 0, 0};
  bool blinking_{false};
  float brightness_{1.0f};
  uint32_t written_{0};  // value on the module (after brightness)
  bool written_valid_{false};

  // Connection
  bool online_{false};
  uint8_t error_count_{0};

  // Button
  bool button_state_{false};
  bool button_state_known_{false};
  bool locked_{false};
  uint8_t click_count_{0};
  bool long_press_fired_{false};

  CallbackManager<void()> click_callback_;
  CallbackManager<void()> double_click_callback_;
  CallbackManager<void()> triple_click_callback_;
  CallbackManager<void()> long_press_callback_;
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
