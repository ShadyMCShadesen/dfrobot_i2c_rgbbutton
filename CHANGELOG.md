# Changelog

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
