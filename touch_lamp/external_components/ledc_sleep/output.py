import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import output
from esphome import pins

from esphome.const import (
    CONF_FREQUENCY,
    CONF_ID,
    CONF_PIN,
    CONF_RESOLUTION,
)

import logging
_LOGGER = logging.getLogger(__name__)

# Define your custom CONF_ constants if they aren't imported from esphome.const
CONF_CLK_CFG = "clk_cfg"
CONF_SLEEP_MODE = "sleep_mode"

# --- Revert these to direct integer mappings ---
LEDC_CLOCK_SOURCES = {
    "AUTO_CLK": 0,
    "APB_CLK": 4,           # Corresponds to LEDC_USE_APB_CLK (which is SOC_MOD_CLK_APB = 1)
    "RC_FAST_CLK": 8,       # Corresponds to LEDC_USE_RC_FAST_CLK (which is SOC_MOD_CLK_RC_FAST = 2)
    "REF_TICK": 11,          # Corresponds to LEDC_USE_REF_TICK (which is SOC_MOD_CLK_REF_TICK = 3)
    "RTC8M_CLK": 8,         # As per the enum, this is an alias for RC_FAST_CLK
}

LEDC_SLEEP_MODES = { # Already confirmed correct from your ledc.h snippet
    "NO_ALIVE_NO_PD": 0,
    "NO_ALIVE_ALLOW_PD": 1,
    "KEEP_ALIVE": 2,
    "INVALID": 3,
}

# Define your custom component's namespace
ledc_sleep_ns = cg.esphome_ns.namespace("ledc_sleep")

# Define your C++ component class.
# IMPORTANT: It MUST inherit from output.FloatOutput (which itself inherits from cg.Component)
LEDC_SleepComponent = ledc_sleep_ns.class_(
    "LEDC_SleepComponent", output.FloatOutput, cg.Component
)



# --- Define the YAML configuration schema for your 'output' platform ---
CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(LEDC_SleepComponent),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_FREQUENCY, default="500Hz"): cv.frequency,
        cv.Optional(CONF_RESOLUTION, default=8): cv.int_range(min=1, max=14),
        cv.Optional(CONF_CLK_CFG, default="APB_CLK"): cv.enum(LEDC_CLOCK_SOURCES, upper=True, space="_"),
        cv.Optional(CONF_SLEEP_MODE, default="NO_ALIVE_NO_PD"): cv.enum(LEDC_SLEEP_MODES, upper=True, space="_"),
    }  
).extend(cv.COMPONENT_SCHEMA)

# --- Code Generation Function ---
def to_code(config):
    pin_var = yield cg.gpio_pin_expression(config[CONF_PIN])
    var = cg.new_Pvariable(config[CONF_ID], pin_var)
    yield cg.register_component(var, config)

    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_resolution(config[CONF_RESOLUTION]))

    # (Optional: remove debug prints here if you're confident, or keep for one last check)
    _LOGGER.info("DEBUG: config[CONF_CLK_CFG] value: %s, type: %s", config[CONF_CLK_CFG], type(config[CONF_CLK_CFG]))
    _LOGGER.info("DEBUG: config[CONF_SLEEP_MODE] value: %s, type: %s", config[CONF_SLEEP_MODE], type(config[CONF_SLEEP_MODE]))

    clk_cfg_key = str(config[CONF_CLK_CFG])
    sleep_mode_key = str(config[CONF_SLEEP_MODE])

    clk_cfg_int_value = LEDC_CLOCK_SOURCES[clk_cfg_key]
    sleep_mode_int_value = LEDC_SLEEP_MODES[sleep_mode_key]

    # --- FINAL CRITICAL CHANGE: Cast the integer value in C++ ---
    # This will generate: my_custom_led_output->set_clk_cfg((ledc_clk_cfg_t)1);
    cg.add(var.set_clk_cfg(
        cg.RawExpression(f"(ledc_clk_cfg_t){clk_cfg_int_value}")
    ))
    # This will generate: my_custom_led_output->set_sleep_mode((ledc_sleep_mode_t)0);
    cg.add(var.set_sleep_mode(
        cg.RawExpression(f"(ledc_sleep_mode_t){sleep_mode_int_value}")
    ))