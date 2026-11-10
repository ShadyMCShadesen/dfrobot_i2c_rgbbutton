# dfrobot_i2c_rgbbutton

ESPHome external component for the
[DFRobot Gravity I2C RGB LED Button (DFR0991)](https://wiki.dfrobot.com/SKU_DFR0991_Gravity_I2C_RGB_LED_Button_Module).

- Button as a **binary sensor** (polled over I2C, or instantly via the module's INT pin)
- LED as an **RGB light** (color picker in Home Assistant, brightness, effects, restore after reboot)
- Action **`dfrobot_i2c_rgbbutton.set_color`** for automations, no lambdas needed
- Several buttons on one bus (address via DIP switch, checked at compile time)
- Clear log messages: wrong address, wrong device and I2C errors are reported; if the module
  is not found, all possible addresses are scanned and the right one is logged

Works with ESP32 (ESP-IDF and Arduino) and ESP8266. Compiled in CI against the current ESPHome
release.

## Quick start

```yaml
external_components:
  - source: github://ShadyMCShadesen/dfrobot_i2c_rgbbutton
    components: [dfrobot_i2c_rgbbutton]

i2c:
  sda: GPIO21
  scl: GPIO22

dfrobot_i2c_rgbbutton:
  - id: rgb_button

binary_sensor:
  - platform: dfrobot_i2c_rgbbutton
    on_press:
      - dfrobot_i2c_rgbbutton.set_color: red
    on_release:
      - dfrobot_i2c_rgbbutton.set_color: off

light:
  - platform: dfrobot_i2c_rgbbutton
```

This creates the entities "RGB Button" (binary sensor) and "RGB Button LED" (light). Pin a
release with `github://ShadyMCShadesen/dfrobot_i2c_rgbbutton@v2.0.0` so that updates of this
repository do not change your device unexpectedly.

## Hub: `dfrobot_i2c_rgbbutton`

One entry per module.

| Option | Default | Description |
|---|---|---|
| `id` | generated | ID used by the platforms and actions |
| `address` | `0x23` | I2C address, `0x23`–`0x2A` (see [Address](#address)) |
| `i2c_id` | first bus | I2C bus |
| `update_interval` | `50ms` | How often the button is read over I2C. Short presses can be missed with long intervals. Not used with `interrupt_pin`. |
| `interrupt_pin` | – | GPIO connected to the module's INT pin. INT is high while the button is pressed, so the state is read from the pin in every loop (no I2C polling, no missed presses). |
| `default_color` | `off` | Color written at boot. Ignored when a `light` is configured (use the light's `restore_mode` / `initial_state` instead). |
| `button` | – | *Legacy:* binary sensor configured inside the hub, as in version 1. Still works; prefer the `binary_sensor` platform. |

### Address

The address is selected with the three-position DIP switch on the back of the module. It cannot
be changed by software.

| DIP switch | 000 | 001 | 010 | 011 | 100 | 101 | 110 | 111 |
|---|---|---|---|---|---|---|---|---|
| Address | 0x23 | 0x24 | 0x25 | 0x26 | 0x27 | 0x28 | 0x29 | 0x2A |

Other addresses are rejected by the config validation. If no module answers at the configured
address, the component scans `0x23`–`0x2A` and logs where it found one, e.g.
`Found an RGB button at address 0x25 (DIP switch 010)`.

## Binary sensor platform

```yaml
binary_sensor:
  - platform: dfrobot_i2c_rgbbutton
    dfrobot_i2c_rgbbutton_id: rgb_button   # only needed with several buttons
    name: Doorbell                         # default: "RGB Button"
    filters:
      - delayed_off: 20ms                  # optional debounce
```

All [binary sensor options](https://esphome.io/components/binary_sensor/) are supported. The
entity is visible in Home Assistant even without `name:`; set `internal: true` to hide it.
Only one binary sensor per module.

## Light platform

```yaml
light:
  - platform: dfrobot_i2c_rgbbutton
    dfrobot_i2c_rgbbutton_id: rgb_button   # only needed with several buttons
    name: Doorbell LED                     # default: "RGB Button LED"
    restore_mode: RESTORE_DEFAULT_OFF
    effects:
      - pulse:
```

All [light options](https://esphome.io/components/light/) are supported. `gamma_correct`
defaults to `1.0` here (ESPHome's default is 2.8), so the module receives exactly the color
values you set; raise it if brightness steps look uneven. Only one light per module.

With a light configured, `set_color` (action and C++) goes through the light, so Home Assistant
always shows the current color. Without a light, the color is written directly.

## Action: `dfrobot_i2c_rgbbutton.set_color`

```yaml
# Color name, hex value or "#RRGGBB" (quote it, '#' starts a YAML comment)
- dfrobot_i2c_rgbbutton.set_color: red

- dfrobot_i2c_rgbbutton.set_color:
    id: rgb_button
    color: 0xFF7F00

# Single channels 0-255; missing channels are 0
- dfrobot_i2c_rgbbutton.set_color:
    id: rgb_button
    red: 255
    blue: !lambda "return id(some_sensor).state > 20 ? 255 : 0;"

# Lambdas return 0xRRGGBB
- dfrobot_i2c_rgbbutton.set_color:
    id: rgb_button
    color: !lambda "return id(alarm).state ? 0xFF0000 : 0x00FF00;"
```

`id` can be omitted if there is only one module. Color names: `red`, `orange`, `yellow`, `green`,
`cyan`, `blue`, `purple`, `white`, `black` / `off`.

## Lambdas (C++ API)

```cpp
id(rgb_button).set_color(0xFF0000);          // 0xRRGGBB
id(rgb_button).set_color(255, 127, 0);       // r, g, b
id(rgb_button).get_button_state();           // last known state, true = pressed
id(rgb_button).get_button_color_hex();       // last written color, 0xRRGGBB
id(rgb_button).get_button_color_rgb();       // const uint8_t[3] {r, g, b}
```

The functions of version 1 still work: `set_button_color(r, g, b, force)`,
`set_button_color(0xRRGGBB, force)` and the color constants such as
`id(rgb_button).eRed`. `get_button_state()` no longer accesses the bus; it returns the state
read in the last update.

## Several buttons

```yaml
dfrobot_i2c_rgbbutton:
  - id: button_left
    address: 0x23
  - id: button_right
    address: 0x24

binary_sensor:
  - platform: dfrobot_i2c_rgbbutton
    dfrobot_i2c_rgbbutton_id: button_left
    name: Left button
  - platform: dfrobot_i2c_rgbbutton
    dfrobot_i2c_rgbbutton_id: button_right
    name: Right button
```

Give each entity its own `name:` (ESPHome does not allow two entities with the same name).

## Upgrading from version 1

Version 1 configurations keep working. Changes in behaviour:

- `update_interval` defaults to `50ms` instead of `1s`.
- The inline `button:` entity is now visible in Home Assistant even without `name:`
  (default name "RGB Button").
- `address` must be one of the DIP switch addresses `0x23`–`0x2A`.
- To disable polling, use `update_interval: never`. `0s` does not disable it; ESPHome then polls
  about 1000 times per second.

## Troubleshooting

| Log message | Cause |
|---|---|
| `No device responds at address 0x..` | Wiring, power, or wrong address. The scan line below it shows the address the module actually uses. |
| `Device at 0x.. has product ID ..` | Another I2C device uses this address. |
| `Reading the button state failed` / `Writing color .. failed` | Bus errors (long cables, missing pull-ups). The component keeps running and recovers by itself. |

## Development

`tests/` contains configurations that are checked and compiled by the GitHub workflow:

```bash
esphome compile tests/test.esp32-idf.yaml
```

## Credits and license

Register map and product ID are taken from DFRobot's
[DFRobot_RGBButton](https://github.com/DFRobot/DFRobot_RGBButton) library
(Copyright 2010 DFRobot Co.Ltd, MIT License). See [LICENSE](LICENSE) for this component.
