#ifndef SRC_SUPLA_SENSOR_INA_219_H_
#define SRC_SUPLA_SENSOR_INA_219_H_

#ifdef SUPLA_INA219

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <supla/log_wrapper.h>
#include <supla/sensor/one_phase_electricity_meter.h>

namespace Supla {
namespace Sensor {

class INA_219 : public OnePhaseElectricityMeter {
 public:
  INA_219(uint8_t address = INA219_ADDRESS, TwoWire *wire = &Wire) : INA(address) {
    if (!INA.begin(wire)) {
      SUPLA_LOG_DEBUG("Unable to find INA219");
    }
    else {
      SUPLA_LOG_DEBUG("INA219 is connected at address: 0x%x", address);
    }
  }

  void onInit() override {
    readValuesFromDevice();
    updateChannelValues();
  }

  virtual void readValuesFromDevice() {
    setVoltage(0, INA.getBusVoltage_V() * 100);  // Convert V to mV
    setCurrent(0, INA.getCurrent_mA() * 1);      // Current in 0.001 A
    setPowerActive(0, INA.getPower_mW() * 100);   // Power in 0.00001 W
  }

 protected:
  Adafruit_INA219 INA;
};

};  // namespace Sensor
};  // namespace Supla
#endif
#endif  // SRC_SUPLA_SENSOR_INA_219_H_
