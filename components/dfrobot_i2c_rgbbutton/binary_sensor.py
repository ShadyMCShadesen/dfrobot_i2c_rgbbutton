import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_TYPE,
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
import esphome.final_validate as fv

from . import (
    CONF_BUTTON,
    CONF_DFROBOT_I2C_RGBBUTTON_ID,
    DEFAULT_BUTTON_NAME,
    DEFAULT_CONNECTED_NAME,
    DFRobotI2CRGBButton,
    with_default_name,
)

DEPENDENCIES = ["dfrobot_i2c_rgbbutton"]

TYPE_BUTTON = "button"
TYPE_CONNECTED = "connected"

_HUB_ID = {
    cv.GenerateID(CONF_DFROBOT_I2C_RGBBUTTON_ID): cv.use_id(DFRobotI2CRGBButton),
}

CONFIG_SCHEMA = cv.typed_schema(
    {
        TYPE_BUTTON: with_default_name(
            DEFAULT_BUTTON_NAME, binary_sensor.binary_sensor_schema().extend(_HUB_ID)
        ),
        TYPE_CONNECTED: with_default_name(
            DEFAULT_CONNECTED_NAME,
            binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ).extend(_HUB_ID),
        ),
    },
    default_type=TYPE_BUTTON,
)


def _final_validate(config):
    hub_id = config[CONF_DFROBOT_I2C_RGBBUTTON_ID]
    type_ = config[CONF_TYPE]
    full = fv.full_config.get()
    count = sum(
        1
        for entry in full.get("binary_sensor", [])
        if entry.get("platform") == "dfrobot_i2c_rgbbutton"
        and entry.get(CONF_DFROBOT_I2C_RGBBUTTON_ID) == hub_id
        and entry.get(CONF_TYPE) == type_
    )
    if type_ == TYPE_BUTTON:
        count += sum(
            1
            for hub in full.get("dfrobot_i2c_rgbbutton", [])
            if hub["id"] == hub_id and CONF_BUTTON in hub
        )
    if count > 1:
        raise cv.Invalid(
            f"Only one '{type_}' binary_sensor per RGB button '{hub_id}'"
            + (
                " (remove the `button:` option of the hub when using the binary_sensor platform)"
                if type_ == TYPE_BUTTON
                else ""
            )
        )
    return config


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config):
    hub = await cg.get_variable(config[CONF_DFROBOT_I2C_RGBBUTTON_ID])
    var = await binary_sensor.new_binary_sensor(config)
    if config[CONF_TYPE] == TYPE_CONNECTED:
        cg.add(hub.set_connected_sensor(var))
    else:
        cg.add(hub.set_button_sensor(var))
