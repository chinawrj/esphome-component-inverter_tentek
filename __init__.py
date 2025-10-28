"""
ESPHome Inverter Control Component for Tentek/MIC POWER

Features:
- HTTP API integration for MIC POWER inverter control
- Automatic JSESSIONID management and re-authentication
- Configurable power output (0-100%)
- Background service task for non-blocking operation
- Statistics tracking and monitoring

Compatible with both:
- ESPHome (via this component)
- ESP-IDF (via standalone project)

Author: GitHub Copilot
Date: 2025-10-27
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import (
    CONF_ID,
)

# Define namespace
inverter_tentek_ns = cg.esphome_ns.namespace("inverter_tentek")
InverterTentekComponent = inverter_tentek_ns.class_(
    "InverterTentekComponent", cg.Component
)

# Actions
SetPowerAction = inverter_tentek_ns.class_("SetPowerAction", automation.Action)

# Configuration key definitions (define our own constants)
CONF_EMAIL = "email"
CONF_PASSWORD = "password"
CONF_DEVICE_SN = "device_sn"
CONF_OUTPUT_POWER = "output_power"
CONF_REQUEST_TIMEOUT = "request_timeout"
CONF_MAX_RETRY_COUNT = "max_retry_count"

# Component configuration schema
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(InverterTentekComponent),
        cv.Required(CONF_EMAIL): cv.string,
        cv.Required(CONF_PASSWORD): cv.string,
        cv.Required(CONF_DEVICE_SN): cv.string,
        cv.Optional(CONF_OUTPUT_POWER, default=100): cv.int_range(min=0, max=100),
        cv.Optional(CONF_REQUEST_TIMEOUT, default="10s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MAX_RETRY_COUNT, default=3): cv.int_range(min=0, max=10),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    """Generate C++ code"""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Add MD5 component dependency
    cg.add_define("USE_MD5")

    # Set configuration parameters
    cg.add(var.set_email(config[CONF_EMAIL]))
    cg.add(var.set_password(config[CONF_PASSWORD]))
    cg.add(var.set_device_sn(config[CONF_DEVICE_SN]))
    cg.add(var.set_output_power(config[CONF_OUTPUT_POWER]))
    cg.add(var.set_request_timeout(config[CONF_REQUEST_TIMEOUT]))
    cg.add(var.set_max_retry_count(config[CONF_MAX_RETRY_COUNT]))


# Action: Set Power Output
@automation.register_action(
    "inverter_tentek.set_power",
    SetPowerAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(InverterTentekComponent),
            cv.Required(CONF_OUTPUT_POWER): cv.templatable(cv.int_range(min=0, max=100)),
        }
    ),
)
async def inverter_set_power_to_code(config, action_id, template_arg, args):
    """Generate code for set_power action"""
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    
    template_ = await cg.templatable(config[CONF_OUTPUT_POWER], args, int)
    cg.add(var.set_power(template_))
    
    return var
