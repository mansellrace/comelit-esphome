#include "comelit_intercom.h"
#include "esphome/core/log.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/application.h"
#include <Arduino.h>

namespace esphome {
namespace comelit_intercom {

static const char *const TAG = "comelit_intercom";
// delay between decoding a frame and starting its acknowledge; the frame is decoded idle_us
// after its end, real devices answer 20-80ms after the end of the frame
static const uint32_t ACK_DELAY_MS = 20;

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
  } else if (hw_version_ == HW_VERSION_TYPE_2_6 || hw_version_ == HW_VERSION_TYPE_2_7) {
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

  if (hw_version_ == HW_VERSION_TYPE_2_7) {
    pinMode(13, OUTPUT);        //D2 OPEN
    digitalWrite(13, LOW);  //D2 GND
    capacitor = true;
    time_cap = millis() + 8000;
  }

  this->rx_pin_->setup();
  this->tx_pin_->setup();
  this->tx_pin_->digital_write(false);

  if (this->tx2_enabled_) {
    this->tx2_pin_->setup();
    this->tx2_pin_->digital_write(false);
  }

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
  ESP_LOGCONFIG(TAG, "Comelit Intercom v. 2026-10-01:");
  LOG_PIN("  Pin RX: ", this->rx_pin_);
  LOG_PIN("  Pin TX: ", this->tx_pin_);
  if (this->tx2_enabled_) {
    LOG_PIN("  Pin TX2: ", this->tx2_pin_);
  } else {
    ESP_LOGCONFIG(TAG, "  Pin TX2: disabled");
  }
  
  switch (hw_version_) {
    case HW_VERSION_TYPE_2_5:
      ESP_LOGCONFIG(TAG, "  HW version: 2.5");
      break;
    case HW_VERSION_TYPE_2_6:
      ESP_LOGCONFIG(TAG, "  HW version: 2.6");
      break;
    case HW_VERSION_TYPE_2_7:
      ESP_LOGCONFIG(TAG, "  HW version: 2.7");
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
      case HW_VERSION_TYPE_2_7:
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
    } else if (hw_version_ == HW_VERSION_TYPE_2_6 || hw_version_ == HW_VERSION_TYPE_2_7) {
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
  if (simplebus_1_) {
    ESP_LOGCONFIG(TAG, "  Simplebus tx protocol: 1");
  } else {
    ESP_LOGCONFIG(TAG, "  Simplebus tx protocol: 2");
  }
  ESP_LOGCONFIG(TAG, "  Send attempts: %i", send_attempts_);
  if (strcmp(event_, "esphome.none") != 0) {
    ESP_LOGCONFIG(TAG, "  Event: %s", event_);
  } else {
    ESP_LOGCONFIG(TAG, "  Event: disabled");
  }
}

void ComelitComponent::loop() {
  uint32_t now_millis = millis();
  if (capacitor) {
    if (now_millis > time_cap) {
      digitalWrite(13, HIGH);
    }
  }
  for (auto &listener : listeners_) { 
    if (listener->timer_ && now_millis > listener->timer_) {
      listener->turn_off(&listener->timer_);
    }
  }
  if (this->sending){
    if (this->simplebus_1_){
      sending_loop_simplebus_1();
    } else {
      sending_loop_simplebus_2();
    }
    if (!this->sending) {
      this->sent_at_ = millis();
      if (this->ack_sending_) {
        this->ack_sending_ = false;
        this->settling_ = true;
      } else if (this->max_attempts_ > 1) {
        // frame sent: wait for the acknowledge, like the intercom does, before sending it again
        this->waiting_ack_ = true;
        this->retry_at_ = this->sent_at_ + 730;
      } else {
        this->settling_ = true;
      }
    }
    return;
  }

  auto &s = this->store_;
  const uint32_t write_at = s.buffer_write_at;
  const uint32_t dist = (s.buffer_size + write_at - s.buffer_read_at) % s.buffer_size;
  // acknowledge a received frame, unless another frame is still waiting to be decoded
  if (this->ack_phase_ && !this->settling_ && dist <= 1 && (int32_t) (now_millis - this->ack_at_) >= 0) {
    this->start_ack_();
    return;
  }
  // no ACK in time: send again, but not while a received frame is still waiting to be decoded
  if (this->waiting_ack_ && dist <= 1 && (int32_t) (now_millis - this->retry_at_) >= 0) {
    this->waiting_ack_ = false;
    if (this->attempt_ < this->max_attempts_) {
      this->attempt_++;
      ESP_LOGD(TAG, "No ACK, sending command %i address %i again (attempt %i of %i)", this->send_data_.command,
               this->send_data_.address, this->attempt_, this->max_attempts_);
      this->rx_pin_->detach_interrupt();
      if (capacitor) {
        digitalWrite(13, LOW);
        time_cap = millis() + 3000;
      }
      this->sending = true;
      this->preamble = true;
      return;
    }
    ESP_LOGD(TAG, "No ACK for command %i address %i after %i attempts", this->send_data_.command,
             this->send_data_.address, this->attempt_);
    this->settling_ = true;
  }
  // sending complete once the bus has been quiet for 100ms, so that the next command does not
  // overlap the answer to this one; then the automation that sent it goes on
  if (this->settling_ && now_millis - this->sent_at_ >= 150 &&
      ((dist <= 1 && micros() - s.buffer[write_at] >= 100000) || now_millis - this->sent_at_ >= 3000)) {
    this->settling_ = false;
    if (this->on_sent_ != nullptr) {
      std::function<void()> on_sent = std::move(this->on_sent_);
      this->on_sent_ = nullptr;
      on_sent();
    }
    if (this->ack_phase_) {
      this->ack_phase_ = false;
      if (this->queued_) {
        this->queued_ = false;
        this->start_send_(this->queued_data_, std::move(this->queued_on_sent_), this->queued_attempts_);
        this->queued_on_sent_ = nullptr;
      }
    }
    return;
  }
  // signals must at least one rising and one leading edge
  if (dist <= 1)
    return;
  const uint32_t now = micros();
  if (now - s.buffer[write_at] < this->idle_us_) {
    // The last change was fewer than the configured idle time ago.
    return;
  }

  // Skip first value, it's from the previous idle level
  const uint32_t last_edge = s.buffer[s.buffer_read_at];
  s.buffer_read_at = (s.buffer_read_at + 1) % s.buffer_size;
  const uint32_t skipped_us = s.buffer[s.buffer_read_at] - last_edge;
  // rx pin is high during carrier and the isr stores the inverted level: odd index = end of carrier
  const bool skipped_mark = (s.buffer_read_at % 2) == 1;
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

  const bool ack = (this->dump_raw_ || this->waiting_ack_) && is_ack(temp_);
  if (this->dump_raw_) {
    ESP_LOGD(TAG, "Received Raw with size %i, preceded by %s of %" PRIu32 " us", temp_.size(),
             skipped_mark ? "MARK" : "space", skipped_us);
    if (this->temp_.size() > 1) this->dump(temp_);
    if (ack) ESP_LOGD(TAG, "Received ACK");
  }
  if (ack && this->waiting_ack_) {
    ESP_LOGD(TAG, "ACK received for command %i address %i at attempt %i", this->send_data_.command,
             this->send_data_.address, this->attempt_);
    this->waiting_ack_ = false;
    this->settling_ = true;
  }
  if (this->temp_.size() == 76 && this->simplebus_1_ == false) {
    ESP_LOGD(TAG, "Warning! received simplebus 1 command but your transmission section is set to simplebus 2.");
    ESP_LOGD(TAG, "Maybe you need to set        simplebus_1: true");
  }
  if ((this->temp_.size() == 38) || (this->temp_.size() == 76)) {
    comelit_decode(temp_);
  }
}

void ComelitComponent::comelit_decode(std::vector<uint32_t> src) {
  char message[18];
  int bits = 0;
  if (src.size() == 38) {
    for (uint16_t i = 1; i < src.size() - 1; i = i + 2) {
      const uint32_t value = src[i];
        if (value < 3200 && value > 1000) {
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
      const uint32_t value = src[i];
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

        // before the listeners: a command sent by their automations must wait for the acknowledge
        for (auto &listener : listeners_) {
          if (listener->ack_ && listener->matches(this->command, this->address)) {
            this->schedule_ack_();
            break;
          }
        }

        if (strcmp(event_, "esphome.none") != 0) {
          ESP_LOGD(TAG, "Send event to home assistant on %s", event_);
          esphome::api::CustomAPIDevice capi;
          capi.fire_homeassistant_event(event_, {{"command", std::to_string(id(command))}, {"address", std::to_string(id(address))}});
        }
        for (auto &listener : listeners_) {
          if (!listener->matches(this->command, this->address))
            continue;
          ESP_LOGD(TAG, "Listener matched: command %i, address %i", this->command, this->address);
          listener->turn_on(&listener->timer_, listener->auto_off_);
          listener->on_command(this->command, this->address);
        }
      }
    }
  }
}

bool ComelitComponent::is_ack(std::vector<uint32_t> src) const {
  // acknowledge: 4 bursts separated by 3ms spaces, often followed by a short spike
  if (src.size() < 8) return false;
  for (uint16_t i = 0; i < 7; i++) {
    const uint32_t value = src[i];
    if (i % 2 == 0) {
      if (!(value < 6200 && value > 3500)) return false;
    } else {
      if (!(value < 3200 && value > 1000)) return false;
    }
  }
  // a command starting with three 0 bits goes on with a bit space and a full burst
  if (src.size() > 8 && src[7] < 6200 && src[8] < 6200 && src[8] > 3500) return false;
  return true;
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

void ComelitComponent::dump(std::vector<uint32_t> src) const {
  char buffer[128];
  uint32_t buffer_offset = 0;
  buffer_offset += sprintf(buffer, "Raw: ");
  for (uint16_t i = 0; i < src.size() - 1; i++) {
    const uint32_t value = src[i];
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


void ComelitComponent::schedule_ack_() {
  if (this->sending || this->waiting_ack_ || this->settling_ || this->ack_phase_) {
    ESP_LOGD(TAG, "ACK for command %i address %i skipped, another sending is in progress", this->command,
             this->address);
    return;
  }
  this->ack_data_.command = this->command;
  this->ack_data_.address = this->address;
  this->ack_phase_ = true;
  this->ack_at_ = millis() + ACK_DELAY_MS;
}

void ComelitComponent::start_ack_() {
  ESP_LOGD(TAG, "Sending ACK for command %i address %i", this->ack_data_.command, this->ack_data_.address);
  this->rx_pin_->detach_interrupt();
  if (capacitor) {
    digitalWrite(13, LOW);
    time_cap = millis() + 3000;
  }
  // acknowledge: 4 bursts separated by 3ms spaces, that is four 0 bits without the start pulse
  for (int i = 0; i < 4; i++) {
    this->send_buffer[i] = false;
  }
  this->send_length_ = 4;
  this->send_index = 0;
  const uint32_t now = micros();
  this->send_next_bit = now + 3000;
  this->send_next_change = this->simplebus_1_ ? now + 3020 : now + 20;
  this->preamble = false;
  this->ack_sending_ = true;
  this->sending = true;
}

bool ComelitComponent::send_command(ComelitIntercomData data, std::function<void()> on_sent, uint8_t send_attempts) {
  if (this->ack_phase_ && !this->queued_) {
    // a frame received just now is being acknowledged: this command follows the acknowledge
    ESP_LOGD(TAG, "Command %i address %i will be sent after the ACK", data.command, data.address);
    this->queued_ = true;
    this->queued_data_ = data;
    this->queued_on_sent_ = std::move(on_sent);
    this->queued_attempts_ = send_attempts;
    return true;
  }
  if (this->sending || this->waiting_ack_ || this->settling_ || this->ack_phase_){
    ESP_LOGD(TAG, "Sending of command %i address %i cancelled, another sending is in progress", data.command, data.address);
    return false;
  }
  this->start_send_(data, std::move(on_sent), send_attempts);
  return true;
}

void ComelitComponent::start_send_(ComelitIntercomData data, std::function<void()> on_sent, uint8_t send_attempts) {
  if (this->simplebus_1_){
    ESP_LOGD(TAG, "Simplebus 1: Sending command %i, address %i", data.command, data.address);
  } else {
    ESP_LOGD(TAG, "Simplebus 2: Sending command %i, address %i", data.command, data.address);
  }
  this->rx_pin_->detach_interrupt();
  int checksum_counter = 0;
  if (capacitor) {
    digitalWrite(13, LOW);
    time_cap = millis() + 3000;
  }

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
  this->send_length_ = 19;
  this->send_data_ = data;
  this->attempt_ = 1;
  this->max_attempts_ = send_attempts > 0 ? send_attempts : this->send_attempts_;
  this->on_sent_ = std::move(on_sent);
  this->sending = true;
  this->preamble = true;
}

void ComelitComponent::sending_loop_simplebus_2() {
  uint32_t now = micros();
  if (this->preamble) {
    if (this->send_next_bit == 0 && this->send_next_change == 0) {  // initializing
      this->tx_pin_->digital_write(true);
      if (this->tx2_enabled_) {
        this->tx2_pin_->digital_write(true);
      }
      this->send_next_bit = now + 3000;
      this->send_next_change = now + 20;
      while (this->send_next_bit >= micros()) {
        if (this->send_next_change < micros()) {
          this->tx_pin_->digital_write(!this->tx_pin_->digital_read());
          if (this->tx2_enabled_) {
            this->tx2_pin_->digital_write(!this->tx2_pin_->digital_read());
          }
          this->send_next_change = this->send_next_change + 20;
        }
      }
      this->send_next_bit = 0;
      this->send_next_change = this->send_next_change + 16000;
      this->tx_pin_->digital_write(false);
      if (this->tx2_enabled_) {
        this->tx2_pin_->digital_write(false);
      }
      return;
    } else {                                     // long pause of initializing
      if (now < this->send_next_change) return;
      this->send_next_bit = now + 3000;
      this->send_next_change = now + 20;
      this->preamble = false;
    }
  } else {                                       // bit sending routine, preamble ended
    if (this->send_index < this->send_length_) {
      if (this->send_next_change > 0) {           // carrier generation
        while (this->send_next_bit >= micros()) {
          if (this->send_next_change < micros()) {
            this->tx_pin_->digital_write(!this->tx_pin_->digital_read());
            if (this->tx2_enabled_) {
              this->tx2_pin_->digital_write(!this->tx2_pin_->digital_read());
            }
            this->send_next_change = this->send_next_change + 20;
          }
        }
        this->send_next_change = 0;
        this->tx_pin_->digital_write(false);
        if (this->tx2_enabled_) {
          this->tx2_pin_->digital_write(false);
        }
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
      if (this->tx2_enabled_) {
        this->tx2_pin_->digital_write(false);
      }
      this->send_next_bit = 0;
      this->send_next_change = 0;
      this->send_index = 0;
      this->rx_pin_->attach_interrupt(ComelitComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
    }
  }
}

void ComelitComponent::sending_loop_simplebus_1() {
  uint32_t now = micros();
  if (this->preamble) {
    if (this->send_next_bit == 0 && this->send_next_change == 0) {  // initializing
      this->tx_pin_->digital_write(true);
      if (this->tx2_enabled_) {
        this->tx2_pin_->digital_write(true);
      }
      this->send_next_bit = now + 3000;
      while (this->send_next_bit >= micros()) {
      }
      this->send_next_bit = 0;
      this->send_next_change = now + 3000 + 16000;  // 16ms pause after the 3ms start pulse, as in simplebus 2
      this->tx_pin_->digital_write(false);
      if (this->tx2_enabled_) {
        this->tx2_pin_->digital_write(false);
      }
      return;
    } else {                                     // long pause of initializing
      if (now < this->send_next_change) return;
      this->send_next_bit = now + 3000;
      this->send_next_change = now + 3020;
      this->preamble = false;
    }
  } else {                                       // bit sending routine, preamble ended
    if (this->send_index < this->send_length_) {
      if (this->send_next_change > 0) {           // carrier generation
        this->tx_pin_->digital_write(true);
        if (this->tx2_enabled_) {
          this->tx2_pin_->digital_write(true);
        }
        while (this->send_next_bit >= micros()) {
        }
        this->send_next_change = 0;
        this->tx_pin_->digital_write(false);
        if (this->tx2_enabled_) {
          this->tx2_pin_->digital_write(false);
        }
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
      if (this->tx2_enabled_) {
        this->tx2_pin_->digital_write(false);
      }
      this->send_next_bit = 0;
      this->send_next_change = 0;
      this->send_index = 0;
      this->rx_pin_->attach_interrupt(ComelitComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
    }
  }
}


}  // namespace comelit_intercom
}  // namespace esphome
