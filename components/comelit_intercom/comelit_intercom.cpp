#include "comelit_intercom.h"
#include "esphome/core/log.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/application.h"
#include <Arduino.h>

namespace esphome {
namespace comelit_intercom {

static const char *const TAG = "comelit_intercom";

ComelitComponent *global_comelit_intercom = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void ComelitComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Comelit Intercom...");

  if (hw_version_ == HW_VERSION_TYPE_2_5) {
    pinMode(16, INPUT);        //D0 OPEN
    if (strcmp(sensitivity_, "low") == 0) {
      pinMode(14, INPUT);      //D5 OPEN
    } else {
      pinMode(14, OUTPUT);     //D5 GND
      digitalWrite(14, LOW);   //D5 GND
    }
  } else if (hw_version_ == HW_VERSION_TYPE_2_6) {
    if (strcmp(sensitivity_, "1") == 0) {
      pinMode(14, OUTPUT);     //D5 3.3V
      digitalWrite(14, HIGH);  //D5 3.3V
      pinMode(16, OUTPUT);     //D0 3.3V
      digitalWrite(16, HIGH);  //D0 3.3V
    } else if (strcmp(sensitivity_, "2") == 0) {
      pinMode(14, OUTPUT);     //D5 3.3V
      digitalWrite(14, HIGH);  //D5 3.3V
      pinMode(16, INPUT);      //D0 OPEN
    } else if (strcmp(sensitivity_, "3") == 0) {
      pinMode(14, OUTPUT);     //D5 3.3V
      digitalWrite(14, HIGH);  //D5 3.3V
      pinMode(16, OUTPUT);     //D0 GND
      digitalWrite(16, LOW);   //D0 GND
    } else if (strcmp(sensitivity_, "4") == 0) {
      pinMode(14, INPUT);      //D5 OPEN
      pinMode(16, OUTPUT);     //D0 3.3V
      digitalWrite(16, HIGH);  //D0 3.3V
    } else if (strcmp(sensitivity_, "5") == 0) {
      pinMode(14, OUTPUT);     //D5 GND
      digitalWrite(14, LOW);   //D5 GND
      pinMode(16, OUTPUT);     //D0 3.3V
      digitalWrite(16, HIGH);  //D0 3.3V
    } else if (strcmp(sensitivity_, "6") == 0) {
      pinMode(14, INPUT);      //D5 OPEN
      pinMode(16, INPUT);      //D0 OPEN
    } else if (strcmp(sensitivity_, "7") == 0) {
      pinMode(14, INPUT);      //D5 OPEN
      pinMode(16, OUTPUT);     //D0 GND
      digitalWrite(16, LOW);   //D0 GND
    } else if (strcmp(sensitivity_, "8") == 0) {
      pinMode(14, OUTPUT);     //D5 GND
      digitalWrite(14, LOW);   //D5 GND
      pinMode(16, INPUT);      //D0 OPEN
    } else if (strcmp(sensitivity_, "9") == 0) {
      pinMode(14, OUTPUT);     //D5 GND
      digitalWrite(14, LOW);   //D5 GND
      pinMode(16, OUTPUT);     //D0 GND
      digitalWrite(16, LOW);   //D0 GND
    } else if (strcmp(sensitivity_, "default") == 0) {  //default = 8
      pinMode(14, OUTPUT);     //D5 GND
      digitalWrite(14, LOW);   //D5 GND
      pinMode(16, INPUT);      //D0 OPEN
    }
  }

  this->rx_pin_->setup();
  this->tx_pin_->setup();
  this->tx_pin_->digital_write(false);

  auto &s = this->store_;
  s.filter_us = this->filter_us_;
  s.buffer_size = this->buffer_size_;

  this->high_freq_.start();
  if (s.buffer_size % 2 != 0) {
    // Make sure divisible by two. This way, we know that every 0bxxx0 index is a space and every 0bxxx1 index is a mark
    s.buffer_size++;
  }

