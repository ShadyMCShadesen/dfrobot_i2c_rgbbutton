import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, i2c
from esphome.const import (
    CONF_ID
)

from esphome import automation

MULTI_CONF = True

AUTO_LOAD = [ "binary_sensor" ]

DEPENDENCIES = ['i2c']

dfrobot_i2c_button_ns = cg.esphome_ns.namespace('dfrobot_i2c_rgbbutton')
DFRobot_i2c_RGBButton = dfrobot_i2c_button_ns.class_('DFRobot_i2c_RGBButton', cg.PollingComponent)

CONF_COLOR = "color"
CONF_ADDRESS = "address"
CONF_WIRE = "wire"
CONF_BUTTON = "button"

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(DFRobot_i2c_RGBButton),
    cv.Optional(CONF_COLOR, default='0xFFFFFF'): cv.int_range(min=0x000000, max=0xFFFFFF),
    cv.Optional(CONF_ADDRESS, default='0x23'): cv.int_range(min=0x23, max=0x2A),
    cv.Optional(CONF_WIRE, default='false'): cv.boolean,
    cv.Optional(CONF_BUTTON): binary_sensor.binary_sensor_schema(),
}).extend(cv.polling_component_schema('1s'))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_COLOR in config:
        cg.add(var.set_color(config[CONF_COLOR]))
    if (CONF_ADDRESS in config) or (CONF_WIRE in config):
        cg.add(var.set_button_address(config[CONF_ADDRESS]))
        cg.add(var.set_button_wire(config[CONF_WIRE]))

    if CONF_BUTTON in config:
        button = await binary_sensor.new_binary_sensor(config[CONF_BUTTON])
        cg.add(var.set_button_sensor(button))