import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

CODEOWNERS = ["@esev"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor", "switch"]
MULTI_CONF = True

CONF_GARAGE_DOOR_ID = "garage_door_id"

garage_door_ns = cg.global_ns.namespace("esev::garage_door")
GarageDoorComponent = garage_door_ns.class_(
    "GarageDoor", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(GarageDoorComponent),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
