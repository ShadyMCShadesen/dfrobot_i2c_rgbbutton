import esphome.codegen as cg
from esphome.components import event
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_BUTTON
import esphome.final_validate as fv

from . import (
    CONF_DFROBOT_I2C_RGBBUTTON_ID,
    DEFAULT_EVENT_NAME,
    DFRobotI2CRGBButton,
    with_default_name,
)

DEPENDENCIES = ["dfrobot_i2c_rgbbutton"]

EVENT_TYPES = ["click", "double_click", "triple_click", "long_press"]

CONFIG_SCHEMA = with_default_name(
    DEFAULT_EVENT_NAME,
    event.event_schema(event.Event, device_class=DEVICE_CLASS_BUTTON).extend(
        {
            cv.GenerateID(CONF_DFROBOT_I2C_RGBBUTTON_ID): cv.use_id(
                DFRobotI2CRGBButton
            ),
        }
    ),
)


def _final_validate(config):
    hub_id = config[CONF_DFROBOT_I2C_RGBBUTTON_ID]
    count = sum(
        1
        for entry in fv.full_config.get().get("event", [])
        if entry.get("platform") == "dfrobot_i2c_rgbbutton"
        and entry.get(CONF_DFROBOT_I2C_RGBBUTTON_ID) == hub_id
    )
    if count > 1:
        raise cv.Invalid(f"Only one event per RGB button '{hub_id}'")
    return config


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DFROBOT_I2C_RGBBUTTON_ID])
    var = await event.new_event(config, event_types=EVENT_TYPES)
    cg.add(hub.set_click_event(var))
    cg.add(hub.set_click_detection(True))
    cg.add(hub.set_multi_click(True))
