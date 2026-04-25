#ifdef SUPLA_CC1101
#ifndef _WmbusMeter_h
#define _WmbusMeter_h
#include <Arduino.h>

#include <rf_mbus.hpp>
#include <Drivers/driver.h>
#include <utils.hpp>
#include <SensorBase.h>

#include <stdint.h>
#include <string>
#include <vector>
#include <supla/element.h>
namespace Supla {
namespace Sensor {
class WmbusMeter : public Element {
 public:
  WmbusMeter(uint8_t mosi = 23, uint8_t miso = 19, uint8_t clk = 18, uint8_t cs = 5, uint8_t gdo0 = 4, uint8_t gdo2 = 2, bool debugMode=false);

  void onFastTimer() override;
  std::map<std::string, Driver *> drivers_{};
  std::multimap<std::string, SensorBase *> sensors_{};
  void add_driver(Driver *driver);
  void add_sensor(SensorBase *sensor);
  std::string extract_meter_id_string(const std::vector<unsigned char> &frame);
  bool decrypt_telegram(std::vector<unsigned char> &telegram, std::vector<unsigned char> key);
  float parse_frame(std::vector<unsigned char> &frame);
  int8_t getLastRssi() const { return lastRssi; }
  uint8_t getLastLqi()  const { return lastLqi; }

 private:
  float lastReadValue = 0.0;
  int8_t  lastRssi = 0;
  uint8_t lastLqi  = 0;
  int packetLength = 192;
  rf_mbus receiver;
};
};  // namespace Sensor
};  // namespace Supla

extern Supla::Sensor::WmbusMeter *meter;
#endif
#endif