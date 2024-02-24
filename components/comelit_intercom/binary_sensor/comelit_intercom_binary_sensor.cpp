#include "comelit_intercom_binary_sensor.h"

namespace esphome {
namespace comelit_intercom {

static const char *const TAG = "comelit.binary";

void ComelitIntercomBinarySensor::turn_on(uint32_t *timer, uint16_t auto_off ) {
  this->publish_state(true);
  if (auto_off > 0) *timer = millis() + (auto_off * 1000);
}

void ComelitIntercomBinarySensor::turn_off(uint32_t *timer) {
  this->publish_state(false);
  *timer = 0;
}


}  // namespace comelit_intercom
}  // namespace esphome
