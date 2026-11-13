#pragma once

#include <vector>

#include "esphome/core/automation.h"
#include "dfrobot_i2c_rgbbutton.h"

namespace esphome {
namespace dfrobot_i2c_rgbbutton {

/// Base for actions that act on one or more buttons.
template<typename... Ts> class ButtonsAction : public Action<Ts...> {
 public:
  explicit ButtonsAction(std::initializer_list<DFRobotI2CRGBButton *> buttons) : buttons_(buttons) {}

 protected:
  std::vector<DFRobotI2CRGBButton *> buttons_;
};

template<typename... Ts> class SetColorAction : public ButtonsAction<Ts...> {
 public:
  using ButtonsAction<Ts...>::ButtonsAction;

  TEMPLATABLE_VALUE(uint32_t, color)
  TEMPLATABLE_VALUE(uint8_t, red)
  TEMPLATABLE_VALUE(uint8_t, green)
  TEMPLATABLE_VALUE(uint8_t, blue)
  TEMPLATABLE_VALUE(bool, blink)
  TEMPLATABLE_VALUE(uint32_t, alternate_color)

  void play(const Ts &...x) override {
    uint32_t color;
    if (this->color_.has_value()) {
      color = this->color_.value(x...);
    } else {
      color = (uint32_t(this->red_.value(x...)) << 16) | (uint32_t(this->green_.value(x...)) << 8) |
              this->blue_.value(x...);
    }
    const bool blink = this->blink_.has_value() && this->blink_.value(x...);
    const uint32_t alternate = this->alternate_color_.has_value() ? this->alternate_color_.value(x...) : 0;
    for (auto *button : this->buttons_) {
      if (blink) {
        button->blink(color, alternate);
      } else {
        button->set_color(color);
      }
    }
  }
};

template<typename... Ts> class SetBrightnessAction : public ButtonsAction<Ts...> {
 public:
  using ButtonsAction<Ts...>::ButtonsAction;

  TEMPLATABLE_VALUE(float, brightness)

  void play(const Ts &...x) override {
    const float brightness = this->brightness_.value(x...);
    for (auto *button : this->buttons_)
      button->set_brightness(brightness);
  }
};

template<typename... Ts> class LockAction : public ButtonsAction<Ts...> {
 public:
  using ButtonsAction<Ts...>::ButtonsAction;

  void play(const Ts &...x) override {
    for (auto *button : this->buttons_)
      button->lock();
  }
};

template<typename... Ts> class UnlockAction : public ButtonsAction<Ts...> {
 public:
  using ButtonsAction<Ts...>::ButtonsAction;

  void play(const Ts &...x) override {
    for (auto *button : this->buttons_)
      button->unlock();
  }
};

template<typename... Ts> class IsPressedCondition : public Condition<Ts...>, public Parented<DFRobotI2CRGBButton> {
 public:
  bool check(const Ts &...x) override { return this->parent_->get_button_state(); }
};

template<typename... Ts> class IsLockedCondition : public Condition<Ts...>, public Parented<DFRobotI2CRGBButton> {
 public:
  bool check(const Ts &...x) override { return this->parent_->is_locked(); }
};

template<typename... Ts> class IsConnectedCondition : public Condition<Ts...>, public Parented<DFRobotI2CRGBButton> {
 public:
  bool check(const Ts &...x) override { return this->parent_->is_connected(); }
};

}  // namespace dfrobot_i2c_rgbbutton
}  // namespace esphome
