import re

from esphome import automation, pins
import esphome.codegen as cg
from esphome.components import binary_sensor, i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_BLUE,
    CONF_BRIGHTNESS,
    CONF_COLOR,
    CONF_GREEN,
    CONF_ID,
    CONF_INTERRUPT_PIN,
    CONF_NAME,
    CONF_ON_CLICK,
    CONF_ON_DOUBLE_CLICK,
    CONF_RED,
)

CODEOWNERS = ["@ShadyMCShadesen"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["binary_sensor"]  # for the inline `button:` option
MULTI_CONF = True

CONF_ALTERNATE_COLOR = "alternate_color"
CONF_BLINK = "blink"
CONF_BLINK_INTERVAL = "blink_interval"
CONF_BUTTON = "button"
CONF_DEFAULT_COLOR = "default_color"
CONF_DFROBOT_I2C_RGBBUTTON_ID = "dfrobot_i2c_rgbbutton_id"
CONF_LONG_PRESS_TIME = "long_press_time"
CONF_MULTI_CLICK_TIME = "multi_click_time"
CONF_ON_LONG_PRESS = "on_long_press"
CONF_ON_TRIPLE_CLICK = "on_triple_click"
CONF_REFRESH_INTERVAL = "refresh_interval"

DEFAULT_BUTTON_NAME = "RGB Button"
DEFAULT_CONNECTED_NAME = "RGB Button Connected"
DEFAULT_EVENT_NAME = "RGB Button Click"
DEFAULT_LIGHT_NAME = "RGB Button LED"

# Addresses selectable with the DIP switch on the module (0x23 + switch value)
FIRST_ADDRESS = 0x23
LAST_ADDRESS = 0x2A

dfrobot_i2c_rgbbutton_ns = cg.esphome_ns.namespace("dfrobot_i2c_rgbbutton")
DFRobotI2CRGBButton = dfrobot_i2c_rgbbutton_ns.class_(
    "DFRobotI2CRGBButton", cg.PollingComponent, i2c.I2CDevice
)
SetColorAction = dfrobot_i2c_rgbbutton_ns.class_("SetColorAction", automation.Action)
SetBrightnessAction = dfrobot_i2c_rgbbutton_ns.class_(
    "SetBrightnessAction", automation.Action
)
LockAction = dfrobot_i2c_rgbbutton_ns.class_("LockAction", automation.Action)
UnlockAction = dfrobot_i2c_rgbbutton_ns.class_("UnlockAction", automation.Action)
IsPressedCondition = dfrobot_i2c_rgbbutton_ns.class_(
    "IsPressedCondition", automation.Condition
)
IsLockedCondition = dfrobot_i2c_rgbbutton_ns.class_(
    "IsLockedCondition", automation.Condition
)
IsConnectedCondition = dfrobot_i2c_rgbbutton_ns.class_(
    "IsConnectedCondition", automation.Condition
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

CLICK_TRIGGERS = (
    (CONF_ON_CLICK, "add_on_click_callback"),
    (CONF_ON_DOUBLE_CLICK, "add_on_double_click_callback"),
    (CONF_ON_TRIPLE_CLICK, "add_on_triple_click_callback"),
    (CONF_ON_LONG_PRESS, "add_on_long_press_callback"),
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DFRobotI2CRGBButton),
            cv.Optional(CONF_DEFAULT_COLOR): color_value,
            cv.Optional(CONF_INTERRUPT_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_BRIGHTNESS, default="100%"): cv.percentage,
            cv.Optional(
                CONF_BLINK_INTERVAL, default="500ms"
            ): cv.positive_not_null_time_period,
            cv.Optional(CONF_REFRESH_INTERVAL, default="60s"): cv.update_interval,
            cv.Optional(
                CONF_LONG_PRESS_TIME, default="1s"
            ): cv.positive_not_null_time_period,
            cv.Optional(
                CONF_MULTI_CLICK_TIME, default="400ms"
            ): cv.positive_not_null_time_period,
            **{
                cv.Optional(key): automation.validate_automation({})
                for key, _ in CLICK_TRIGGERS
            },
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

    if CONF_DEFAULT_COLOR in config:
        cg.add(var.set_default_color(config[CONF_DEFAULT_COLOR]))
    if config[CONF_BRIGHTNESS] < 1.0:
        cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_blink_interval(config[CONF_BLINK_INTERVAL].total_milliseconds))
    cg.add(var.set_refresh_interval(config[CONF_REFRESH_INTERVAL].total_milliseconds))
    cg.add(var.set_long_press_time(config[CONF_LONG_PRESS_TIME].total_milliseconds))
    cg.add(var.set_multi_click_time(config[CONF_MULTI_CLICK_TIME].total_milliseconds))

    if CONF_INTERRUPT_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(pin))

    if CONF_BUTTON in config:
        button = await binary_sensor.new_binary_sensor(config[CONF_BUTTON])
        cg.add(var.set_button_sensor(button))

    if any(key in config for key, _ in CLICK_TRIGGERS):
        cg.add(var.set_click_detection(True))
    if CONF_ON_DOUBLE_CLICK in config or CONF_ON_TRIPLE_CLICK in config:
        cg.add(var.set_multi_click(True))
    await automation.build_callback_automations(
        var,
        config,
        tuple(
            automation.CallbackAutomation(key, method) for key, method in CLICK_TRIGGERS
        ),
    )


