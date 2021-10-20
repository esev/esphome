import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import CONF_GARAGE_DOOR_ID, GarageDoorComponent

DEPENDENCIES = ["garage_door"]

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(CONF_GARAGE_DOOR_ID): cv.use_id(GarageDoorComponent),
    }
)


async def to_code(config):
    gd = await cg.get_variable(config[CONF_GARAGE_DOOR_ID])
    await binary_sensor.register_binary_sensor(gd.eye_sensor, config)
