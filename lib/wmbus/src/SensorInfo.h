#ifndef _SensorInfo_h
#define _SensorInfo_h
#include <Arduino.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <supla/channel_element.h>
#include <supla/sensor/general_purpose_measurement.h>
#include <supla/sensor/general_purpose_meter.h>
#include "SensorBase.h"

namespace Supla {
namespace Sensor {

// Impulse Counter implementation
class SensorInfoCounter : public SensorBase, public ChannelElement {
 public:
  SensorInfoCounter(std::string meter_id, std::string type, std::string property_to_send, std::string keyString)
      : SensorBase(meter_id, type, property_to_send, keyString) {
    channel.setType(SUPLA_CHANNELTYPE_IMPULSE_COUNTER);
    channel.setDefault(SUPLA_CHANNELFNC_IC_WATER_METER);
    channel.setInitialCaption(property_to_send.c_str());
  };

  void setNewValue(uint64_t value) override {
    Serial.print("wMBus-lib: Setting new value for sensor (Counter) with id: ");
    Serial.print(this->get_meter_id().c_str());
    Serial.print(" value: ");
    Serial.println(value);
    channel.setNewValue(value);
  };
};

// General Purpose Measurement implementation
class SensorInfoGeneraPurposeMeasurement : public SensorBase, public GeneralPurposeMeasurement {
 public:
  SensorInfoGeneraPurposeMeasurement(std::string meter_id, std::string type, std::string property_to_send, std::string keyString)
      : SensorBase(meter_id, type, property_to_send, keyString), GeneralPurposeMeasurement(nullptr, false) {
    setInitialCaption(property_to_send.c_str());
  };

  void setNewValue(uint64_t value) override {
    Serial.print("wMBus-lib: Setting new value for sensor (General Purpose Measurement) with id: ");
    _lastValue = static_cast<double>(value)/1000.0;
    Serial.print(this->get_meter_id().c_str());
    Serial.print(" value: ");
    Serial.println(_lastValue);
  };

  double getValue() override {
    return _lastValue;
  };

 private:
  double _lastValue = 0.0;
};

class SensorInfoGeneralPurposeMeter : public SensorBase, public GeneralPurposeMeter {
 public:
  SensorInfoGeneralPurposeMeter(std::string meter_id, std::string type, std::string property_to_send, std::string keyString)
      : SensorBase(meter_id, type, property_to_send, keyString), GeneralPurposeMeter(nullptr, false) {
    setInitialCaption(property_to_send.c_str());
  };

  void setNewValue(uint64_t value) override {
    Serial.print("wMBus-lib: Setting new value for sensor (General Purpose Meter) with id: ");
    _lastValue = static_cast<double>(value)/1000.0;
    Serial.print(this->get_meter_id().c_str());
    Serial.print(" value: ");
    Serial.println(_lastValue);
    
  };

  double getValue() override {
    return _lastValue;
  };

 private:
  double _lastValue = 0.0;
};

// Factory function to create appropriate sensor type based on channel type
inline SensorBase*
createSensorInfo(std::string meter_id, std::string type, std::string property_to_send, std::string keyString, uint8_t channelType) {
  if (channelType == 0) {
    return new SensorInfoCounter(meter_id, type, property_to_send, keyString);
  }
  if (channelType == 1) {
    return new SensorInfoGeneraPurposeMeasurement(meter_id, type, property_to_send, keyString);
  }

  if (channelType == 2) {
    return new SensorInfoGeneralPurposeMeter(meter_id, type, property_to_send, keyString);
  }

  return new SensorInfoGeneraPurposeMeasurement(meter_id, type, property_to_send, keyString);
}

};  // namespace Sensor
};  // namespace Supla
#endif