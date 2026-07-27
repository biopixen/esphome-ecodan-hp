#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ecodan_scanner {

class EcodanScanner : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

 protected:
  void send_connect_();
  void send_get_(uint8_t type);
  uint8_t checksum_(const uint8_t *data, size_t len);
  void log_packet_(const char *prefix, const uint8_t *data, size_t len);
  void handle_reply_(uint8_t type, const uint8_t *data, size_t len);

  static const size_t MONITOR_COUNT = 5;
  static const uint8_t MONITOR_TYPES[MONITOR_COUNT];

  enum class State { CONNECTING, WAIT_CONNECT_ACK, POLLING, WAIT_REPLY };
  State state_{State::CONNECTING};
  uint32_t last_action_time_{0};
  size_t monitor_index_{0};
  uint8_t rx_buffer_[64];
  size_t rx_len_{0};
  uint32_t last_byte_time_{0};

  uint8_t last_reply_[MONITOR_COUNT][21];
  bool has_last_[MONITOR_COUNT] = {false, false, false, false, false};
};

}  // namespace ecodan_scanner
}  // namespace esphome
