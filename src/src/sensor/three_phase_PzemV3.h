/*
  Copyright (C) krycha88

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifdef SUPLA_PZEM_V_3

#ifndef GUI_SRC_SUPLA_SENSOR_THREE_PHASE_PZEMV3_H_
#define GUI_SRC_SUPLA_SENSOR_THREE_PHASE_PZEMV3_H_

#include <Arduino.h>
// Dependence: Arduino library for the Updated PZEM-004T v3.0 Power and Energy
// meter  https://github.com/mandulaj/PZEM-004T-v30
#include <PZEM004Tv30.h>

#if defined(ESP8266)
#include <SoftwareSerial.h>
#endif

#include <supla/sensor/electricity_meter.h>

#define UPDATE_TIME 200

namespace Supla {
namespace Sensor {
class ThreePhasePZEMv3 : public ElectricityMeter {
 public:
#if defined(ESP8266)
  ThreePhasePZEMv3(int8_t pinRX1, int8_t pinTX1, int8_t pinRX2, int8_t pinTX2, int8_t pinRX3, int8_t pinTX3)
      : softSerial1(pinRX1, pinTX1),
        softSerial2(pinRX2, pinTX2),
        softSerial3(pinRX3, pinTX3),
        pzem{PZEM004Tv30(softSerial1), PZEM004Tv30(softSerial2), PZEM004Tv30(softSerial3)} {
    softSerial1.begin(PZEM_BAUD_RATE);
    softSerial2.begin(PZEM_BAUD_RATE);
    softSerial3.begin(PZEM_BAUD_RATE);
  }
#elif defined(ESP32)
  ThreePhasePZEMv3(HardwareSerial &serial1,
                   int8_t pinRx1,
                   int8_t pinTx1,
                   HardwareSerial &serial2,
                   int8_t pinRx2,
                   int8_t pinTx2,
                   HardwareSerial &serial3,
                   int8_t pinRx3,
                   int8_t pinTx3)
      : serial1(serial1),
        serial2(serial2),
        serial3(serial3),
        pzem{PZEM004Tv30(serial1, pinRx1, pinTx1), PZEM004Tv30(serial2, pinRx2, pinTx2), PZEM004Tv30(serial3, pinRx3, pinTx3)} {
    serial1.begin(PZEM_BAUD_RATE);
    serial2.begin(PZEM_BAUD_RATE);
    serial3.begin(PZEM_BAUD_RATE);
  }
#endif

  void onInit() {
    readValuesFromDevice();
    updateChannelValues();
  }

  virtual void readValuesFromDevice() {
    bool atLeastOnePzemWasRead = false;

    for (int i = 0; i < 3; i++) {
      float current = pzem[i].current();
      bool currentValid = !isnan(current) && current >= 0;
      if (!currentValid) {
        Serial.printf("Error reading current from PZEM %d\n", i + 1);
        continue;
      }

      atLeastOnePzemWasRead = true;

      float voltage = pzem[i].voltage();
      bool voltageValid = !isnan(voltage) && voltage > 0;
      if (!voltageValid) {
        Serial.printf("Error reading voltage from PZEM %d\n", i + 1);
      }

      float active = pzem[i].power();
      bool activeValid = !isnan(active) && active >= 0;
      if (!activeValid) {
        Serial.printf("Error reading power from PZEM %d\n", i + 1);
      }

      float apparent = (voltageValid && currentValid) ? (voltage * current) : NAN;
      float reactive = (voltageValid && activeValid && apparent > active) ? sqrt(apparent * apparent - active * active) : 0;

      float energy = pzem[i].energy();
      bool energyValid = !isnan(energy) && energy >= 0;
      if (!energyValid) {
        Serial.printf("Error reading energy from PZEM %d\n", i + 1);
      }

      float powerFactor = pzem[i].pf();
      bool powerFactorValid = !isnan(powerFactor) && powerFactor >= 0 && powerFactor <= 1;
      if (!powerFactorValid) {
        Serial.printf("Error reading power factor from PZEM %d\n", i + 1);
      }

      float frequency = pzem[i].frequency();
      bool frequencyValid = !isnan(frequency) && frequency > 0;
      if (!frequencyValid) {
        Serial.printf("Error reading frequency from PZEM %d\n", i + 1);
      }

      if (voltageValid) {
        setVoltage(i, static_cast<int>(voltage * 100));
      }
      if (currentValid) {
        setCurrent(i, static_cast<int>(current * 1000));
      }
      if (activeValid) {
        setPowerActive(i, static_cast<int>(active * 100000));
      }
      if (energyValid) {
        setFwdActEnergy(i, static_cast<int>(energy * 100000));
      }
      if (powerFactorValid) {
        setPowerFactor(i, static_cast<int>(powerFactor * 1000));
      }
      if (voltageValid && currentValid) {
        setPowerApparent(i, static_cast<int>(apparent * 100000));
        setPowerReactive(i, static_cast<int>(reactive * 100000));
      }
      if (frequencyValid) {
        setFreq(static_cast<unsigned short>(frequency * 100));
      }

      delay(UPDATE_TIME);  // Delay between readings
    }

    if (!atLeastOnePzemWasRead) {
      resetReadParameters();
    }
  }

  void resetStorage() {
    for (int i = 0; i < 3; i++) {
      pzem[i].resetEnergy();
    }
  }

 protected:
#if defined(ESP8266)
  SoftwareSerial softSerial1;
  SoftwareSerial softSerial2;
  SoftwareSerial softSerial3;
#endif
#if defined(ESP32)
  HardwareSerial &serial1;
  HardwareSerial &serial2;
  HardwareSerial &serial3;
#endif
  PZEM004Tv30 pzem[3];
};
}  // namespace Sensor
}  // namespace Supla

#endif
#endif
