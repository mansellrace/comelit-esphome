#pragma once

#include <algorithm>
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
    HW_VERSION_TYPE_2_7,
    HW_VERSION_TYPE_OLDER,
};

struct ComelitIntercomData {
  uint16_t command;
  uint16_t address;
};

class ComelitIntercomListener {
  public:
    void set_command(uint16_t command) { this->command_ = command; }
    template<typename V> void set_address(V address) { this->address_ = address; }
    void set_auto_off(uint16_t auto_off) { this->auto_off_ = auto_off; }

    /// Return true when a frame concerns this listener. The default matches one exact
    /// command/address pair; the command is checked first so that a templated address is
    /// only evaluated when it can matter.
    virtual bool matches(uint16_t command, uint16_t address) {
      return this->command_ == command && this->address_.value() == address;
    }

    virtual void turn_on(uint32_t *timer, uint16_t auto_off){};
    virtual void turn_off(uint32_t *timer){};
    /// Called for every matching frame, with the values that were actually received.
    virtual void on_command(uint16_t command, uint16_t address){};
    // A listener that does not use these (an event entity has nothing to turn off)
    // must still not leave loop() reading indeterminate values.
    uint32_t timer_{0};
  //private:
    TemplatableValue<uint16_t> address_;
    uint16_t command_{0};
    uint16_t auto_off_{0};
};

/// Listener driven by a list of commands; an empty list accepts every command.
class ComelitIntercomFilteredListener : public ComelitIntercomListener {
 public:
  void set_commands(std::vector<uint8_t> commands) { this->commands_ = std::move(commands); }

 protected:
  static bool accepts(const std::vector<uint8_t> &list, uint16_t value) {
    return list.empty() || std::find(list.begin(), list.end(), value) != list.end();
  }
  std::vector<uint8_t> commands_;
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
  void comelit_decode(std::vector<uint32_t> src);
  void dump(std::vector<uint32_t>) const;
  bool is_ack(std::vector<uint32_t> src) const;
  void sending_loop_simplebus_2();
  void sending_loop_simplebus_1();

  void set_rx_pin(InternalGPIOPin *pin) { rx_pin_ = pin; }
  void set_tx_pin(InternalGPIOPin *pin) { tx_pin_ = pin; }
  void set_tx2_pin(InternalGPIOPin *pin) { tx2_pin_ = pin; }
  void set_tx2_enabled(bool enabled) { this->tx2_enabled_ = enabled; }
  void set_hw_version(HardwareType hw_version) { hw_version_ = hw_version; }
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
  bool tx2_enabled_ = false; 
  HardwareType hw_version_;
  const char* sensitivity_;
  const char* event_;
  bool dump_raw_;
  bool simplebus_1_;
  ComelitComponentStore store_;
  uint16_t filter_us_{10};
  uint32_t idle_us_{10000};
  uint32_t buffer_size_{};
  uint32_t time_cap{0};
  bool capacitor{false};

  HighFrequencyLoopRequester high_freq_;
  std::vector<uint32_t> temp_;
  std::vector<ComelitIntercomListener *> listeners_{};
};

template<typename... Ts> class ComelitIntercomSendAction : public Action<Ts...> {
 public:
  ComelitIntercomSendAction(ComelitComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint16_t, command)
  TEMPLATABLE_VALUE(uint16_t, address)

  void play(const Ts &... x) override {
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
