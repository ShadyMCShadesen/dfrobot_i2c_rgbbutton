import re

from esphome import automation, pins
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_BLUE,
    CONF_COLOR,
    CONF_GREEN,
    CONF_ID,
    CONF_INTERRUPT_PIN,
    CONF_NAME,
    CONF_RED,
)

CODEOWNERS = ["@ShadyMCShadesen"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["binary_sensor"]  # for the inline `button:` option
MULTI_CONF = True

CONF_BUTTON = "button"
CONF_DEFAULT_COLOR = "default_color"
CONF_DFROBOT_I2C_RGBBUTTON_ID = "dfrobot_i2c_rgbbutton_id"

DEFAULT_BUTTON_NAME = "RGB Button"
DEFAULT_LIGHT_NAME = "RGB Button LED"

# Addresses selectable with the DIP switch on the module (0x23 + switch value)
FIRST_ADDRESS = 0x23
LAST_ADDRESS = 0x2A

dfrobot_i2c_rgbbutton_ns = cg.esphome_ns.namespace("dfrobot_i2c_rgbbutton")
DFRobotI2CRGBButton = dfrobot_i2c_rgbbutton_ns.class_(
    "DFRobotI2CRGBButton", cg.PollingComponent, i2c.I2CDevice
)

COLOR_NAMES = {
    "red": 0xFF0000,
    "orange": 0xFF7F00,
    "yellow": 0xFFFF00,
    "green": 0x00FF00,
    "cyan": 0x00FFFF,
    "blue": 0x0000FF,
    "purple": 0x8B00FF,
    "white": 0xFFFFFF,
    "black": 0x000000,
    "off": 0x000000,
}


def color_value(value):
    """A color as 0xRRGGBB, "#RRGGBB" or one of COLOR_NAMES."""
    if value is False:  # a bare `off` is parsed as a boolean by YAML
        return 0
    if isinstance(value, str):
        text = value.strip().lower()
        if text in COLOR_NAMES:
            return COLOR_NAMES[text]
        match = re.fullmatch(r"(?:#|0x)([0-9a-f]{6})", text)
        if match is None:
            raise cv.Invalid(
                f"Invalid color '{value}'. Use 0xRRGGBB, \"#RRGGBB\" or one of: "
                + ", ".join(COLOR_NAMES)
            )
        return int(match.group(1), 16)
    if isinstance(value, bool):
        raise cv.Invalid("Expected a color, not a boolean")
    return cv.int_range(min=0, max=0xFFFFFF)(value)


def validate_address(config):
    address = config[CONF_ADDRESS]
    if not FIRST_ADDRESS <= address <= LAST_ADDRESS:
        table = ", ".join(
            f"{value:03b}=0x{FIRST_ADDRESS + value:02X}" for value in range(8)
        )
        raise cv.Invalid(
            f"The RGB button only supports the addresses 0x{FIRST_ADDRESS:02X}-"
            f"0x{LAST_ADDRESS:02X}, selected with its DIP switch ({table})",
            path=[CONF_ADDRESS],
        )
    return config


def with_default_name(name, schema):
    """Entities get a name (and are therefore visible) even if only an id is set.

    ESPHome normally makes entities without `name:` internal; `internal: true` still does that.
    """

    def set_default(config):
        if isinstance(config, dict) and CONF_NAME not in config:
            config = {**config, CONF_NAME: name}
        return config

    return cv.All(set_default, schema)


BUTTON_SCHEMA = with_default_name(
    DEFAULT_BUTTON_NAME, binary_sensor.binary_sensor_schema()
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DFRobotI2CRGBButton),
            cv.Optional(CONF_DEFAULT_COLOR, default=0): color_value,
            cv.Optional(CONF_INTERRUPT_PIN): pins.gpio_input_pin_schema,
            # Kept for configurations of earlier versions; prefer the binary_sensor platform.
            cv.Optional(CONF_BUTTON): BUTTON_SCHEMA,
        }
    )
    .extend(cv.polling_component_schema("50ms"))
    .extend(i2c.i2c_device_schema(FIRST_ADDRESS)),
    validate_address,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_default_color(config[CONF_DEFAULT_COLOR]))

    if CONF_INTERRUPT_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(pin))

    if CONF_BUTTON in config:
        button = await binary_sensor.new_binary_sensor(config[CONF_BUTTON])
        cg.add(var.set_button_sensor(button))


def _fill_rgb(config):
    has_rgb = any(key in config for key in (CONF_RED, CONF_GREEN, CONF_BLUE))
    if has_rgb == (CONF_COLOR in config):
        raise cv.Invalid("Set either 'color' or 'red'/'green'/'blue'")
    if has_rgb:
        for key in (CONF_RED, CONF_GREEN, CONF_BLUE):
            config.setdefault(key, 0)
    return config


_CHANNEL = cv.templatable(cv.int_range(min=0, max=255))

_SET_COLOR_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(DFRobotI2CRGBButton),
        cv.Optional(CONF_COLOR): cv.templatable(color_value),
        cv.Optional(CONF_RED): _CHANNEL,
        cv.Optional(CONF_GREEN): _CHANNEL,
        cv.Optional(CONF_BLUE): _CHANNEL,
    }
)


def _set_color_shorthand(value):
    # `dfrobot_i2c_rgbbutton.set_color: red` is short for `color: red`.
    if isinstance(value, dict):
        return value
    return {CONF_COLOR: value}


SET_COLOR_ACTION_SCHEMA = cv.All(_set_color_shorthand, _SET_COLOR_SCHEMA, _fill_rgb)

automation.register_apply_action(
    "dfrobot_i2c_rgbbutton.set_color",
    SET_COLOR_ACTION_SCHEMA,
    automation.ApplyField(CONF_COLOR, "set_color", cg.uint32),
    automation.ApplyCall(
        "set_color({}, {}, {})",
        ((CONF_RED, cg.uint8), (CONF_GREEN, cg.uint8), (CONF_BLUE, cg.uint8)),
    ),
)
