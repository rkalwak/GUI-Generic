#ifndef _WmbusMeter_h
#define _WmbusMeter_h
#include <Arduino.h>

#include <rf_mbus.hpp>
#include <DriversWmbusMeters/utils.h>
#include <DriversWmbusMeters/meters_common_implementation.h>
#include <SensorBase.h>

#include <stdint.h>
#include <string>
#include <vector>
#include <supla/element.h>
namespace Supla {
namespace Sensor {
class WmbusMeter : public Element {
 public:
  WmbusMeter(uint8_t mosi = 23, uint8_t miso = 19, uint8_t clk = 18, uint8_t cs = 5, uint8_t gdo0 = 4, uint8_t gdo2 = 2);

  void onFastTimer() override;
  std::map<std::string, SensorBase *> sensors_{};
  void add_sensor(SensorBase *sensor);
  float parse_frame(std::vector<unsigned char> &frame);

 private:
  float lastReadValue = 0.0;
  int packetLength = 192;
  rf_mbus receiver;
};
};  // namespace Sensor
};  // namespace Supla

extern Supla::Sensor::WmbusMeter *meter;
#endif