# dfrobot_i2c_rgbbutton

ESPHome external component for the
[DFRobot Gravity I2C RGB LED Button (DFR0991)](https://wiki.dfrobot.com/SKU_DFR0991_Gravity_I2C_RGB_LED_Button_Module).

- Button as a **binary sensor** (polled over I2C, or instantly via the module's INT pin)
- LED as an **RGB light** (color picker in Home Assistant, brightness, effects, restore after reboot)
- **Clicks:** `on_click`, `on_double_click`, `on_triple_click`, `on_long_press` and an **event**
  entity for Home Assistant
- Actions **`set_color`** (also blinking, several buttons at once), **`set_brightness`**,
  **`lock`** / **`unlock`**, no lambdas needed
- **Blinking** in the component: all buttons blink in the same phase
- **Robust:** a module that is missing at boot, unplugged or loses power is found again and gets
  its color back; optional **connected** sensor per module
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
release with `github://ShadyMCShadesen/dfrobot_i2c_rgbbutton@v2.1.0` so that updates of this
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
| `default_color` | `off` | Color written at boot. Not allowed together with a `light`; use the light's `restore_mode` / `initial_state` there. |
| `brightness` | `100%` | Brightness of the LED, applied to every color. Can be changed with [`set_brightness`](#action-dfrobot_i2c_rgbbuttonset_brightness). |
| `blink_interval` | `500ms` | Duration of each blink phase. Buttons with the same interval blink in the same phase. |
| `refresh_interval` | `60s` | The color is written again at this interval, in case the module lost power for a moment. `never` disables it. |
| `long_press_time` | `1s` | Holding the button this long is a long press (reported while still held). |
| `multi_click_time` | `400ms` | Maximum pause between the clicks of a double or triple click. |
| `on_click` / `on_double_click` / `on_triple_click` / `on_long_press` | – | Automations, see [Clicks](#clicks). |
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

### Connection loss

If the module does not answer three times in a row (or is missing at boot), it is reported as
lost: the device shows a warning, the button reads as released, and the module is looked for
every 5 s. When it answers again, its color is written again and everything continues. A module
that lost power only for a moment comes back dark; `refresh_interval` rewrites the color, so it is
back after at most that long. With `interrupt_pin`, a lost module is noticed at the next color
write or refresh.

## Clicks

```yaml
dfrobot_i2c_rgbbutton:
  - id: rgb_button
    on_click:
      - light.toggle: hall_light
    on_double_click:
      - dfrobot_i2c_rgbbutton.set_color: blue
    on_long_press:
      - dfrobot_i2c_rgbbutton.set_color: off
```

A click is reported after the release. If `on_double_click`, `on_triple_click` or an
[event](#event-platform) is configured, the component waits `multi_click_time` for further
clicks first; otherwise a click is reported immediately. A long press is reported as soon as
the button was held for `long_press_time`, without a click on release. More than three clicks
count as a triple click. For other patterns, use `on_multi_click` of the binary sensor.

## Binary sensor platform

```yaml
binary_sensor:
  - platform: dfrobot_i2c_rgbbutton
    dfrobot_i2c_rgbbutton_id: rgb_button   # only needed with several buttons
    name: Doorbell                         # default: "RGB Button"
    filters:
      - delayed_off: 20ms                  # optional debounce

  - platform: dfrobot_i2c_rgbbutton
    type: connected                        # default: button
    name: Doorbell connected               # default: "RGB Button Connected"
```

All [binary sensor options](https://esphome.io/components/binary_sensor/) are supported. The
entity is visible in Home Assistant even without `name:`; set `internal: true` to hide it.

| `type` | State |
|---|---|
| `button` (default) | `on` while the button is pressed (always `off` while [locked](#actions-dfrobot_i2c_rgbbuttonlock--unlock)) |
| `connected` | `on` while the module answers on the bus (diagnostic entity) |

One binary sensor of each type per module.

## Event platform

```yaml
event:
  - platform: dfrobot_i2c_rgbbutton
    dfrobot_i2c_rgbbutton_id: rgb_button   # only needed with several buttons
    name: Doorbell click                   # default: "RGB Button Click"
```

An [event entity](https://esphome.io/components/event/) with the event types `click`,
`double_click`, `triple_click` and `long_press`. In Home Assistant, one automation can react to
all buttons and all click types. One event per module.

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

```yaml
# Several buttons at once
- dfrobot_i2c_rgbbutton.set_color:
    id: [button_left, button_right]
    color: green

# Blink between the color and off ...
- dfrobot_i2c_rgbbutton.set_color:
    id: rgb_button
    color: orange
    blink: true

# ... or between two colors (alternate_color implies blink: true)
- dfrobot_i2c_rgbbutton.set_color:
    id: rgb_button
    color: red
    alternate_color: blue
```

`id` can be omitted if there is only one module. Color names: `red`, `orange`, `yellow`, `green`,
`cyan`, `blue`, `purple`, `white`, `black` / `off`.

Blinking uses the hub's `blink_interval`. All buttons with the same interval blink in the same
phase: `color` in the first half, `alternate_color` in the second. For opposite phases, swap the
two colors. A `set_color` without `blink` ends blinking. With a light configured, blinking is not
shown by the light entity, and any change of the light ends it.

## Action: `dfrobot_i2c_rgbbutton.set_brightness`

```yaml
- dfrobot_i2c_rgbbutton.set_brightness: 30%     # only button

- dfrobot_i2c_rgbbutton.set_brightness:
    id: [button_left, button_right]
    brightness: !lambda "return id(night_mode).state ? 0.2 : 1.0;"
```

Scales every color of the module (0–100 %), including blinking and the light. Starts with the
hub's `brightness` option.

## Actions: `dfrobot_i2c_rgbbutton.lock` / `unlock`

```yaml
- dfrobot_i2c_rgbbutton.lock: rgb_button
- dfrobot_i2c_rgbbutton.unlock: [button_left, button_right]
```

While locked, presses are ignored: the button binary sensor stays `off` and no clicks are
reported. The LED keeps working. Locking is not stored and ends with a reboot.

## Conditions

```yaml
condition:
  dfrobot_i2c_rgbbutton.is_pressed: rgb_button     # button held right now
# also: dfrobot_i2c_rgbbutton.is_locked, dfrobot_i2c_rgbbutton.is_connected
```

## Lambdas (C++ API)

```cpp
id(rgb_button).set_color(0xFF0000);          // 0xRRGGBB, ends blinking
id(rgb_button).set_color(255, 127, 0);       // r, g, b
id(rgb_button).blink(0xFF0000, 0x000000);    // blink between two colors
id(rgb_button).is_blinking();
id(rgb_button).set_brightness(0.5f);         // 0.0 - 1.0
id(rgb_button).get_brightness();
id(rgb_button).lock();                       // ignore presses
id(rgb_button).unlock();
id(rgb_button).is_locked();
id(rgb_button).is_connected();               // module answers on the bus
id(rgb_button).get_button_state();           // last known state, true = pressed
id(rgb_button).get_button_color_hex();       // color set last (before brightness), 0xRRGGBB
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
| `I2C communication with 0x.. failed` | A single bus error (long cables, missing pull-ups). Harmless if it is rare. |
| `Module at 0x.. lost (..), looking for it every 5 s` | No answer three times in a row: unplugged, no power, cable. `Module at 0x.. connected` follows when it is back. |

## Development

`tests/` contains configurations that are checked and compiled by the GitHub workflow:

```bash
esphome compile tests/test.esp32-idf.yaml
```

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## Credits and license

MIT License, see [LICENSE](LICENSE).

Register map, product ID and predefined colors are taken from DFRobot's
[DFRobot_RGBButton](https://github.com/DFRobot/DFRobot_RGBButton) library
(Copyright 2010 DFRobot Co.Ltd, MIT License; notice included in [LICENSE](LICENSE)).
