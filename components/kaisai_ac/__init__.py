import esphome.codegen as cg
from esphome.components import climate, switch, select, sensor

CODEOWNERS = ["@Nortonko"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["switch", "select", "sensor"]

kaisai_ns = cg.esphome_ns.namespace("kaisai_ac")

KaisaiAC = kaisai_ns.class_("KaisaiAC", climate.Climate, cg.Component)
KaisaiSwitch = kaisai_ns.class_("KaisaiSwitch", switch.Switch, cg.Component)
KaisaiSelect = kaisai_ns.class_("KaisaiSelect", select.Select, cg.Component)
KaisaiSensor = kaisai_ns.class_("KaisaiSensor", sensor.Sensor, cg.Component)

KaisaiFunction = kaisai_ns.enum("KaisaiFunction")

# spoločný kľúč, ktorým switch/select/sensor odkazujú na hlavný climate komponent
CONF_KAISAI_ID = "kaisai_ac_id"
