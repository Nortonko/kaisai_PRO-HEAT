import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from . import KaisaiAC, KaisaiSelect, CONF_KAISAI_ID

CONF_TARGET = "target"

# target -> číselný kód (musí zodpovedať enumu KaisaiSelectTarget v kaisai_ac.h)
TARGETS = {
    "vane_vertical": 0,
    "vane_horizontal": 1,
    "fan": 2,
}

# hodnoty musia presne zodpovedať poliam VANE_V / VANE_H / FAN_NAMES v kaisai_ac.cpp
VANE_V_OPTIONS = [
    "kývanie", "prúd nadol", "prúd nahor", "hore",
    "hore-stred", "stred", "stred-dole", "dole",
]
VANE_H_OPTIONS = [
    "kývanie", "prúd vľavo", "prúd stred", "prúd vpravo",
    "vľavo", "vľavo-stred", "stred", "stred-vpravo", "vpravo",
]
FAN_OPTIONS = ["AUTO", "MUTE", "LOW", "LOW-MID", "MID", "MID-HIGH", "HIGH", "TURBO"]

OPTIONS_BY_TARGET = {
    0: VANE_V_OPTIONS,
    1: VANE_H_OPTIONS,
    2: FAN_OPTIONS,
}

CONFIG_SCHEMA = select.select_schema(KaisaiSelect).extend(
    {
        cv.Required(CONF_KAISAI_ID): cv.use_id(KaisaiAC),
        cv.Required(CONF_TARGET): cv.enum(TARGETS, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    target = TARGETS[str(config[CONF_TARGET])]  # 'fan' -> 2
    options = OPTIONS_BY_TARGET[target]
    var = await select.new_select(config, options=options)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_KAISAI_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_target(target))
