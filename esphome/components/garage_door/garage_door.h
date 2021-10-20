#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"

namespace esev {
namespace garage_door {

// Switch with a callback into the GarageDoor class.
class GarageDoorSwitch : public esphome::switch_::Switch {
 public:
  using Callback = std::function<void(void)>;
  GarageDoorSwitch(
    std::string icon_true, std::string icon_false, Callback&& callback);

  void write_state(bool state) override;
  std::string icon() override;

 private:
  const Callback callback_;
  const std::string icon_true_;
  const std::string icon_false_;
};


struct SerialInput {
  unsigned long millis;
  uint8_t data;
};

// Serial data & timings for toggling things on/off.
struct ToggleSequence {
  const uint8_t data1;
  const uint8_t data2;
  const uint8_t data3;

  const uint16_t data2_millis;
  const uint16_t data3_millis;
};


class GarageDoor : public esphome::Component, public esphome::uart::UARTDevice {
 public:
  GarageDoor();
  void loop() override;

  // Home Assistant Entities.
  GarageDoorSwitch* door;
  GarageDoorSwitch* light;
  GarageDoorSwitch* lock;
  esphome::binary_sensor::BinarySensor* eye_sensor;

 private:
  static constexpr int kInputLen = 5;  // Circular buffer length.
  static constexpr int kMaxResponseDelayMs = 100;

  SerialInput input_[kInputLen];  // Circular buffer.
  size_t input_ptr_;
  esphome::HighFrequencyLoopRequester fast_looping_;

  SerialInput& GetInput(size_t ptr) {
    return input_[ptr % kInputLen];
  }

  const SerialInput& GetInput(size_t ptr) const {
    return input_[ptr % kInputLen];
  }

  SerialInput& NextInput() {
    return GetInput(input_ptr_);
  }

  // ptr should point to the oldest element in the circular buffer.
  void update_state(size_t ptr);

  // Attempt to find a toggle sequence within the `input_` buffer.
  bool CheckToggle23(size_t ptr, const ToggleSequence& sequence) const;

  // Send the toggle sequence to change the state of a GarageDoorSwitch.
  void toggle(const ToggleSequence& sequence);

  // Dump the state of the internal `input_` circular buffer.
  void DumpInputState(size_t ptr) const;
};

}  // namespace garage_door
}  // namespace esev