  s.buffer = new uint32_t[s.buffer_size];
  void *buf = (void *) s.buffer;
  memset(buf, 0, s.buffer_size * sizeof(uint32_t));

  s.rx_pin = this->rx_pin_->to_isr();
  //s.reset();

  this->rx_pin_->attach_interrupt(ComelitComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);

  for (auto &listener : listeners_) {
      listener->turn_off(&listener->timer_);
  }
}

void ComelitComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Comelit Intercom TEST VERSION 2.1.0:");
  LOG_PIN("  Pin RX: ", this->rx_pin_);
  LOG_PIN("  Pin TX: ", this->tx_pin_);
  switch (hw_version_) {
    case HW_VERSION_TYPE_2_5:
      ESP_LOGCONFIG(TAG, "  HW version: 2.5");
      break;
    case HW_VERSION_TYPE_2_6:
      ESP_LOGCONFIG(TAG, "  HW version: 2.6");
      break;
    case HW_VERSION_TYPE_OLDER:
      ESP_LOGCONFIG(TAG, "  HW version: older");
      break;
  }
  if (strcmp(sensitivity_, "default") == 0) {
    switch (hw_version_) {
      case HW_VERSION_TYPE_2_5:
        ESP_LOGCONFIG(TAG, "  Sensitivity: default (high) 107mV");
        break;
      case HW_VERSION_TYPE_2_6:
        ESP_LOGCONFIG(TAG, "  Sensitivity: default (8) 108mV");
        break;
      case HW_VERSION_TYPE_OLDER: break;
    }
  } else {
    if (hw_version_ == HW_VERSION_TYPE_2_5) {
      if (strcmp(sensitivity_, "high") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: high  -  107mV");
      } else if (strcmp(sensitivity_, "low") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: low  -  205mV");
      }
    } else if (hw_version_ == HW_VERSION_TYPE_2_6) {
      if (strcmp(sensitivity_, "1") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 1  -  2025mV");
      } else if (strcmp(sensitivity_, "2") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 2  -  1695mV");
      } else if (strcmp(sensitivity_, "3") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 3  -  1377mV");
      } else if (strcmp(sensitivity_, "4") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 4  -  1184mV");
      } else if (strcmp(sensitivity_, "5") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 5  -  736mV");
      } else if (strcmp(sensitivity_, "6") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 6  -  200mV");
      } else if (strcmp(sensitivity_, "7") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 7  -  141mV");
      } else if (strcmp(sensitivity_, "8") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 8  -  108mV");
      } else if (strcmp(sensitivity_, "9") == 0) {
        ESP_LOGCONFIG(TAG, "  Sensitivity: 9  -  88mV");
      }
    }
  }
  ESP_LOGCONFIG(TAG, "  Filter: %ius", filter_us_);
  ESP_LOGCONFIG(TAG, "  Idle:   %ius", idle_us_);
  ESP_LOGCONFIG(TAG, "  Buffer: %ib", buffer_size_);
  if (dump_raw_) ESP_LOGCONFIG(TAG, "  Dump raw value: True");
  if (strcmp(event_, "esphome.none") != 0) {
    ESP_LOGCONFIG(TAG, "  Event: %s", event_);
  } else {
    ESP_LOGCONFIG(TAG, "  Event: disabled");
  }
}

