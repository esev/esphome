#pragma once

#include "esphome/core/controller.h"
#include "ble_characteristic.h"
#include <map>

#ifdef ARDUINO_ARCH_ESP32

#include <esp_gatts_api.h>

namespace esphome {
namespace esp32_ble {

class BLEServer;

class BLEController : public Controller {
 public:
  BLEController(BLEServer *server) : server_(server) {}

  void setup();

  void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#ifdef USE_BINARY_SENSOR
  void on_binary_sensor_update(binary_sensor::BinarySensor *obj, bool state) override;
#endif
#ifdef USE_COVER
  void on_cover_update(cover::Cover *obj) override;
#endif
#ifdef USE_FAN
  void on_fan_update(fan::FanState *obj) override;
#endif
#ifdef USE_LIGHT
  void on_light_update(light::LightState *obj) override;
#endif
#ifdef USE_SENSOR
  void on_sensor_update(sensor::Sensor *obj, float state) override;
#endif
#ifdef USE_SWITCH
  void on_switch_update(switch_::Switch *obj, bool state) override;
#endif
#ifdef USE_TEXT_SENSOR
  void on_text_sensor_update(text_sensor::TextSensor *obj, std::string state) override;
#endif
#ifdef USE_CLIMATE
  void on_climate_update(climate::Climate *obj) override;
#endif

 protected:
  std::map<uint32_t, BLECharacteristic *> characteristics_;
  BLEServer *server_;

  BLEService *esphome_service_;
#ifdef USE_LOGGER
  BLECharacteristic *logger_characteristic_;
#endif

#ifdef USE_BINARY_SENSOR
  BLEService *binary_sensor_service_;
#endif
#ifdef USE_COVER
  BLEService *cover_service_;
#endif
#ifdef USE_FAN
  BLEService *fan_service_;
#endif
#ifdef USE_LIGHT
  BLEService *light_service_;
#endif
#ifdef USE_SENSOR
  BLEService *sensor_service_;
#endif
#ifdef USE_SWITCH
  BLEService *switch_service_;
#endif
#ifdef USE_TEXT_SENSOR
  BLEService *text_sensor_service_;
#endif
#ifdef USE_CLIMATE
  BLEService *climate_service_;
#endif
};

}  // namespace esp32_ble
}  // namespace esphome

#endif
