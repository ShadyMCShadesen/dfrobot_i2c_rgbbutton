CODEOWNERS = ["@ShadyMCShadesen"]

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, i2c
from esphome.const import (
    CONF_ID
)

DEPENDENCIES = ['i2c']
AUTO_LOAD = [ "binary_sensor" ]
MULTI_CONF = True

dfrobot_i2c_button_ns = cg.esphome_ns.namespace('dfrobot_i2c_rgbbutton')
DFRobot_i2c_RGBButton = dfrobot_i2c_button_ns.class_('DFRobot_i2c_RGBButton', i2c.I2CDevice, cg.PollingComponent)

CONF_DEFAULT_COLOR = "default_color"
CONF_BUTTON = "button"

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(DFRobot_i2c_RGBButton),
    cv.Optional(CONF_DEFAULT_COLOR, default='0x000000'): cv.int_range(min=0x000000, max=0xFFFFFF),
    cv.Optional(CONF_BUTTON): binary_sensor.binary_sensor_schema(),
}).extend(i2c.i2c_device_schema(0x23)).extend(cv.polling_component_schema('1s'))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_DEFAULT_COLOR in config:
        cg.add(var.set_default_color(config[CONF_DEFAULT_COLOR]))

    if CONF_BUTTON in config:
        button = await binary_sensor.new_binary_sensor(config[CONF_BUTTON])
        cg.add(var.set_button_sensor(button))