void ComelitComponent::loop() {
  uint32_t now_millis = millis();
  for (auto &listener : listeners_) { 
    if (listener->timer_ && now_millis > listener->timer_) {
      listener->turn_off(&listener->timer_);
    }
  }
  if (this->sending){
    if (this->send_76){
      sending_loop_76();
    } else {
      sending_loop();
    }
    return;
  }

  auto &s = this->store_;
  const uint32_t write_at = s.buffer_write_at;
  const uint32_t dist = (s.buffer_size + write_at - s.buffer_read_at) % s.buffer_size;
  // signals must at least one rising and one leading edge
  if (dist <= 1)
    return;
  const uint32_t now = micros();
  if (now - s.buffer[write_at] < this->idle_us_) {
    // The last change was fewer than the configured idle time ago.
    return;
  }

  // Skip first value, it's from the previous idle level
  s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;
  uint32_t prev = s.buffer_read_at;
  s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;
  const uint32_t reserve_size = 1 + (s.buffer_size + write_at - s.buffer_read_at) % s.buffer_size;
  this->temp_.clear();
  this->temp_.reserve(reserve_size);

  for (uint32_t i = 0; prev != write_at; i++) {
    int32_t delta = s.buffer[s.buffer_read_at] - s.buffer[prev];
    if (uint32_t(delta) >= this->idle_us_) {
      // already found a space longer than idle. There must have been two pulses
      break;
    }
    this->temp_.push_back(delta);
    prev = s.buffer_read_at;
    s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;
  }
  s.buffer_read_at = (s.buffer_size + s.buffer_read_at - 1) % s.buffer_size;
  this->temp_.push_back(this->idle_us_);

  if (this->temp_.size() > 1 && this->dump_raw_) {
    ESP_LOGD(TAG, "Received Raw with size %i", temp_.size());
    this->dump(temp_);
  }
  if (this->temp_.size() == 76 && !this->dump_raw_) {
    ESP_LOGD(TAG, "Received Raw with size 76");
  }
  if ((this->temp_.size() == 38) || (this->temp_.size() == 76)) {
    comelit_decode(temp_);
  }
}

void ComelitComponent::comelit_decode(std::vector<uint16_t> src) {
  if (strcmp(event_, "esphome.none") != 0 || logbook_language_ != LANGUAGE_DISABLED) {

  }
  auto capi = new esphome::api::CustomAPIDevice();
  char message[18];
  int bits = 0;
  if (src.size() == 38) {
    for (uint16_t i = 1; i < src.size() - 1; i = i + 2) {
      const uint16_t value = src[i];
        if (value < 2500 && value > 1000) {
          message[bits] = 0;
          bits += 1;
        }
        else if (value < 6200 && value > 3500) {
          message[bits] = 1;
          bits += 1;
        }
    }
  } else if (src.size() == 76) {
    for (uint16_t i = 3; i < src.size() - 1; i = i + 4) {
      const uint16_t value = src[i];
        if (value < 2500 && value > 1000) {
          message[bits] = 0;
          bits += 1;
        }
        else if (value < 6200 && value > 3500) {
          message[bits] = 1;
          bits += 1;
        }
    }
  }

  if (bits == 18) {
    int sum = 0;
    int checksum = 0;
    for (int i = 0; i < 14; i++) {
      if (message[i] == 1) sum++;
    }
    checksum = (message[17] * 8) + (message[16] * 4) + (message[15] * 2) + message[14];
    if (checksum == sum) {
      int msgAddr[6];
      int msgCode[8];
      for (int j = 0; j < 14; j++) {
        if (j < 6)  msgAddr[j] = message[j];
        else msgCode[j - 6] = message[j];
      }
      this->command = (msgAddr[5] * 32) + (msgAddr[4] * 16) + (msgAddr[3] * 8) + (msgAddr[2] * 4) + (msgAddr[1] * 2) + msgAddr[0];
      this->address = (msgCode[7] * 128) + (msgCode[6] * 64) + (msgCode[5] * 32) + (msgCode[4] * 16) + (msgCode[3] * 8) + (msgCode[2] * 4) + (msgCode[1] * 2) + msgCode[0];
      if (this->command != 63){
        ESP_LOGD(TAG, "Received command %i, address %i", this->command, this->address);
        
        if (strcmp(event_, "esphome.none") != 0) {
          ESP_LOGD(TAG, "Send event to home assistant on %s", event_);
          capi->fire_homeassistant_event(event_, {{"command", std::to_string(id(command))}, {"address", std::to_string(id(address))}});
        }
        for (auto &listener : listeners_) {
          if (listener->command_ == command && listener->address_ == address) {
            ESP_LOGD(TAG, "Binary sensor fired! %i %i", listener->command_, listener->address_);
            listener->turn_on(&listener->timer_, listener->auto_off_);
          }
        }
        if (logbook_language_ != LANGUAGE_DISABLED) {
          if (strcmp(logbook_entity_, "none") != 0) {
            capi->call_homeassistant_service("logbook.log", {
              {"name", "Comelit"},
              {"message", logbook_gen()},
              {"entity_id", logbook_entity_},
              });
          } else {
            capi->call_homeassistant_service("logbook.log", {
              {"name", "Comelit"},
              {"message", logbook_gen()},
              });
          }
        }
      }
    }
  }
}


