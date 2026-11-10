import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
import esphome.final_validate as fv

from . import (
    CONF_BUTTON,
    CONF_DFROBOT_I2C_RGBBUTTON_ID,
    DEFAULT_BUTTON_NAME,
    DFRobotI2CRGBButton,
    with_default_name,
)

DEPENDENCIES = ["dfrobot_i2c_rgbbutton"]

CONFIG_SCHEMA = with_default_name(
    DEFAULT_BUTTON_NAME,
    binary_sensor.binary_sensor_schema().extend(
        {
            cv.GenerateID(CONF_DFROBOT_I2C_RGBBUTTON_ID): cv.use_id(
                DFRobotI2CRGBButton
            ),
        }
    ),
)


def _final_validate(config):
    hub_id = config[CONF_DFROBOT_I2C_RGBBUTTON_ID]
    full = fv.full_config.get()
    count = sum(
        1
        for entry in full.get("binary_sensor", [])
        if entry.get("platform") == "dfrobot_i2c_rgbbutton"
        and entry.get(CONF_DFROBOT_I2C_RGBBUTTON_ID) == hub_id
    )
    count += sum(
        1
        for hub in full.get("dfrobot_i2c_rgbbutton", [])
        if hub["id"] == hub_id and CONF_BUTTON in hub
    )
    if count > 1:
        raise cv.Invalid(
            f"Only one button binary_sensor per RGB button '{hub_id}' "
            "(remove the `button:` option of the hub when using the binary_sensor platform)"
        )
    return config


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DFROBOT_I2C_RGBBUTTON_ID])
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(hub.set_button_sensor(var))