# ---------------------------------------------------------------------------
# Actions and conditions
# ---------------------------------------------------------------------------


def _ids(value):
    """One button id or a list of them (the only button if omitted)."""
    if isinstance(value, list):
        if not value:
            raise cv.Invalid("At least one id is required")
        return [cv.use_id(DFRobotI2CRGBButton)(item) for item in value]
    return [cv.use_id(DFRobotI2CRGBButton)(value)]


def _ids_schema(extra):
    return cv.Schema({cv.GenerateID(): _ids, **extra})


def _shorthand(key):
    """`action: value` is short for `action: {key: value}`."""

    def validator(value):
        if isinstance(value, dict):
            return value
        return {key: value}

    return validator


async def _new_buttons_action(config, action_id, template_arg):
    buttons = [await cg.get_variable(button_id) for button_id in config[CONF_ID]]
    return cg.new_Pvariable(action_id, template_arg, buttons)


def _fill_rgb(config):
    has_rgb = any(key in config for key in (CONF_RED, CONF_GREEN, CONF_BLUE))
    if has_rgb == (CONF_COLOR in config):
        raise cv.Invalid("Set either 'color' or 'red'/'green'/'blue'")
    if has_rgb:
        for key in (CONF_RED, CONF_GREEN, CONF_BLUE):
            config.setdefault(key, 0)
    return config


_CHANNEL = cv.templatable(cv.int_range(min=0, max=255))

SET_COLOR_ACTION_SCHEMA = cv.All(
    _shorthand(CONF_COLOR),
    _ids_schema(
        {
            cv.Optional(CONF_COLOR): cv.templatable(color_value),
            cv.Optional(CONF_RED): _CHANNEL,
            cv.Optional(CONF_GREEN): _CHANNEL,
            cv.Optional(CONF_BLUE): _CHANNEL,
            cv.Optional(CONF_BLINK): cv.templatable(cv.boolean),
            cv.Optional(CONF_ALTERNATE_COLOR): cv.templatable(color_value),
        }
    ),
    _fill_rgb,
)


@automation.register_action(
    "dfrobot_i2c_rgbbutton.set_color",
    SetColorAction,
    SET_COLOR_ACTION_SCHEMA,
    synchronous=True,
)
async def set_color_to_code(config, action_id, template_arg, args):
    var = await _new_buttons_action(config, action_id, template_arg)
    if CONF_COLOR in config:
        cg.add(var.set_color(await cg.templatable(config[CONF_COLOR], args, cg.uint32)))
    else:
        for key, setter in (
            (CONF_RED, var.set_red),
            (CONF_GREEN, var.set_green),
            (CONF_BLUE, var.set_blue),
        ):
            cg.add(setter(await cg.templatable(config[key], args, cg.uint8)))
    blink = config.get(CONF_BLINK)
    if blink is None and CONF_ALTERNATE_COLOR in config:
        blink = True  # an alternate color implies blinking
    if blink is not None:
        cg.add(var.set_blink(await cg.templatable(blink, args, cg.bool_)))
    if CONF_ALTERNATE_COLOR in config:
        cg.add(
            var.set_alternate_color(
                await cg.templatable(config[CONF_ALTERNATE_COLOR], args, cg.uint32)
            )
        )
    return var


@automation.register_action(
    "dfrobot_i2c_rgbbutton.set_brightness",
    SetBrightnessAction,
    cv.All(
        _shorthand(CONF_BRIGHTNESS),
        _ids_schema({cv.Required(CONF_BRIGHTNESS): cv.templatable(cv.percentage)}),
    ),
    synchronous=True,
)
async def set_brightness_to_code(config, action_id, template_arg, args):
    var = await _new_buttons_action(config, action_id, template_arg)
    cg.add(
        var.set_brightness(
            await cg.templatable(config[CONF_BRIGHTNESS], args, cg.float_)
        )
    )
    return var


def _ids_only(value):
    # `action: id` / `action: [id1, id2]` / `action: {id: ...}` / no value (single button)
    if value is None:
        value = {}
    if not isinstance(value, dict):
        value = {CONF_ID: value}
    return _ids_schema({})(value)


for _name, _class in (
    ("dfrobot_i2c_rgbbutton.lock", LockAction),
    ("dfrobot_i2c_rgbbutton.unlock", UnlockAction),
):

    @automation.register_action(_name, _class, _ids_only, synchronous=True)
    async def _lock_to_code(config, action_id, template_arg, args):
        return await _new_buttons_action(config, action_id, template_arg)


for _name, _class in (
    ("dfrobot_i2c_rgbbutton.is_pressed", IsPressedCondition),
    ("dfrobot_i2c_rgbbutton.is_locked", IsLockedCondition),
    ("dfrobot_i2c_rgbbutton.is_connected", IsConnectedCondition),
):

    @automation.register_condition(
        _name,
        _class,
        automation.maybe_simple_id({cv.GenerateID(): cv.use_id(DFRobotI2CRGBButton)}),
    )
    async def _condition_to_code(config, condition_id, template_arg, args):
        var = cg.new_Pvariable(condition_id, template_arg)
        await cg.register_parented(var, config[CONF_ID])
        return var
