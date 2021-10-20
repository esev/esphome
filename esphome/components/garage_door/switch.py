import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from . import CONF_GARAGE_DOOR_ID, GarageDoorComponent

DEPENDENCIES = ["garage_door"]
SWITCH_VALUES = ["door", "light", "lock"]

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_GARAGE_DOOR_ID): cv.use_id(GarageDoorComponent),
        }
    )
    .extend(
        {
            cv.Optional(v): switch.SWITCH_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(switch.Switch)
                }
            )
            for v in SWITCH_VALUES
        }
    )
)


async def to_code(config):
    gd = await cg.get_variable(config[CONF_GARAGE_DOOR_ID])

    for key in SWITCH_VALUES:
        if key not in config:
            continue

        conf = config[key]
        await switch.register_switch(getattr(gd, key), conf)
