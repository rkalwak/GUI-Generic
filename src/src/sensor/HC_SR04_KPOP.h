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
#ifdef SUPLA_HCSR04KPOP
#ifndef _hc_sr04_kpop_h
#define _hc_sr04_kpop_h

#include <math.h>
#include <NewPing.h>
#include <supla/sensor/general_purpose_measurement.h>

namespace Supla {
namespace Sensor {
class HC_SR04_KPOP : public GeneralPurposeMeasurement {
 public:
  HC_SR04_KPOP(int8_t trigPin, int8_t echoPin, int16_t minRange = 0, int16_t maxRange = 500, bool calculate = false, bool simulate = false)
  : GeneralPurposeMeasurement(nullptr, false) {
    _trigPin = trigPin;
    _echoPin = echoPin;
    _minRange = minRange;
    _maxRange = maxRange;
    _calculate = calculate;
    _simulate = simulate;

    if(!_simulate) { 
      sonar = new NewPing(_trigPin, _echoPin, _maxRange > 0 ? _maxRange : 500);
      delay(100);  // give time to initialize, preventing ping_median fails
      sonar->ping_median(5);
    }
    setDefaultUnitAfterValue("%");
    setDefaultValuePrecision(2);
  }

  virtual double getValue() {
    if(_simulate) {
      return random(_minRange, _maxRange);  // Simulated value for testing purposes
    }

    uint32_t echoTime = sonar->ping_median(5);
    if (echoTime == 0) {
      return NAN;
    }

    float distance = (float)echoTime / US_ROUNDTRIP_CM;

    if (_maxRange == _minRange) {
      return NAN;
    }

    if(_calculate) {
      float percent = (distance - _minRange) / (float)(_maxRange - _minRange) * 100.0f;
      if (percent < 0.0f) {
        percent = 0.0f;
      }
      if (percent > 100.0f) {
        percent = 100.0f;
      }

      return static_cast<double>(percent);
    } 
    else {
      return static_cast<double>(distance);
    }

  }

 protected:
  int8_t _trigPin;
  int8_t _echoPin;
  int16_t _minRange;
  int16_t _maxRange;
  bool _calculate;
  bool _simulate;

  NewPing *sonar = nullptr;
};

};  // namespace Sensor
};  // namespace Supla

#endif
#endif