std::string ComelitComponent::logbook_gen() {
  return "ciao";
}

void IRAM_ATTR HOT ComelitComponentStore::gpio_intr(ComelitComponentStore *arg) {
  const uint32_t now = micros();
  // If the lhs is 1 (rising edge) we should write to an uneven index and vice versa
  const uint32_t next = (arg->buffer_write_at + 1) % arg->buffer_size;
  const bool level = !arg->rx_pin.digital_read();
  if (level != next % 2)
    return;

  // If next is buffer_read, we have hit an overflow
  if (next == arg->buffer_read_at)
    return;

  const uint32_t last_change = arg->buffer[arg->buffer_write_at];
  const uint32_t time_since_change = now - last_change;
  if (time_since_change <= arg->filter_us)
    return;

  arg->buffer[arg->buffer_write_at = next] = now;
}

void ComelitComponent::dump(std::vector<uint16_t> src) const {
  char buffer[128];
  uint32_t buffer_offset = 0;
  buffer_offset += sprintf(buffer, "Raw: ");
  for (uint16_t i = 0; i < src.size() - 1; i++) {
    const uint16_t value = src[i];
    const uint32_t remaining_length = sizeof(buffer) - buffer_offset;
    int written;

    if (i + 1 < src.size() - 1) {
      written = snprintf(buffer + buffer_offset, remaining_length, "%" PRId32 ", ", value);
    } else {
      written = snprintf(buffer + buffer_offset, remaining_length, "%" PRId32, value);
    }

    if (written < 0 || written >= int(remaining_length)) {
      // write failed, flush...
      buffer[buffer_offset] = '\0';
      ESP_LOGD(TAG, "%s", buffer);
      buffer_offset = 0;
      written = sprintf(buffer, "  ");
      if (i + 1 < src.size()) {
        written += sprintf(buffer + written, "%" PRId32 ", ", value);
      } else {
        written += sprintf(buffer + written, "%" PRId32, value);
      }
    }

    buffer_offset += written;
  }
  if (buffer_offset != 0) {
    ESP_LOGD(TAG, "%s", buffer);
  }
}

void ComelitComponent::register_listener(ComelitIntercomListener *listener) {
    this->listeners_.push_back(listener); 
  }


void ComelitComponent::send_command(ComelitIntercomData data) {
  if (this->sending){
    ESP_LOGD(TAG, "Sending of command %i address %i cancelled, another sending is in progress", data.command, data.address);
    //return;
  }
  if (data.send_76){
    ESP_LOGD(TAG, "Sending 76 bit command %i, address %i", data.command, data.address);
    this->send_76 = true;
  } else {
    ESP_LOGD(TAG, "Sending command %i, address %i", data.command, data.address);
    this->send_76 = false;
  }
  this->rx_pin_->detach_interrupt();
  int checksum_counter = 0;

  for (int i=0; i<6; i++){
    if (bitRead(data.command, i)) {
      this->send_buffer[this->send_index] = true;
      checksum_counter++;
    } else {
      this->send_buffer[this->send_index] = false;
    }
    this->send_index++;
  }

  for (int i=0; i<8; i++) {
    if (bitRead(data.address, i)) {
      this->send_buffer[this->send_index] = true;
      checksum_counter++;
    } else {
      this->send_buffer[this->send_index] = false;
    }
    this->send_index++;
  }

  for (int i=0; i<4; i++) {
    if (bitRead(checksum_counter, i)) {
      this->send_buffer[this->send_index] = true;
    } else {
      this->send_buffer[this->send_index] = false;
    }
    this->send_index++;
  }
  this->send_buffer[this->send_index] = false;

  this->send_index = 0;
  this->sending = true;
  this->preamble = true;
}

