#include "ble_controller.h"

#include "esphome/core/application.h"
#include "ble_server.h"
#include "ble_2901.h"
#include "ble_2902.h"

#ifdef USE_LOGGER
#include "esphome/components/logger/logger.h"
#endif

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace esp32_ble {

static const char *ESPHOME_SERVICE_UUID = "03774663-d394-496e-8dcd-000000000000";
static const char *LOGGER_CHARACTERISTIC_UUID = "03774663-d394-496e-8dcd-000000000001";

static const char *BINARY_SENSOR_SERVICE_UUID = "03774663-d394-496e-8dcd-000100000000";
static const char *COVER_SERVICE_UUID = "03774663-d394-496e-8dcd-000200000000";
static const char *FAN_SERVICE_UUID = "03774663-d394-496e-8dcd-000300000000";
static const char *LIGHT_SERVICE_UUID = "03774663-d394-496e-8dcd-000400000000";
static const char *SENSOR_SERVICE_UUID = "03774663-d394-496e-8dcd-000500000000";
static const char *SWITCH_SERVICE_UUID = "03774663-d394-496e-8dcd-000600000000";
static const char *TEXT_SENSOR_SERVICE_UUID = "03774663-d394-496e-8dcd-000700000000";
static const char *CLIMATE_SERVICE_UUID = "03774663-d394-496e-8dcd-000800000000";

static const char *TAG = "esp32_ble.controller";

void BLEController::setup() {
  this->esphome_service_ = this->server_->create_service(ESPHOME_SERVICE_UUID);

#ifdef USE_LOGGER
  this->logger_characteristic_ = this->esphome_service_->create_characteristic(
      LOGGER_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  BLEDescriptor *logger_name = new BLE2901("Logger");
  this->logger_characteristic_->add_descriptor(logger_name);

  BLEDescriptor *descriptor_2902 = new BLE2902();
  this->logger_characteristic_->add_descriptor(descriptor_2902);

  logger::global_logger->add_on_log_callback([this](int level, const char *tag, const char *message) {
    std::string log;
    log += "[";
    log += tag;
    log += "] ";
    log += message;
    this->logger_characteristic_->set_value(log);
    this->logger_characteristic_->notify();
  });
#endif

  this->esphome_service_->start();

#ifdef USE_BINARY_SENSOR
  auto binary_sensors = App.get_binary_sensors();
  if (!binary_sensors.empty()) {
    BLEService *service = this->server_->create_service(BINARY_SENSOR_SERVICE_UUID);
    for (auto *obj : binary_sensors) {
      std::string uuid = std::string(BINARY_SENSOR_SERVICE_UUID).substr(0, 28);
      uuid += uint32_to_string(obj->get_object_id_hash());
      BLECharacteristic *characteristic =
          service->create_characteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

      BLEDescriptor *name = new BLE2901(obj->get_name());
      characteristic->add_descriptor(name);

      BLEDescriptor *descriptor = new BLE2902();
      characteristic->add_descriptor(descriptor);

      this->characteristics_.insert(
          std::pair<uint32_t, BLECharacteristic *>(obj->get_object_id_hash(), characteristic));
    }
    service->start();
    this->binary_sensor_service_ = service;
  }
#endif
#ifdef USE_COVER
  for (auto *cover : App.get_covers()) {
  }
#endif
#ifdef USE_FAN
  for (auto *fan : App.get_fans()) {
  }
#endif
#ifdef USE_LIGHT
  for (auto *light : App.get_lights()) {
  }
#endif
#ifdef USE_SENSOR
  auto sensors = App.get_sensors();
  if (!sensors.empty()) {
    BLEService *service = this->server_->create_service(SENSOR_SERVICE_UUID);
    for (auto *obj : sensors) {
      std::string uuid = std::string(SENSOR_SERVICE_UUID).substr(0, 28);
      uuid += uint32_to_string(obj->get_object_id_hash());
      BLECharacteristic *characteristic =
          service->create_characteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

      BLEDescriptor *name = new BLE2901(obj->get_name());
      characteristic->add_descriptor(name);

      BLEDescriptor *descriptor = new BLE2902();
      characteristic->add_descriptor(descriptor);

      this->characteristics_.insert(
          std::pair<uint32_t, BLECharacteristic *>(obj->get_object_id_hash(), characteristic));
    }
    service->start();
    this->sensor_service_ = service;
  }
#endif
#ifdef USE_SWITCH
  auto switches = App.get_switches();
  if (!switches.empty()) {
    BLEService *service = this->server_->create_service(SWITCH_SERVICE_UUID);
    for (auto *obj : switches) {
      std::string uuid = std::string(SWITCH_SERVICE_UUID).substr(0, 28);
      uuid += uint32_to_string(obj->get_object_id_hash());
      BLECharacteristic *characteristic =
          service->create_characteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
                                                   BLECharacteristic::PROPERTY_WRITE);

      BLEDescriptor *name = new BLE2901(obj->get_name());
      characteristic->add_descriptor(name);

      BLEDescriptor *descriptor = new BLE2902();
      characteristic->add_descriptor(descriptor);

      this->characteristics_.insert(
          std::pair<uint32_t, BLECharacteristic *>(obj->get_object_id_hash(), characteristic));

      characteristic->on_write([obj](std::vector<uint8_t> data) {
        if (data[0])
          obj->turn_on();
        else
          obj->turn_off();
      });
    }
    service->start();
    this->switch_service_ = service;
  }
#endif
#ifdef USE_TEXT_SENSOR
  auto text_sensors = App.get_text_sensors();
  if (!text_sensors.empty()) {
    BLEService *service = this->server_->create_service(TEXT_SENSOR_SERVICE_UUID);
    for (auto *obj : text_sensors) {
      std::string uuid = std::string(TEXT_SENSOR_SERVICE_UUID).substr(0, 28);
      uuid += uint32_to_string(obj->get_object_id_hash());
      BLECharacteristic *characteristic =
          service->create_characteristic(uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

      BLEDescriptor *name = new BLE2901(obj->get_name());
      characteristic->add_descriptor(name);

      BLEDescriptor *descriptor = new BLE2902();
      characteristic->add_descriptor(descriptor);

      this->characteristics_.insert(
          std::pair<uint32_t, BLECharacteristic *>(obj->get_object_id_hash(), characteristic));
    }
    service->start();
    this->text_sensor_service_ = service;
  }
#endif
#ifdef USE_CLIMATE
  for (auto *climate : App.get_climates()) {
  }
#endif

  this->setup_controller();
}

void BLEController::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {}

#ifdef USE_BINARY_SENSOR
void BLEController::on_binary_sensor_update(binary_sensor::BinarySensor *obj, bool state) {
  if (obj->is_internal())
    return;
  auto *characteristic = this->characteristics_[obj->get_object_id_hash()];
  characteristic->set_value(state);
  characteristic->notify();
}
#endif
#ifdef USE_COVER
void BLEController::on_cover_update(cover::Cover *obj) {
  if (obj->is_internal())
    return;
}
#endif
#ifdef USE_FAN
void BLEController::on_fan_update(fan::FanState *obj) {
  if (obj->is_internal())
    return;
}
#endif
#ifdef USE_LIGHT
void BLEController::on_light_update(light::LightState *obj) {
  if (obj->is_internal())
    return;
}
#endif
#ifdef USE_SENSOR
void BLEController::on_sensor_update(sensor::Sensor *obj, float state) {
  if (obj->is_internal())
    return;
  auto *characteristic = this->characteristics_[obj->get_object_id_hash()];
  characteristic->set_value(state);
  characteristic->notify();
}
#endif
#ifdef USE_SWITCH
void BLEController::on_switch_update(switch_::Switch *obj, bool state) {
  if (obj->is_internal())
    return;
  auto *characteristic = this->characteristics_[obj->get_object_id_hash()];
  characteristic->set_value(state);
  characteristic->notify();
}
#endif
#ifdef USE_TEXT_SENSOR
void BLEController::on_text_sensor_update(text_sensor::TextSensor *obj, std::string state) {
  if (obj->is_internal())
    return;
  auto *characteristic = this->characteristics_[obj->get_object_id_hash()];
  characteristic->set_value(state);
  characteristic->notify();
}
#endif
#ifdef USE_CLIMATE
void BLEController::on_climate_update(climate::Climate *obj) {
  if (obj->is_internal())
    return;
}
#endif

}  // namespace esp32_ble
}  // namespace esphome

#endif
