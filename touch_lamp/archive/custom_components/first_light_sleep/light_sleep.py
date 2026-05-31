import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_SLEEP_DURATION, CONF_PIN, CONF_MODE

# Define the C++ namespace and classes
light_sleep_ns = cg.esphome_ns.namespace("light_sleep")
LightSleepComponent = light_sleep_ns.class_("LightSleepComponent", cg.Component)
EnterLightSleepAction = light_sleep_ns.class_("EnterLightSleepAction", cg.Action)

# Define WakeupPinMode enum (matching your C++ enum)
WAKEUP_PIN_MODES = {
    "IGNORE": "WAKEUP_PIN_MODE_IGNORE",
    "KEEP_AWAKE": "WAKEUP_PIN_MODE_KEEP_AWAKE",
    "INVERT_WAKEUP": "WAKEUP_PIN_MODE_INVERT_WAKEUP",
}

# Component Schema for 'light_sleep:'
# This defines global settings for the light_sleep component
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LightSleepComponent),
        cv.Optional(CONF_SLEEP_DURATION): cv.All(
            cv.positive_time_period_milliseconds,
            "sleep_duration must be a positive time period (e.g., 15s, 5min)",
        ),
        cv.Optional(CONF_PIN): cv.All(cv.gpio_pin_with_flags, cv.only_on_esp32),
        cv.Optional(CONF_MODE, default="IGNORE"): cv.enum(
            WAKEUP_PIN_MODES, upper=True
        ),
        # Add other general light sleep settings here if needed
    }
).extend(cv.COMPONENT_SCHEMA)


# Code generation for the 'light_sleep:' component
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_SLEEP_DURATION in config:
        # Convert ms to us for the C++ setter (as per esp_sleep_enable_timer_wakeup)
        cg.add(var.set_sleep_duration(config[CONF_SLEEP_DURATION]))

    if CONF_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_PIN])
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