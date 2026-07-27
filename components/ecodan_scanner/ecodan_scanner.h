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

  enum class State { CONNECTING, WAIT_CONNECT_ACK, SCANNING, WAIT_REPLY, DONE };
  State state_{State::CONNECTING};
  uint32_t last_action_time_{0};
  uint16_t current_type_{0};
  uint8_t rx_buffer_[64];
  size_t rx_len_{0};
  uint32_t last_byte_time_{0};
};

}  // namespace ecodan_scanner
}  // namespace esphome
