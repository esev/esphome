#include "garage_door.h"

#include "esphome/core/log.h"
#include "esphome/core/optional.h"

namespace esev {
namespace garage_door {

namespace {
using ::esphome::optional;
using ::esphome::nullopt;
using ::esphome::binary_sensor::BinarySensor;
using ::esphome::esp_log_printf_;

static const char* TAG = "garage_door";

void UpdateIfChanged(const optional<bool>& opt, GarageDoorSwitch* val) {
  if (opt.value_or(val->state) != val->state) {
    val->publish_state(*opt);
  }
}

void UpdateIfChanged(const optional<bool>& opt, BinarySensor* val) {
  if (opt.value_or(val->state) != val->state) {
    val->publish_state(*opt);
  }
}

static constexpr ToggleSequence kToggleDoor  = {0x30, 0x31, 0x31, 220, 20};
static constexpr ToggleSequence kToggleLight = {0x32, 0x33, 0x33, 220, 20};
static constexpr ToggleSequence kToggleLock  = {0x34, 0x35, 0x35, 220, 20};

// Request from the button to the opener.
enum Command {
  DOOR_STATE = 0x38,
  EYE_SENSOR_STATE = 0x39,
  LIGHT_AND_LOCK_STATE = 0x3a,
};

// First nibble: 0x5
// Last nibble bits: <unlocked>:<light_on>:0:1
static constexpr uint8_t kLightStateMask = 0xf7;
enum LightState { // Must be masked first.
  ON = 0x55,
  OFF = 0x51,
};
static constexpr uint8_t kLockStateMask = 0xfb;
enum LockState { // Must be masked first.
  UNLOCKED = 0x59,
  LOCKED = 0x51,
};

enum DoorState {
  OPENING = 0x01,
  CLOSING = 0x04,
  STOPPED = 0x06,  // Between open & closed but not moving.

