import esphome.codegen as cg
from esphome.components import climate, switch, select

CODEOWNERS = ["@Nortonko"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["switch", "select"]

kaisai_ns = cg.esphome_ns.namespace("kaisai_ac")

KaisaiAC = kaisai_ns.class_(
    "KaisaiAC", climate.Climate, cg.Component
)
KaisaiSwitch = kaisai_ns.class_("KaisaiSwitch", switch.Switch, cg.Component)
KaisaiSelect = kaisai_ns.class_("KaisaiSelect", select.Select, cg.Component)

KaisaiFunction = kaisai_ns.enum("KaisaiFunction")

# spoločný kľúč, ktorým switch/select odkazujú na hlavný climate komponent
CONF_KAISAI_ID = "kaisai_ac_id"
