#pragma once

#include <utility>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/automation.h"


namespace esphome {
namespace comelit_intercom {

enum HardwareType {
    HW_VERSION_TYPE_2_5,
    HW_VERSION_TYPE_2_6,
    HW_VERSION_TYPE_OLDER,
};

enum LanguageType {
    LANGUAGE_DISABLED,
    LANGUAGE_PLAIN_COMMAND,
    LANGUAGE_ITALIAN,
    LANGUAGE_ENGLISH,
};

struct ComelitIntercomData {
  uint16_t command;
  uint16_t address;
};

class ComelitIntercomListener {
  public:
    void set_command(uint16_t command) { this->command_ = command; }
    void set_address(uint16_t address) { this->address_ = address; }
    void set_auto_off(uint16_t auto_off) { this->auto_off_ = auto_off; }

    virtual void turn_on(uint32_t *timer, uint16_t auto_off){};
    virtual void turn_off(uint32_t *timer){};
    uint32_t timer_;
  //private:
    uint16_t address_;
    uint16_t command_;
    uint16_t auto_off_;
};

struct ComelitComponentStore {
  static void gpio_intr(ComelitComponentStore *arg);

  /// Stores the time (in micros) that the leading/falling edge happened at
  ///  * An even index means a falling edge appeared at the time stored at the index
  ///  * An uneven index means a rising edge appeared at the time stored at the index
  volatile uint32_t *buffer{nullptr};
  /// The position last written to
  volatile uint32_t buffer_write_at;
  /// The position last read from
  uint32_t buffer_read_at{0};
  bool overflow{false};
  uint32_t buffer_size{400};
  uint16_t filter_us{500};
  ISRInternalGPIOPin rx_pin;
};

class ComelitComponent : public Component {
 public:
  void comelit_decode(std::vector<uint16_t> src);
  void dump(std::vector<uint16_t>) const;
  std::string logbook_gen();
  void sending_loop_simplebus_2();
  void sending_loop_simplebus_1();

  void set_rx_pin(InternalGPIOPin *pin) { rx_pin_ = pin; }
  void set_tx_pin(InternalGPIOPin *pin) { tx_pin_ = pin; }
  void set_tx2_pin(InternalGPIOPin *pin) { tx2_pin_ = pin; }
  void set_tx2_enabled(bool enabled) { this->tx2_enabled_ = enabled; }
  void set_hw_version(HardwareType hw_version) { hw_version_ = hw_version; }
  void set_logbook_language(LanguageType logbook_language) { logbook_language_ = logbook_language; }
  void set_logbook_entity(const char *logbook_entity) { logbook_entity_ = logbook_entity; }
  void set_sensitivity(const char *sensitivity) { sensitivity_ = sensitivity; }
  void set_buffer_size(uint32_t buffer_size) { this->buffer_size_ = buffer_size; }
  void set_filter_us(uint16_t filter_us) { this->filter_us_ = filter_us; }
  void set_idle_us(uint32_t idle_us) { this->idle_us_ = idle_us; }
  void set_dump(bool dump_raw) { this->dump_raw_ = dump_raw; }
  void set_event(const char *event) { this->event_ = event; }
  void set_simplebus_1(bool simplebus_1) { this->simplebus_1_ = simplebus_1; }

  void setup() override;
  void dump_config() override;
  void loop() override;
  uint16_t command, address;
  void register_listener(ComelitIntercomListener *listener);
  void send_command(ComelitIntercomData data);
  bool send_buffer[19];
  bool sending, preamble;
  int send_index;
  uint32_t send_next_bit;
  uint32_t send_next_change;

 protected:
  InternalGPIOPin *rx_pin_;
  InternalGPIOPin *tx_pin_;
  InternalGPIOPin *tx2_pin_;
  bool tx_2_enabled_ = false; 
  HardwareType hw_version_;
  LanguageType logbook_language_;
  const char* logbook_entity_;
  const char* sensitivity_;
  const char* event_;
  bool dump_raw_;
  bool simplebus_1_;
  ComelitComponentStore store_;
  uint16_t filter_us_{10};
  uint32_t idle_us_{10000};
  uint32_t buffer_size_{};

  HighFrequencyLoopRequester high_freq_;
  std::vector<uint16_t> temp_;
  std::vector<ComelitIntercomListener *> listeners_{};
};

template<typename... Ts> class ComelitIntercomSendAction : public Action<Ts...> {
 public:
  ComelitIntercomSendAction(ComelitComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, command)
  TEMPLATABLE_VALUE(uint16_t, address)

  void play(Ts... x) {
    ComelitIntercomData data{};
    data.command = this->command_.value(x...);
    data.address = this->address_.value(x...);
    this->parent_->send_command(data);
  }

 protected:
  ComelitComponent *parent_;
};


}  // namespace comelit_intercom
}  // namespace esphome
