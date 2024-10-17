#ifndef SRC_SUPLA_SENSOR_INA_219_H_
#define SRC_SUPLA_SENSOR_INA_219_H_

#ifdef SUPLA_INA219

#include <Wire.h>
#include <INA219.h>
#include <supla/log_wrapper.h>
#include <supla/sensor/one_phase_electricity_meter.h>

namespace Supla {
namespace Sensor {

class INA_219 : public OnePhaseElectricityMeter {
 public:
  INA_219(uint8_t address = 0x40, TwoWire *wire = &Wire, float maxAmp = 3.4, float shuntRes = 0.1) : INA(address, wire) {
    if (!INA.begin()) {
      SUPLA_LOG_DEBUG("Unable to find INA219");
    }
    else {
      SUPLA_LOG_DEBUG("INA219 is connected at address: 0x%x", address);
    }
    INA.setMaxCurrentShunt(maxAmp, shuntRes);
  }

  void onInit() override {
    readValuesFromDevice();
    updateChannelValues();
  }

  virtual void readValuesFromDevice() {
    setVoltage(0, INA.getBusVoltage() * 100);
    setCurrent(0, INA.getCurrent() * 1000);
    setPowerActive(0, INA.getPower() * 100000);
  }

 protected:
  ::INA219 INA;
};

};  // namespace Sensor
};  // namespace Supla
#endif
#endif  // SRC_SUPLA_SENSOR_INA_219_H_
