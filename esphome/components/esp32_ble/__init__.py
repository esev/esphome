import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MODEL, ESP_PLATFORM_ESP32

ESP_PLATFORMS = [ESP_PLATFORM_ESP32]
CODEOWNERS = ["@jesserockz"]

CONF_MANUFACTURER = "manufacturer"
CONF_SERVER = "server"
CONF_USE_CONTROLLER = "use_controller"

esp32_ble_ns = cg.esphome_ns.namespace("esp32_ble")
ESP32BLE = esp32_ble_ns.class_("ESP32BLE", cg.Component)
BLEServer = esp32_ble_ns.class_("BLEServer", cg.Component)

BLEServiceComponent = esp32_ble_ns.class_("BLEServiceComponent")


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ESP32BLE),
        cv.Optional(CONF_SERVER): cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(BLEServer),
                cv.Optional(CONF_MANUFACTURER, default="ESPHome"): cv.string,
                cv.Optional(CONF_MODEL): cv.string,
                cv.Optional(CONF_USE_CONTROLLER, default=False): cv.boolean,
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_SERVER in config:
        server = await server_to_code(config[CONF_SERVER])
        cg.add(var.set_server(server))


async def server_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_use_controller(config[CONF_USE_CONTROLLER]))
    cg.add(var.set_manufacturer(config[CONF_MANUFACTURER]))
    if CONF_MODEL in config:
        cg.add(var.set_model(config[CONF_MODEL]))
    cg.add_define("USE_ESP32_BLE_SERVER")
    return var
