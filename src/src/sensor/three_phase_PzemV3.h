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

#include <supla/sensor/three_phase_PzemV3.h>

namespace Supla {
namespace Sensor {

class CustomThreePhasePZEMv3 : public ThreePhasePZEMv3 {
 public:
#if defined(PZEM004_SOFTSERIAL)
  CustomThreePhasePZEMv3(int8_t pinRX1, int8_t pinTX1, int8_t pinRX2, int8_t pinTX2, int8_t pinRX3, int8_t pinTX3)
      : ThreePhasePZEMv3(pinRX1, pinTX1, pinRX2, pinTX2, pinRX3, pinTX3) {
  }
#endif

#if defined(ESP32)
  CustomThreePhasePZEMv3(HardwareSerial *serial1,
                         int8_t pinRx1,
                         int8_t pinTx1,
                         HardwareSerial *serial2,
                         int8_t pinRx2,
                         int8_t pinTx2,
                         HardwareSerial *serial3,
                         int8_t pinRx3,
                         int8_t pinTx3)
      : ThreePhasePZEMv3(serial1, pinRx1, pinTx1, serial2, pinRx2, pinTx2, serial3, pinRx3, pinTx3) {
  }
#else
  CustomThreePhasePZEMv3(HardwareSerial *serial1, HardwareSerial *serial2, HardwareSerial *serial3) : ThreePhasePZEMv3(serial1, serial2, serial3) {
  }
#endif

  int handleCalcfgFromServer(TSD_DeviceCalCfgRequest *request) {
    if (request && request->Command == SUPLA_CALCFG_CMD_RESET_COUNTERS) {
      for (int i = 0; i < 3; i++) {
        pzem[i].resetEnergy();
      }
      return SUPLA_CALCFG_RESULT_DONE;
    }
    return SUPLA_CALCFG_RESULT_NOT_SUPPORTED;
  }
};

};  // namespace Sensor
};  // namespace Supla
#endif
#endif