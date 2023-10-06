# dfrobot_i2c_rgbbutton
External component for ESPHome supporting the [DFRobot I2C RGB Button](https://wiki.dfrobot.com/SKU_DFR0991_Gravity_I2C_RGB_LED_Button_Module).

# Compatibility

I have tested the code on ESP32, it works fine.

# Functionality

You can interact with the button via functions:

```yaml
set_button_color(uint8_t r, uint8_t g, uint8_t b, bool force = false)
```
to set the color of the button (r, g, b; for example `(0, 0, 0)` for black/off)


```yaml
set_button_color(long color, bool force = false)
```
to set the color of the button (hex; for example `(0xFFFFFF)` for white)

Alternatively, hardcoded color variables can also be used:
```cpp
eRed    = 0xFF0000,   /**< Red */
eOrange = 0xFF7F00,   /**< Orange */
eYellow = 0xFFFF00,   /**< Yellow */
eGreen  = 0x00FF00,   /**< Green */
eCyan   = 0x00FFFF,   /**< Indigo */
eBlue   = 0x0000FF,   /**< Blue */
ePurple = 0x8B00FF,   /**< Purple */
eWhite  = 0xFFFFFF,   /**< White (for turning on LED) */
eBlack  = 0x000000,   /**< Black (for turning off LED) */
```
for example: `id({button_id}).set_button_color({button_id}->eRed);`


```yaml
get_button_color_rgb()
```
to get the color of the button (uint8_t[3]; for example `{255, 255, 255}` for white)


```yaml
get_button_color_hex()
```
to get the color of the button (unsigned long; for example `0xFFFFFF` for white)


```yaml
get_button_state()
```
to get the current state of the button (boolean; for example `true` if pressed)

# YAML

Example:
```yaml
# Import external_component from github
external_components:
  - source:
      type: git
      url: https://github.com/ShadyMCShadesen/dfrobot_i2c_rgbbutton
    components: [ dfrobot_i2c_rgbbutton ]

# Setup i2c bus
i2c:    
  - id: bus_a
    sda: GPIO21
    scl: GPIO22

# Setup DFRobot I2C RGBButton
dfrobot_i2c_rgbbutton:
  - id: i2c_rgbbutton
    address: 0x23
    i2c_id: bus_a
    default_color: 0x000000
    update_interval: 1s
    button: # binary_sensor configuration, see https://esphome.io/components/binary_sensor/index.html
      id: i2c_rgbbutton_sensor
      on_press: # set button color to red while pressed
        then:
          - lambda: id(i2c_rgbbutton).set_button_color(0xFF0000);
      on_release: # set button color back to 'off' if released
        then:
          - lambda: id(i2c_rgbbutton).set_button_color(i2c_rgbbutton->eBlack);
```

# Optional parameters

`address:` i2c address of the button module, default 0x23

`i2c_id:` i2c bus of the button module, default first of configured busses

`default_color:` default color of the button, default 0x000000 (black / off)

`update_interval:` interval for the state update of the button, default 1s (for better button state detection, I recommend a shorter interval)

`button:` binary_sensor configuration representing the button state [ESPHome](https://esphome.io/components/binary_sensor/index.html)
