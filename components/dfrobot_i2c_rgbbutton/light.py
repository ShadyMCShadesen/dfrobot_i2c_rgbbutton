import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_GAMMA_CORRECT, CONF_ID, CONF_OUTPUT_ID
import esphome.final_validate as fv

from . import (
    CONF_DEFAULT_COLOR,
    CONF_DFROBOT_I2C_RGBBUTTON_ID,
    DEFAULT_LIGHT_NAME,
    DFRobotI2CRGBButton,
    dfrobot_i2c_rgbbutton_ns,
    with_default_name,
)

DEPENDENCIES = ["dfrobot_i2c_rgbbutton"]

DFRobotI2CRGBButtonLight = dfrobot_i2c_rgbbutton_ns.class_(
    "DFRobotI2CRGBButtonLight", light.LightOutput
)

CONFIG_SCHEMA = with_default_name(
    DEFAULT_LIGHT_NAME,
    light.light_schema(DFRobotI2CRGBButtonLight, light.LightType.RGB).extend(
        {
            cv.GenerateID(CONF_DFROBOT_I2C_RGBBUTTON_ID): cv.use_id(
                DFRobotI2CRGBButton
            ),
            # 1.0: the module receives exactly the configured / set_color values.
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
        }
    ),
)


def _final_validate(config):
    hub_id = config[CONF_DFROBOT_I2C_RGBBUTTON_ID]
    full = fv.full_config.get()
    for hub in full.get("dfrobot_i2c_rgbbutton", []):
        if hub["id"] == hub_id and CONF_DEFAULT_COLOR in hub:
            raise cv.Invalid(
                f"RGB button '{hub_id}' has a light: remove `default_color` from the hub "
                "and use the light's `restore_mode` / `initial_state` instead"
            )
    count = sum(
        1
        for entry in full.get("light", [])
        if entry.get("platform") == "dfrobot_i2c_rgbbutton"
        and entry.get(CONF_DFROBOT_I2C_RGBBUTTON_ID) == hub_id
    )
    if count > 1:
        raise cv.Invalid(f"Only one light per RGB button '{hub_id}'")
    return config


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DFROBOT_I2C_RGBBUTTON_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    cg.add(var.set_parent(hub))
    await light.register_light(var, config)
    state = await cg.get_variable(config[CONF_ID])
    cg.add(hub.set_light_state(state))
