import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from . import KaisaiAC, KaisaiSwitch, KaisaiFunction, CONF_KAISAI_ID

CONF_FUNCTION = "function"

FUNCTIONS = {
    "eco": KaisaiFunction.FUNC_ECO,
    "sleep": KaisaiFunction.FUNC_SLEEP,
    "health": KaisaiFunction.FUNC_HEALTH,
    "anti_mildew": KaisaiFunction.FUNC_MILDEW,
    "display": KaisaiFunction.FUNC_DISPLAY,
    "beep": KaisaiFunction.FUNC_BEEP,
    "soft_wind": KaisaiFunction.FUNC_SOFTWIND,
}

CONFIG_SCHEMA = switch.switch_schema(KaisaiSwitch).extend(
    {
        cv.Required(CONF_KAISAI_ID): cv.use_id(KaisaiAC),
        cv.Required(CONF_FUNCTION): cv.enum(FUNCTIONS, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_KAISAI_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_function(config[CONF_FUNCTION]))
