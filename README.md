# dfrobot_i2c_rgbbutton (Work in progress)
ESPHome external component for the [DFRobot I2C RGBButton](https://wiki.dfrobot.com/SKU_DFR0991_Gravity_I2C_RGB_LED_Button_Module).

# !! Currently not tested !!

**_The buttons are ordered, I will test it soon (09.2023)_**

# Functionality

You can interact with the button via functions:

```yaml
set_button_color(uint8_t r, uint8_t g, uint8_t b)
```
to set the color of the button (r, g, b; for example `(0, 0, 0)` for black/off)

```yaml
set_button_color(long color)
```
to set the color of the button (hex; for example `(0xFFFFFF)` for white)

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
      ref: main
    components: [ dfrobot_i2c_rgbbutton ]

# Import original library for the DFRobot_RGBButton from github
esphome:
  libraries:
    - "https://github.com/DFRobot/DFRobot_RGBButton"

# Setup i2c bus
i2c:    
  - id: bus_a # if your device is connected to the first i2c bus (or only one bus exists), 'secondary_bus' needs to be false
    sda: GPIO21
    scl: GPIO22
  - id: bus_b # if your device is connected to a secondary i2c bus, 'secondary_bus' needs to be true
    sda: GPIO16
    scl: GPIO17

dfrobot_i2c_rgbbutton:
  - id: i2c_rgbbutton
    address: 0x23
    secondary_bus: false
    default_color: 0xFFFFFF
    update_interval: 1s
    button: # binary_sensor configuration, see https://esphome.io/components/binary_sensor/index.html
      id: i2c_rgbbutton_sensor
      on_press: # set button color to red while pressed
        then:
          - lambda: id(i2c_rgbbutton).set_button_color(0xFF0000);
      on_release: # set button color back to white if released
        then:
          - lambda: id(i2c_rgbbutton).set_button_color(0xFFFFFF);
```

# Optional parameters

`address:` i2c address of the button module, default 0x23

`secondary_bus:` true if the button is connected on a secondary i2c bus, default false

`default_color:` default color of the button, default 0xFFFFFF (white)

`update_interval:` interval for the state update of the button, default 1s

`button:` binary_sensor configuration representing the button state [ESPHome](https://esphome.io/components/binary_sensor/index.html)
