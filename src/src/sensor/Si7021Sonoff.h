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
#ifdef SUPLA_SI7021_SONOFF

#ifndef SUPLA_SENSOR_SI_7021_SONOFF_H_
#define SUPLA_SENSOR_SI_7021_SONOFF_H_

// Dependency: DHTNew Library
// https://github.com/RobTillaart/DHTNew

#include <dhtnew.h>

#include <supla/sensor/therm_hygro_meter.h>
#include <supla/log_wrapper.h>

namespace Supla {
namespace Sensor {
namespace GUI {
class Si7021Sonoff : public ThermHygroMeter {
 public:
  Si7021Sonoff(int8_t pin) : sensor(pin) {
    statusSensor = DHTLIB_WAITING_FOR_READ;
    retryCountTemp = 0;
    retryCountHumi = 0;
    lastValidTemp = TEMPERATURE_NOT_AVAILABLE;
    lastValidHumi = HUMIDITY_NOT_AVAILABLE;

    sensor.setType(70);  // type for sonoff
  }

  void onInit() {
    statusSensor = sensor.read();
  }

  double getTemp() {
    double value = TEMPERATURE_NOT_AVAILABLE;

    value = sensor.getTemperature();

    if (statusSensor != DHTLIB_OK) {
      value = TEMPERATURE_NOT_AVAILABLE;
    }

    if (value == TEMPERATURE_NOT_AVAILABLE) {
      retryCountTemp++;
      if (retryCountTemp > 3) {
        retryCountTemp = 0;
      }
      else {
        value = lastValidTemp;
      }
    }
    else {
      retryCountTemp = 0;
    }
    lastValidTemp = value;

    return value;
  }

  double getHumi() {
    double value = HUMIDITY_NOT_AVAILABLE;

    value = sensor.getHumidity();

    if (statusSensor != DHTLIB_OK) {
      value = HUMIDITY_NOT_AVAILABLE;
    }

    if (value == HUMIDITY_NOT_AVAILABLE) {
      retryCountHumi++;
      if (retryCountHumi > 3) {
        retryCountHumi = 0;
      }
      else {
        value = lastValidHumi;
      }
    }
    else {
      retryCountHumi = 0;
    }
    lastValidHumi = value;

    return value;
  }

  void iterateAlways() {
    if (lastReadTime + 10000 < millis()) {
      lastReadTime = millis();

      statusSensor = sensor.read();
      const char* sensorIdentifier = "Si7021Sonoff";

      switch (statusSensor) {
        case DHTLIB_OK:
          SUPLA_LOG_DEBUG("%s: OK\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_CHECKSUM:
          SUPLA_LOG_DEBUG("%s: Checksum error\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_TIMEOUT_A:
          SUPLA_LOG_DEBUG("%s: Time out A error\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_TIMEOUT_B:
          SUPLA_LOG_DEBUG("%s: Time out B error\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_TIMEOUT_C:
          SUPLA_LOG_DEBUG("%s: Time out C error\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_TIMEOUT_D:
          SUPLA_LOG_DEBUG("%s: Time out D error\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_SENSOR_NOT_READY:
          SUPLA_LOG_DEBUG("%s: Sensor not ready\t", sensorIdentifier);
          break;
        case DHTLIB_ERROR_BIT_SHIFT:
          SUPLA_LOG_DEBUG("%s: Bit shift error\t", sensorIdentifier);
          break;
        default:
          SUPLA_LOG_DEBUG("%s: Unknown: %d\t", sensorIdentifier, statusSensor);
          break;
      }

      channel.setNewValue(getTemp(), getHumi());
    }
  }

 protected:
  ::DHTNEW sensor;
  int statusSensor;
  double lastValidTemp;
  double lastValidHumi;
  int8_t retryCountTemp;
  int8_t retryCountHumi;
};

};  // namespace GUI
};  // namespace Sensor
};  // namespace Supla

#endif  // SRC_SUPLA_SENSOR_LM75_H_
#endif