  CLOSED = 0x55,
  OPEN = 0x52,
  // 0x5b?
};
enum EyeSensorState {
  CLEAR = 0x00,
  BLOCKED = 0x04,
};
}  // namespace

GarageDoorSwitch::GarageDoorSwitch(
  std::string icon_true, std::string icon_false, Callback&& callback)
  : callback_(std::move(callback)),
    icon_true_(std::move(icon_true)),
    icon_false_(std::move(icon_false)) {}

void GarageDoorSwitch::write_state(bool state) {
  callback_();
   // publish_state(state);  // It'll get a state update after the next poll.
}

std::string GarageDoorSwitch::icon() {
  return state ? icon_true_ : icon_false_;
}


GarageDoor::GarageDoor()
  : door(new GarageDoorSwitch("mdi:garage-open", "mdi:garage", [this]{
      this->toggle(kToggleDoor);
    })),
    light(new GarageDoorSwitch("mdi:lightbulb-on", "mdi:lightbulb", [this]{
      this->toggle(kToggleLight);
    })),
    lock(new GarageDoorSwitch("mdi:lock", "mdi:lock-open", [this]{
      this->toggle(kToggleLock);
    })),
    eye_sensor(new esphome::binary_sensor::BinarySensor) {
  for (int i = 0; i < kInputLen; i++) {
    input_[i].millis = 0;
    input_[i].data = 0;
  }
  input_ptr_ = 0;
}

void GarageDoor::loop() {
  size_t ptr = input_ptr_;

  while (available()) {
    SerialInput& input = NextInput();
    if (read_byte(&input.data)) {
      input.millis = millis();
      input_ptr_++;
    }
  }

  if (ptr != input_ptr_) {
    update_state(input_ptr_);
  }
}

void GarageDoor::update_state(size_t ptr) {
  optional<bool> door_state;
  optional<bool> light_state;
  optional<bool> eye_sensor_state;
  optional<bool> door_lock_state;
  bool has_toggle = false;

  for (int i = 0; i < kInputLen; i++) {
    const SerialInput& input = GetInput(i+ptr);
    if (!input.millis) continue;
    const bool has_command = (
      input.data == Command::LIGHT_AND_LOCK_STATE ||
      input.data == Command::DOOR_STATE ||
      input.data == Command::EYE_SENSOR_STATE);
    has_toggle |= (
      input.data == kToggleDoor.data1 ||
      input.data == kToggleLight.data1 ||
      input.data == kToggleLock.data1);

    // Look for request & response patterns.
    if (has_command && i + 1 < kInputLen) {
      i++;
      const SerialInput& state = GetInput(i+ptr);
      if (state.millis - input.millis > kMaxResponseDelayMs) {
        ESP_LOGI(
          TAG,
          "Time between request/response exceeds %d milliseconds: %lu",
          kMaxResponseDelayMs, state.millis - input.millis);
        continue;
      }
      switch (input.data) {
        case Command::LIGHT_AND_LOCK_STATE:
          switch (state.data & kLightStateMask) {
            case LightState::ON:
            light_state = true;
            break;

            case LightState::OFF:
            light_state = false;
            break;

            default:
            ESP_LOGI(
              TAG, "Unknown LightState: 0x%02x/0x%02x",
              state.data & kLightStateMask, state.data);
            break;
          }
          switch(state.data & kLockStateMask) {
            case LockState::UNLOCKED:
            door_lock_state = false;
            break;

            case LockState::LOCKED:
            door_lock_state = true;
            break;

            default:
            ESP_LOGI(
              TAG, "Unknown LockState: 0x%02x/0x%02x",
              state.data & kLockStateMask, state.data);
            break;
          }
          break;

        case Command::DOOR_STATE:
          switch (state.data) {
            case DoorState::CLOSING:
            ESP_LOGI(TAG, "Door CLOSING");
            case DoorState::CLOSED:
            door_state = false;
            break;

            case DoorState::STOPPED:
            ESP_LOGI(TAG, "Door STOPPED");
            case DoorState::OPENING:
            ESP_LOGI(TAG, "Door OPENING");
            case DoorState::OPEN:
            door_state = true;
            break;

            default:
            ESP_LOGW(
              TAG, "Unexpected Door state: 0x%02x",
              state.data);
            break;
          }
          break;

        case Command::EYE_SENSOR_STATE:
          switch (state.data) {
            case EyeSensorState::CLEAR:
            eye_sensor_state = false;
            break;

            case EyeSensorState::BLOCKED:
            eye_sensor_state = true;
            break;

            default:
              ESP_LOGW(
                TAG, "Unexpected EyeSensor state: 0x%02x",
                state.data);
          }
         break;
      }
    } else if (has_toggle && i == 0) {
      switch(input.data) {
        case kToggleDoor.data1:
        if (CheckToggle23(ptr, kToggleDoor)) {
          ESP_LOGI(TAG, "Door toggled");
        }
        break;

        case kToggleLight.data1:
        if (CheckToggle23(ptr, kToggleLight)) {
          ESP_LOGI(TAG, "Light toggled");
        }
        break;

        case kToggleLock.data1:
        if (CheckToggle23(ptr, kToggleLock)) {
          ESP_LOGI(TAG, "Lock toggled");
        }
        break;

        default:
        ESP_LOGE(TAG, "Unknown toggle sequence");
        DumpInputState(ptr);
        break;
      }
    } else if (i + 1 == kInputLen && !has_command && !has_toggle) {
      ESP_LOGI(
        TAG, "Unexpected input data: Millis(%lums) Data(0x%02x)",
        input.millis, input.data);
    }
  }

  UpdateIfChanged(door_state, door);
  UpdateIfChanged(light_state, light);
  UpdateIfChanged(eye_sensor_state, eye_sensor);
  UpdateIfChanged(door_lock_state, lock);
}

bool GarageDoor::CheckToggle23(size_t ptr, const ToggleSequence& sequence) const {
  const unsigned long start = GetInput(ptr).millis;
  // Handle the case where data3 directly follows data2.
  for (int i = 1; i < kInputLen - 1; i++) {
    const SerialInput& data2 = GetInput(ptr+i);
    const SerialInput& data3 = GetInput(ptr+i+1);
    if (data2.data == sequence.data2 && data3.data == sequence.data3) {
      ESP_LOGI(
        TAG,
        "ToggleSequence timings: (data2-data1)=%lums (data3-data2)=%lums",
        data2.millis - start,
        data3.millis - data2.millis);
      return true;
    }
  }
  // Handle the case where data2 & data3 have a command between them.
  for (int i = 1; i < kInputLen - 3; i++) {
    const SerialInput& data2 = GetInput(ptr+i);
    const SerialInput& data3 = GetInput(ptr+i+3);
    if (data2.data == sequence.data2 && data3.data == sequence.data3) {
      ESP_LOGI(
        TAG,
        "ToggleSequence timings: (data2-data1)=%lums (data3-data2)=%lums",
        data2.millis - start,
        data3.millis - data2.millis);
      return true;
    }
  }
  ESP_LOGE(
    TAG, "Toggle sequence not found: 0x%02x 0x%02x 0x%02x",
    sequence.data1, sequence.data2, sequence.data3);
  DumpInputState(ptr);
  return false;
}

void GarageDoor::toggle(const ToggleSequence& sequence) {
  fast_looping_.start();

  const SerialInput& prev_input = GetInput(input_ptr_ + kInputLen - 1);
  const unsigned long now = millis();
  const bool delay = prev_input.millis + 200 > now;
  const uint32_t start_time = delay ? 100 : 0;

  set_timeout(
    "garage_toggle_1", start_time,
    [this, &sequence] { this->write_byte(sequence.data1); });
  set_timeout(
    "garage_toggle_2",
    start_time + sequence.data2_millis,
    [this, &sequence] { this->write_byte(sequence.data2); });
  set_timeout(
    "garage_toggle_3",
    start_time + sequence.data2_millis + sequence.data3_millis,
    [this, &sequence] {
      this->write_byte(sequence.data3);
      this->fast_looping_.stop();
    });
}

void GarageDoor::DumpInputState(size_t ptr) const {
  const unsigned long start = GetInput(ptr).millis;
  for (int i = 0; i < kInputLen; i++) {
    const SerialInput& input = GetInput(i+ptr);
    ESP_LOGE(
      TAG, "Input[%d]: 0x%02x (%lums)",
      i, input.data, input.millis - start);
  }
}

}  // namespace garage_door
}  // namespace esev