void ComelitComponent::sending_loop() {
  uint32_t now = micros();
  if (this->preamble) {
    if (this->send_next_bit == 0 && this->send_next_change == 0) {  // initializing
      this->tx_pin_->digital_write(true);
      this->send_next_bit = now + 3000;
      this->send_next_change = now + 20;
      while (this->send_next_bit >= micros()) {
        if (this->send_next_change < micros()) {
          this->tx_pin_->digital_write(!this->tx_pin_->digital_read());
          this->send_next_change = this->send_next_change + 20;
        }
      }
      this->send_next_bit = 0;
      this->send_next_change = this->send_next_change + 16000;
      this->tx_pin_->digital_write(false);
      return;
    } else {                                     // long pause of initializing
      if (now < this->send_next_change) return;
      this->send_next_bit = now + 3000;
      this->send_next_change = now + 20;
      this->preamble = false;
    }
  } else {                                       // bit sending routine, preamble ended
    if (this->send_index < 19) {
      if (this->send_next_change > 0) {           // carrier generation
        while (this->send_next_bit >= micros()) {
          if (this->send_next_change < micros()) {
            this->tx_pin_->digital_write(!this->tx_pin_->digital_read());
            this->send_next_change = this->send_next_change + 20;
          }
        }
        this->send_next_change = 0;
        this->tx_pin_->digital_write(false);
        if (this->send_buffer[this->send_index]) {
          this->send_next_bit = this->send_next_bit + 6000;
        } else {
          this->send_next_bit = this->send_next_bit + 3000;
        }
      } else {                                    // no signal generation
        if (now < this->send_next_bit) return;
        this->send_next_bit = now + 3000;
        this->send_next_change = now + 20;
        this->send_index++;
      }
    } else {                                      // end of transmission
      this->sending = false;
      this->tx_pin_->digital_write(false);
      this->send_next_bit = 0;
      this->send_next_change = 0;
      this->send_index = 0;
      this->rx_pin_->attach_interrupt(ComelitComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
    }
  }
}

void ComelitComponent::sending_loop_76() {
  uint32_t now = micros();
  if (this->preamble) {
    if (this->send_next_bit == 0 && this->send_next_change == 0) {  // initializing
      this->tx_pin_->digital_write(true);
      this->send_next_bit = now + 3000;
      while (this->send_next_bit >= micros()) {
      }
      this->send_next_bit = 0;
      this->send_next_change = now + 16000;
      this->tx_pin_->digital_write(false);
      return;
    } else {                                     // long pause of initializing
      if (now < this->send_next_change) return;
      this->send_next_bit = now + 3000;
      this->send_next_change = now + 3020;
      this->preamble = false;
    }
  } else {                                       // bit sending routine, preamble ended
    if (this->send_index < 19) {
      if (this->send_next_change > 0) {           // carrier generation
        this->tx_pin_->digital_write(true);
        while (this->send_next_bit >= micros()) {
        }
        this->send_next_change = 0;
        this->tx_pin_->digital_write(false);
        if (this->send_buffer[this->send_index]) {
          this->send_next_bit = this->send_next_bit + 6000;
        } else {
          this->send_next_bit = this->send_next_bit + 3000;
        }
      } else {                                    // no signal generation
        if (now < this->send_next_bit) return;
        this->send_next_bit = now + 3000;
        this->send_next_change = now + 3020;
        this->send_index++;
      }
    } else {                                      // end of transmission
      this->sending = false;
      this->tx_pin_->digital_write(false);
      this->send_next_bit = 0;
      this->send_next_change = 0;
      this->send_index = 0;
      this->rx_pin_->attach_interrupt(ComelitComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
    }
  }
}


}  // namespace comelit_intercom
}  // namespace esphome