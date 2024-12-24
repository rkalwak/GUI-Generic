
// /*
//   Copyright (C) krycha88

//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version.
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// */
#ifdef SUPLA_ANALOG_READING_KPOP

#ifndef _analog_reading_map_kpop_h
#define _analog_reading_map_kpop_h

#include <Arduino.h>
#include <supla/sensor/general_purpose_measurement.h>
#include <supla/storage/storage.h>

#ifdef ARDUINO_ARCH_ESP32
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#endif

namespace Supla {
namespace Sensor {

#define NO_OF_SAMPLES 10

#ifdef ARDUINO_ARCH_ESP32
void initADC();
void cleanupADC();
#endif

class AnalogReading : public GeneralPurposeMeasurement {
 public:
  AnalogReading(uint8_t pin);

  void onInit();
  ~AnalogReading();

  uint16_t readValuesFromDevice();
  double getValue();

  void onSaveState();
  void onLoadState();

  double mapDouble(double x, double in_min, double in_max, double out_min, double out_max);

  void calibrateMinValue();
  void calibrateMaxValue();

  void setMinValue(float value);
  float getMinValue();

  void setMaxValue(float value);
  float getMaxValue();

  void setMinDesiredValue(float value);
  float getMinDesiredValue();

  void setMaxDesiredValue(float value);
  float getMaxDesiredValue();

#ifdef ARDUINO_ARCH_ESP32
  static adc_channel_t get_ADC_channel(uint8_t pin);
#endif

 protected:
  uint8_t pin;
  float min;
  float max;
  float minDesired;
  float maxDesired;
};

};  // namespace Sensor
};  // namespace Supla

#endif
#endif