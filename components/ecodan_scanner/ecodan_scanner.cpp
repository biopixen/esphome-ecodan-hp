#include "ecodan_scanner.h"
#include "esphome/core/log.h"
#include <cstdio>

namespace esphome {
namespace ecodan_scanner {

static const char *const TAG = "ecodan_scanner";

uint8_t EcodanScanner::checksum_(const uint8_t *data, size_t len) {
  uint32_t sum = 0;
  for (size_t i = 1; i < len; i++) sum += data[i];
  return (uint8_t) ((0x100 - (sum & 0xFF)) & 0xFF);
}

void EcodanScanner::log_packet_(const char *prefix, const uint8_t *data, size_t len) {
  char buf[3 * 64 + 1] = {0};
  size_t pos = 0;
  for (size_t i = 0; i < len && pos < sizeof(buf) - 3; i++) {
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%02X ", data[i]);
  }
  ESP_LOGI(TAG, "%s %s", prefix, buf);
}

void EcodanScanner::setup() {
  ESP_LOGI(TAG, "Ecodan Scanner starting...");
  state_ = State::CONNECTING;
  last_action_time_ = millis();
}

void EcodanScanner::send_connect_() {
  uint8_t pkt[8] = {0xFC, 0x5A, 0x01, 0x30, 0x02, 0xCA, 0x01, 0x00};
  pkt[7] = checksum_(pkt, 7);
  this->write_array(pkt, sizeof(pkt));
  log_packet_(">>> CONNECT", pkt, sizeof(pkt));
}

void EcodanScanner::send_get_(uint8_t type) {
  uint8_t pkt[21] = {0};
  pkt[0] = 0xFC;
  pkt[1] = 0x42;
  pkt[2] = 0x01;
  pkt[3] = 0x30;
  pkt[4] = 0x10;
  pkt[5] = type;
  pkt[20] = checksum_(pkt, 20);
  this->write_array(pkt, sizeof(pkt));
  char prefix[32];
  snprintf(prefix, sizeof(prefix), ">>> GET 0x%02X", type);
  log_packet_(prefix, pkt, sizeof(pkt));
}

void EcodanScanner::loop() {
  uint32_t now = millis();

  while (available()) {
    if (rx_len_ < sizeof(rx_buffer_)) {
      rx_buffer_[rx_len_++] = read();
    } else {
      read();
    }
    last_byte_time_ = now;
  }

  switch (state_) {
    case State::CONNECTING:
      send_connect_();
      state_ = State::WAIT_CONNECT_ACK;
      last_action_time_ = now;
      rx_len_ = 0;
      break;

    case State::WAIT_CONNECT_ACK:
      if (rx_len_ > 0 && (now - last_byte_time_) > 100) {
        log_packet_("<<< CONNECT_ACK", rx_buffer_, rx_len_);
        rx_len_ = 0;
        current_type_ = 0;
        state_ = State::SCANNING;
        last_action_time_ = now;
      } else if (now - last_action_time_ > 2000) {
        ESP_LOGW(TAG, "No connect ack, retrying connect...");
        state_ = State::CONNECTING;
      }
      break;

    case State::SCANNING:
      if (current_type_ > 0xFF) {
        ESP_LOGI(TAG, "=== SCAN COMPLETE (0x00-0xFF) ===");
        state_ = State::DONE;
        break;
      }
      send_get_((uint8_t) current_type_);
      rx_len_ = 0;
      state_ = State::WAIT_REPLY;
      last_action_time_ = now;
      break;

    case State::WAIT_REPLY:
      if (rx_len_ > 0 && (now - last_byte_time_) > 150) {
        char prefix[40];
        snprintf(prefix, sizeof(prefix), "<<< REPLY to 0x%02X", (uint8_t) current_type_);
        log_packet_(prefix, rx_buffer_, rx_len_);
        current_type_++;
        state_ = State::SCANNING;
      } else if (now - last_action_time_ > 700) {
        ESP_LOGD(TAG, "    (no reply for 0x%02X)", (uint8_t) current_type_);
        current_type_++;
        state_ = State::SCANNING;
      }
      break;

    case State::DONE:
      break;
  }
}

}  // namespace ecodan_scanner
}  // namespace esphome
