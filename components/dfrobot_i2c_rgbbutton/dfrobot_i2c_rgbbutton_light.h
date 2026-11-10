#pragma once

#include "esphome/core/defines.h"

#ifdef USE_LIGHT

#include <cmath>

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/core/helpers.h"
#include "dfrobot_i2c_rgbbutton.h"

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

/// The button's LED as an ESPHome RGB light.
class DFRobotI2CRGBButtonLight : public light::LightOutput, public Parented<DFRobotI2CRGBButton> {
 public:
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }

  void write_state(light::LightState *state) override {
    float red, green, blue;
    state->current_values_as_rgb(&red, &green, &blue);
    this->parent_->write_color(to_byte_(red), to_byte_(green), to_byte_(blue));
  }

 protected:
  static uint8_t to_byte_(float value) { return static_cast<uint8_t>(std::lround(clamp(value, 0.0f, 1.0f) * 255.0f)); }
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome

#endif  // USE_LIGHT
