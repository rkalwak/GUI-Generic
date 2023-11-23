#ifndef SuplaSensorDS18B20_h
#define SuplaSensorDS18B20_h

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWireNg.h>

#include <supla-common/log.h>
#include <supla/sensor/thermometer.h>
enum class OperationType
{
  CONVERSION,
  READ
};

class DS18B20 : public Supla::Sensor::Thermometer {
 private:
  OneWire oneWire;
  DallasTemperature sensors;
  double lastValidValue;
  uint8_t retryCounter;
  unsigned long lastUpdateTime;
  OperationType lastOperationType = OperationType::CONVERSION;

  uint8_t address[8];

 public:
  DS18B20(uint8_t pin, uint8_t* deviceAddress = nullptr);
  void iterateAlways();
  double getValue();
  void onInit();
  void setDeviceAddress(uint8_t* deviceAddress);
};

void findAndSaveDS18B20Addresses();

#endif  // SuplaSensorDS18B20_h
