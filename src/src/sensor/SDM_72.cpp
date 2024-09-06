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
#ifdef SUPLA_MODBUS_SDM_72_V2
#include "SDM_72.h"

namespace Supla {
namespace Sensor {

#if defined(ESP8266)
SDM72V2::SDM72V2(int8_t pinRX, int8_t pinTX, long baud) : ReadValuesSDM(pinRX, pinTX, baud){};
#else
SDM72V2::SDM72V2(HardwareSerial& serial, int8_t pinRX, int8_t pinTX, long baud) : ReadValuesSDM(serial, pinRX, pinTX, baud){};
#endif

void SDM72V2::onInit() {
  readValuesFromDevice();
  updateChannelValues();
}

void SDM72V2::readValuesFromDevice() {
  clearSuccCount();
  clearErrCount();
  clearErrCode();

  // Odczyty bez podziału na fazy dla energii i energii biernej
  float energy = ReadValuesSDM::getFwdActEnergy();
  float reactEnergy = ReadValuesSDM::getFwdReactEnergy();
  float rvrActEnergy = ReadValuesSDM::getRvrActEnergy();
  float rvrReactEnergy = ReadValuesSDM::getRvrReactEnergy();

  if (!isnan(energy))
    setFwdActEnergy(0, energy * 100000);

  if (!isnan(reactEnergy))
    setFwdReactEnergy(0, reactEnergy * 100000);

  if (!isnan(rvrActEnergy))
    setRvrActEnergy(0, rvrActEnergy * 100000);

  if (!isnan(rvrReactEnergy))
    setRvrReactEnergy(0, rvrReactEnergy * 100000);

  // Odczyty z podziałem na fazy dla pozostałych parametrów
  float freq = ReadValuesSDM::getFreq();
  for (int i = 0; i < MAX_PHASES; i++) {
    float voltage = ReadValuesSDM::getVoltage(i);
    float current = ReadValuesSDM::getCurrent(i);
    float powerActive = ReadValuesSDM::getPowerActive(i);
    float powerFactor = ReadValuesSDM::getPowerFactor(i);
    float powerReactive = ReadValuesSDM::getPowerReactive(i);
    float powerApparent = ReadValuesSDM::getPowerApparent(i);
    float phaseAngle = ReadValuesSDM::getPhaseAngle(i);

    if (!isnan(voltage))
      setVoltage(i, voltage * 100);

    if (!isnan(current))
      setCurrent(i, current * 1000);

    if (!isnan(powerActive))
      setPowerActive(i, powerActive * 100000);

    if (!isnan(powerFactor))
      setPowerFactor(i, powerFactor * 1000);

    if (!isnan(freq))
      setFreq(freq * 100);

    if (!isnan(powerReactive))
      setPowerReactive(i, powerReactive * 100000);

    if (!isnan(powerApparent))
      setPowerApparent(i, powerApparent * 100000);

    if (!isnan(phaseAngle))
      setPhaseAngle(i, phaseAngle * 10);

    delay(0);
  }
}

};  // namespace Sensor
};  // namespace Supla
#endif