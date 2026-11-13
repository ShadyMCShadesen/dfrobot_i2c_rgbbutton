# Changelog

## v2.1.0

v2.0.0 configurations keep working.

### New

- **Clicks:** `on_click`, `on_double_click`, `on_triple_click` and `on_long_press` on the hub,
  with `long_press_time` and `multi_click_time`. Replaces long `on_multi_click` blocks.
- **`event` platform:** an event entity with the types `click`, `double_click`, `triple_click`
  and `long_press`, so one Home Assistant automation can handle all buttons.
- **Blinking:** `set_color` with `blink: true` (color / off) or `alternate_color:` (two colors),
  C++ `blink(color, alternate)`. Hub option `blink_interval` (default `500ms`); all buttons with
  the same interval blink in the same phase.
- **Brightness:** hub option `brightness` and action `dfrobot_i2c_rgbbutton.set_brightness`,
  applied to every color (also blinking and the light).
- **Several buttons at once:** `set_color`, `set_brightness`, `lock` and `unlock` accept a list
  of ids.
- **Lock:** actions `dfrobot_i2c_rgbbutton.lock` / `unlock`; while locked, presses are ignored.
- **Conditions:** `dfrobot_i2c_rgbbutton.is_pressed`, `is_locked`, `is_connected`.
- **Connection monitoring:** `binary_sensor` with `type: connected` (diagnostic entity).

### Changed behaviour

- A module that is missing at boot no longer disables the component. It is looked for every
  5 s and set up when it appears (a device with the wrong product ID still fails).
- A module that stops answering (three errors in a row) is reported as lost and looked for every
  5 s; its button reads as released meanwhile. When it is back, its color is written again.
- The color is rewritten every `refresh_interval` (default `60s`), so a module that lost power
  for a moment gets its color back. `refresh_interval: never` disables this.
- Log messages for bus errors changed, see the troubleshooting table in the README.

## v2.0.0

Version 1 configurations keep working. Please read **Changed behaviour** before updating.

### New

- **`light` platform:** the LED as an RGB light. In Home Assistant: color picker,
  brightness, effects (e.g. `pulse`), restore after reboot (`restore_mode`, `initial_state`).
- **`binary_sensor` platform:** the button as its own entity. The `button:` option of the hub
  is still supported.
- **Action `dfrobot_i2c_rgbbutton.set_color`:** set the color without lambdas. Accepts a color
  name (`red`, `off`, …), `0xRRGGBB`, `"#RRGGBB"`, single channels (`red`/`green`/`blue`) or
  lambdas. Short form: `- dfrobot_i2c_rgbbutton.set_color: red`.
- **`interrupt_pin`:** read the button from the module's INT pin. No I2C polling, no missed
  presses.
- **Address check:** `address:` must be one of the DIP switch addresses `0x23`–`0x2A`. If no
  module answers at startup, all eight addresses are scanned and the right one is logged.
- **`dump_config()`:** address, interval, pin, color and entities appear in the log at startup.
- New C++ functions `set_color(0xRRGGBB)` and `set_color(r, g, b)`.

### Changed behaviour

- `update_interval` defaults to `50ms` (was `1s`), so short presses are detected.
- Entities are visible in Home Assistant even without `name:`. Default names: "RGB Button"
  and "RGB Button LED". Use `internal: true` to hide one.
- `address:` outside `0x23`–`0x2A` is rejected.
- `get_button_state()` no longer reads the bus. It returns the state from the last update.
- `get_button_color_rgb()` returns `const uint8_t *`.
- If a light is configured, `set_color` / `set_button_color` go through the light, so Home
  Assistant always shows the current color. `default_color` cannot be combined with a light.
- C++ class renamed to `DFRobotI2CRGBButton` (only matters if lambdas name the type).
- `get_i2c_address()` of the component was removed (it did not work); ESPHome's own
  `get_i2c_address()` returns the configured address.

### Fixed

- Read errors no longer produce random button presses; the last known state is kept.
- A failed color write is retried with the next call instead of being skipped as "unchanged".
- I2C errors show as a warning on the device and clear by themselves when the bus recovers,
  without flooding the log.
- Missing includes that only compiled by accident.

### Project

- License changed to MIT (was a copy of ESPHome's license text). DFRobot's MIT notice added.
- Test configurations in `tests/`, GitHub workflow compiles them for ESP32 (ESP-IDF and
  Arduino) and ESP8266 on every push and weekly against the current ESPHome release.
- README rewritten.

## v1 (2023-09 – 2024-05)

Initial versions: button color via lambdas, button state as inline binary sensor.
