import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import STATE_CLASS_MEASUREMENT

from . import KaisaiAC, KaisaiSensor, CONF_KAISAI_ID

CONF_TYPE = "type"

# type -> číselný kód (musí zodpovedať enumu KaisaiSensorType v kaisai_ac.h)
SENSOR_TYPES = {
    "fan_rpm": 0,           # pole 0x64 — otáčky ventilátora vonkajšej jednotky (RPM)
    "compressor_freq": 1,   # pole 0x65 — frekvencia kompresora (Hz)
    "coil_temp_inner": 2,   # pole 0x5C — teplota vnútorného výmenníka (°C)
    "coil_temp_outer": 3,   # pole 0x60 — teplota vonkajšieho výmenníka (°C)
}

CONFIG_SCHEMA = sensor.sensor_schema(
    KaisaiSensor,
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.Required(CONF_KAISAI_ID): cv.use_id(KaisaiAC),
        cv.Required(CONF_TYPE): cv.enum(SENSOR_TYPES, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_KAISAI_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_sensor_type(SENSOR_TYPES[str(config[CONF_TYPE])]))
