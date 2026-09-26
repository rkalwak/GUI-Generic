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
#ifdef SUPLA_ADS1115_KPOP

#ifndef _ads1115_h
#define _ads1115_h

#include <Arduino.h>
#include <supla/io/ADS1115.h>
#include <supla/sensor/general_purpose_measurement.h>
#include <supla/element.h>

namespace Supla {
namespace Sensor {

class ADS1115_X : public Element {
 public:
  explicit ADS1115_X(uint8_t address = 0x48,
                    uint8_t gain = 0,
                    TwoWire *wire = &Wire,
                    uint32_t refreshIntervalMs = 1000)
      : ads_(address, nullptr, wire), address_(address), gain_(gain), refreshIntervalMs_(refreshIntervalMs) {
    ads_.setGain(gain_);
  }

  void iterateAlways() override {
    if (millis() - lastReadTime < refreshIntervalMs_) {
      return;
    }

    lastReadTime = millis();
    for (uint8_t channel = 0; channel < 4; ++channel) {
      values_[channel] = ads_.customAnalogRead(0, channel);
    }
  }

  void onInit() override {
    lastReadTime = millis();
    ads_.setGain(gain_);

    Serial.print(F("Turning ON ADS1115 sensor at address 0x"));
    Serial.println(address_, HEX);
  }

  void setGain(uint8_t gain) {
    gain_ = gain;
    ads_.setGain(gain_);
  }

  int16_t getValue(uint8_t channel) const {
    if (channel >= 4) {
      return -1;
    }
    return values_[channel];
  }

 protected:
  Supla::Io::ADS1115 ads_;
  uint8_t address_ = 0x48;
  uint8_t gain_ = 0;
  uint32_t refreshIntervalMs_ = 1000;
  uint32_t lastReadTime = 0;
  int16_t values_[4] = {-1, -1, -1, -1};
};

class ADS1115_A0 : public GeneralPurposeMeasurement {
 public:
  explicit ADS1115_A0(ADS1115_X *sensor)
      : GeneralPurposeMeasurement(nullptr, false), sensor_(sensor) {
    setDefaultUnitAfterValue("ADC");
    setInitialCaption("ADS1115 A0");
    getChannel()->setDefaultIcon(8);
    setDefaultValuePrecision(0);
  }

  double getValue() override {
    if (sensor_ == nullptr) {
      return NAN;
    }
    return sensor_->getValue(0);
  }

 protected:
  ADS1115_X *sensor_ = nullptr;
};

class ADS1115_A1 : public GeneralPurposeMeasurement {
 public:
  explicit ADS1115_A1(ADS1115_X *sensor)
      : GeneralPurposeMeasurement(nullptr, false), sensor_(sensor) {
    setDefaultUnitAfterValue("ADC");
    setInitialCaption("ADS1115 A1");
    getChannel()->setDefaultIcon(8);
    setDefaultValuePrecision(0);
  }

  double getValue() override {
    if (sensor_ == nullptr) {
      return NAN;
    }
    return sensor_->getValue(1);
  }

 protected:
  ADS1115_X *sensor_ = nullptr;
};

class ADS1115_A2 : public GeneralPurposeMeasurement {
 public:
  explicit ADS1115_A2(ADS1115_X *sensor)
      : GeneralPurposeMeasurement(nullptr, false), sensor_(sensor) {
    setDefaultUnitAfterValue("ADC");
    setInitialCaption("ADS1115 A2");
    getChannel()->setDefaultIcon(8);
    setDefaultValuePrecision(0);
  }

  double getValue() override {
    if (sensor_ == nullptr) {
      return NAN;
    }
    return sensor_->getValue(2);
  }

 protected:
  ADS1115_X *sensor_ = nullptr;
};

class ADS1115_A3 : public GeneralPurposeMeasurement {
 public:
  explicit ADS1115_A3(ADS1115_X *sensor)
      : GeneralPurposeMeasurement(nullptr, false), sensor_(sensor) {
    setDefaultUnitAfterValue("ADC");
    setInitialCaption("ADS1115 A3");
    getChannel()->setDefaultIcon(8);
    setDefaultValuePrecision(0);
  }

  double getValue() override {
    if (sensor_ == nullptr) {
      return NAN;
    }
    return sensor_->getValue(3);
  }

 protected:
  ADS1115_X *sensor_ = nullptr;
};

}  // namespace Sensor
}  // namespace Supla

#endif
#endif
