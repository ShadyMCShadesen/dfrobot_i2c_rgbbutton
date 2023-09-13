# DFRobot_I2C_RGBButton (Work in progress)
ESPHome custom integration for the [DFRobot I2C RGBButton](https://wiki.dfrobot.com/SKU_DFR0991_Gravity_I2C_RGB_LED_Button_Module).

# !! Currently not working !!

**_ESPHome does not recognize this custom component as a component :(_**

# Functionality

You can interact with the button via functions:

`set_button_color(uint8_t r, uint8_t g, uint8_t b)` to set the color of the button (r, g, b)

`get_button_color()` to get the color of the button (not implemented yet)

`get_button_state()` to get the current state of the button (boolean)

Example:
```yaml
esphome:
  libraries:
    - Wire
    - "https://github.com/DFRobot/DFRobot_RGBButton"

i2c:    
  sda: D0
  scl: D1

binary_sensor:
  - platform: dfrobot_i2c_button
    id: button_1
    address: 0x2A
    wire: false
    lambda: |-
      return;
```

# Optional parameters

`address:` i2c address of the button module, default 0x23

`wire:` true if the button is connected on a secondary i2c bus, default false

`lambda:` place to code button behavior, for example button color change on press
