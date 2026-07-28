import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from . import KaisaiAC, KaisaiSelect, CONF_KAISAI_ID

CONF_AXIS = "axis"

# hodnoty musia presne zodpovedať poľu VANE_V / VANE_H v kaisai_ac.cpp
VANE_V_OPTIONS = [
    "kývanie",
    "prúd nadol",
    "prúd nahor",
    "hore",
    "hore-stred",
    "stred",
    "stred-dole",
    "dole",
]
VANE_H_OPTIONS = [
    "kývanie",
    "prúd vľavo",
    "prúd stred",
    "prúd vpravo",
    "vľavo",
    "vľavo-stred",
    "stred",
    "stred-vpravo",
    "vpravo",
]

# axis -> horizontal? (True = horizontálna lamela)
AXES = {"vertical": False, "horizontal": True}

CONFIG_SCHEMA = select.select_schema(KaisaiSelect).extend(
    {
        cv.Required(CONF_KAISAI_ID): cv.use_id(KaisaiAC),
        cv.Required(CONF_AXIS): cv.enum(AXES, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    horizontal = config[CONF_AXIS]  # cv.enum už vrátil bool (True/False)
    options = VANE_H_OPTIONS if horizontal else VANE_V_OPTIONS
    var = await select.new_select(config, options=options)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_KAISAI_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_horizontal(horizontal))
