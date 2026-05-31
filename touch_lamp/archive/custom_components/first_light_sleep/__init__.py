CODEOWNERS = ["@ChrisCullin"]

from esphome import automation, pins
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import (
    CONF_DEFAULT,
    CONF_HOUR,
    CONF_ID,
    CONF_MINUTE,
    CONF_MODE,
    CONF_NUMBER,
    CONF_PINS,
    CONF_RUN_DURATION,
    CONF_SECOND,
    CONF_SLEEP_DURATION,
    CONF_TIME_ID,
    CONF_WAKEUP_PIN,
    PLATFORM_ESP32,
    PLATFORM_ESP8266,
)

WAKEUP_PINS = [
        0,
        2,
        4,
        12,
        13,
        14,
        15,
        25,
        26,
        27,
        32,
        33,
        34,
        35,
        36,
        37,
        38,
        39,
    ]

# Define the C++ namespace and classes
light_sleep_ns = cg.esphome_ns.namespace("light_sleep")
LightSleepComponent = light_sleep_ns.class_("LightSleepComponent", cg.Component)
EnterLightSleepAction = light_sleep_ns.class_("EnterLightSleepAction", automation.Action)
PreventDeepSleepAction = light_sleep_ns.class_(
    "PreventDeepSleepAction",
    automation.Action,
    cg.Parented.template(LightSleepComponent),
)
AllowDeepSleepAction = light_sleep_ns.class_(
    "AllowDeepSleepAction",
    automation.Action,
    cg.Parented.template(LightSleepComponent),
)

WakeupPinMode = light_sleep_ns.enum("WakeupPinMode")
WAKEUP_PIN_MODES = {
    "IGNORE": WakeupPinMode.WAKEUP_PIN_MODE_IGNORE,
    "KEEP_AWAKE": WakeupPinMode.WAKEUP_PIN_MODE_KEEP_AWAKE,
    "INVERT_WAKEUP": WakeupPinMode.WAKEUP_PIN_MODE_INVERT_WAKEUP,
}

esp_sleep_ext1_wakeup_mode_t = cg.global_ns.enum("esp_sleep_ext1_wakeup_mode_t")
Ext1Wakeup = light_sleep_ns.struct("Ext1Wakeup")
EXT1_WAKEUP_MODES = {
    "ALL_LOW": esp_sleep_ext1_wakeup_mode_t.ESP_EXT1_WAKEUP_ALL_LOW,
    "ANY_HIGH": esp_sleep_ext1_wakeup_mode_t.ESP_EXT1_WAKEUP_ANY_HIGH,
}


CONF_WAKEUP_PIN_MODE = "wakeup_pin_mode"
CONF_ESP32_EXT1_WAKEUP = "esp32_ext1_wakeup"
CONF_TOUCH_WAKEUP = "touch_wakeup"


def validate_pin_number(value):
    valid_pins = WAKEUP_PINS
    if value[CONF_NUMBER] not in valid_pins:
        raise cv.Invalid(
            f"Only pins {', '.join(str(x) for x in valid_pins)} support wakeup"
        )
    return value

# Component Schema for 'light_sleep:'
# This defines global settings for the light_sleep component
CONFIG_SCHEMA = cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LightSleepComponent),
            cv.Optional(CONF_SLEEP_DURATION): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_WAKEUP_PIN): cv.All(
                pins.internal_gpio_input_pin_schema,
                validate_pin_number,
            ),
            cv.Optional(CONF_WAKEUP_PIN_MODE): cv.All(
                cv.enum(WAKEUP_PIN_MODES), upper=True
            ),
            cv.Optional(CONF_ESP32_EXT1_WAKEUP): cv.All(
                cv.Schema(
                    {
                        cv.Required(CONF_PINS): cv.ensure_list(
                            pins.internal_gpio_input_pin_schema, validate_pin_number
                        ),
                        cv.Required(CONF_MODE): cv.enum(EXT1_WAKEUP_MODES, upper=True),
                    }
                ),
            ),
            cv.Optional(CONF_TOUCH_WAKEUP): cv.All(cv.only_on_esp32, cv.boolean),
        }
    ).extend(cv.COMPONENT_SCHEMA)


# Code generation for the 'light_sleep:' component
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_SLEEP_DURATION in config:
        # Convert ms to us for the C++ setter (as per esp_sleep_enable_timer_wakeup)
        cg.add(var.set_sleep_duration(config[CONF_SLEEP_DURATION]))

    if CONF_PINS in config:
        pin = await cg.gpio_pin_expression(config[CONF_PINS])
        cg.add(var.set_wakeup_pin(pin))
        mode = config[CONF_MODE]
        cg.add(var.set_wakeup_pin_mode(light_sleep_ns.enum("WakeupPinMode")[mode]))



# Action Schema for 'light_sleep.enter:'
# This defines how the action to trigger sleep is configured
@automation.register_action(
    "light_sleep.enter",
    EnterLightSleepAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(LightSleepComponent),
            cv.Optional(CONF_SLEEP_DURATION): cv.All(
                cv.positive_time_period_milliseconds,
                "sleep_duration must be a positive time period (e.g., 15s, 5min)",
            ),
            # Add other action-specific overrides if needed (e.g., temporary pin change)
        }
    ),
)
async def light_sleep_enter_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    if CONF_SLEEP_DURATION in config:
        # Pass milliseconds, C++ setter will convert to microseconds
        template_ = await cg.templatable(
            config[CONF_SLEEP_DURATION], args, cg.uint32
        )
        cg.add(var.set_sleep_duration(template_))
    return var
