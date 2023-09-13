import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, i2c
from esphome.const import CONF_ID, CONF_LAMBDA

DEPENDENCIES = ['i2c']

dfrobot_i2c_button_ns = cg.esphome_ns.namespace('dfrobot_i2c_button')
DFRobot_i2c_RGBButton = dfrobot_i2c_button_ns.class_('DFRobot_i2c_RGBButton', binary_sensor.BinarySensor, cg.PollingComponent)

CONF_COLOR = "color"
CONF_ADDRESS = "address"
CONF_WIRE = "wire"

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(DFRobot_i2c_RGBButton),
    cv.Optional(CONF_COLOR, default='eWhite'): cv.string,
    cv.Optional(CONF_ADDRESS, default='0x2A'): cv.int_range(min=0x23, max=0x2A),
    cv.Optional(CONF_WIRE, default='false'): cv.boolean,
}).extend(cv.polling_component_schema('1s'))

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield binary_sensor.register_binary_sensor(var, config)

    if CONF_LAMBDA in config:
        lambda_ = yield cg.process_lambda(config[CONF_LAMBDA],
                                          [(DFRobot_i2c_RGBButton.operator('ref'), 'it')],
                                          return_type=cg.void)
        cg.add(var.set_writer(lambda_))
    if config[CONF_COLOR]:
        cg.add(var.set_color(config[CONF_COLOR]))
    if config[CONF_ADDRESS] or config[CONF_WIRE]:
        cg.add(var.set_button_address(config[CONF_ADDRESS]))
        cg.add(var.set_button_wire(config[CONF_WIRE]))