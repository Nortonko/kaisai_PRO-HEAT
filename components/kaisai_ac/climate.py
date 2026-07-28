import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID

from . import KaisaiAC

CONF_POLL_INTERVAL = "poll_interval"

CONFIG_SCHEMA = (
    climate.climate_schema(KaisaiAC)
    .extend(
        {
            cv.Optional(
                CONF_POLL_INTERVAL, default="2s"
            ): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_poll_interval(config[CONF_POLL_INTERVAL]))
