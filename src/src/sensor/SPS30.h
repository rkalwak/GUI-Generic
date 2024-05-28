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
#ifdef SUPLA_SPS30_KPOP

#ifndef _sps30_h
#define _sps30_h

#include <Arduino.h>
#include <supla/sensor/general_purpose_measurement.h>
#include <supla/element.h>
#include <sps30.h>

namespace Supla {
namespace Sensor {
// 1, 2.5, 4. , 10
class SPS30_X : public Element {
 public:
  explicit SPS30_X() {
  }

  void iterateAlways() {
    if (millis() - lastReadTime > 60000) {  // 1min
      lastReadTime = millis();
      do {
        ret = sps30_read_data_ready(&data_ready);
        if (ret < 0) {
          Serial.print("error reading data-ready flag: ");
          Serial.println(ret);
        }
        else if (!data_ready)
          Serial.print("data not ready, no new measurement available\n");
        else
          break;
        delay(100); /* retry in 100ms */
      } while (1);

      ret = sps30_read_measurement(&m);
    }
  }

  void onInit() {
    lastReadTime = millis();
    Serial.println(F("Turning ON SPS sensor"));
    sensirion_i2c_init();
    while (sps30_probe() != 0) {
      Serial.print("SPS sensor probing failed\n");
      delay(500);
    }

    ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
    if (ret) {
      Serial.print("error setting the auto-clean interval: ");
      Serial.println(ret);
    }

    ret = sps30_start_measurement();
    if (ret < 0) {
      Serial.print("error starting measurement\n");
    }

#ifndef PLOTTER_FORMAT
    Serial.print("measurements started\n");
#endif /* PLOTTER_FORMAT */
  }

  sps30_measurement getMeasurement() {
    return this->m;
  }

 protected:
  unsigned long lastSleepTime = 0;
  unsigned long lastReadTime = 0;
  bool sleepSensor = false;
  int16_t ret;
  uint8_t auto_clean_days = 4;
  uint32_t auto_clean;
  uint16_t data_ready;
  struct sps30_measurement m;
};

class SPS30_PM005 : public GeneralPurposeMeasurement {
 public:
  SPS30_PM005(SPS30_X *sensor) {
    sps30 = sensor;

    this->setDefaultUnitAfterValue("μg/m³");
    this->setInitialCaption("Light sensor 🌞");
  }

  double getValue() {
    double value = NAN;
     auto measurement =sps30->getMeasurement();
    value = measurement.nc_0p5;
    return value;
  }

 protected:
  SPS30_X *sps30;
};

class SPS30_PM01 : public GeneralPurposeMeasurement {
 public:
  SPS30_PM01(SPS30_X *sensor) {
    sps30 = sensor;

    this->setDefaultUnitAfterValue("μg/m³");
  }

  double getValue() {
    double value = NAN;
     auto measurement =sps30->getMeasurement();
    value = measurement.nc_1p0-measurement.nc_0p5;
    return value;
  }

 protected:
  SPS30_X *sps30;
};

class SPS30_PM025 : public GeneralPurposeMeasurement {
 public:
  SPS30_PM025(SPS30_X *sensor) {
    sps30 = sensor;

    this->setDefaultUnitAfterValue("μg/m³");
  }

  double getValue() {
    double value = NAN;
     auto measurement =sps30->getMeasurement();
    value = measurement.nc_2p5-measurement.nc_1p0;
    return value;
  }

 protected:
  SPS30_X *sps30;
};

class SPS30_PM04 : public GeneralPurposeMeasurement {
 public:
  SPS30_PM04(SPS30_X *sensor) {
    sps30 = sensor;

    this->setDefaultUnitAfterValue("μg/m³");
  }

  double getValue() {
    double value = NAN;
     auto measurement =sps30->getMeasurement();
    value = measurement.nc_4p0-measurement.nc_2p5;
    return value;
  }

 protected:
  SPS30_X *sps30;
};

class SPS30_PM10 : public GeneralPurposeMeasurement {
 public:
  SPS30_PM10(SPS30_X *sensor) {
    sps30 = sensor;

    this->setDefaultUnitAfterValue("μg/m³");
  }

  double getValue() {
    double value = NAN;
    auto measurement =sps30->getMeasurement();
    value = measurement.nc_10p0-measurement.nc_4p0;
    return value;
  }

 protected:
  SPS30_X *sps30;
};

}  // namespace Sensor
}  // namespace Supla

#endif
#endif