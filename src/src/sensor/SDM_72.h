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
#ifndef _ONE_PHASE_SDM_72_V2_h
#define _ONE_PHASE_SDM_72_V2_h

#include <Arduino.h>
#include <SDM.h>
#include <supla/sensor/electricity_meter.h>
#include "SDM_ReadValues.h"

namespace Supla {
namespace Sensor {

class SDM72V2 : public ReadValuesSDM, public ElectricityMeter {
 public:
#if defined(ESP8266)
  SDM72V2(int8_t pinRX, int8_t pinTX, long baud = 9600);
#else
  SDM72V2(HardwareSerial& serial, int8_t pinRX, int8_t pinTX, long baud = 9600);
#endif
  void onInit();
  virtual void readValuesFromDevice();
};

};  // namespace Sensor
};  // namespace Supla

#endif
#endif