
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

#ifndef _analog_reding_map_kpop_h
#define _analog_reding_map_kpop_h

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
class AnalogReading : public GeneralPurposeMeasurement {
 public:
  AnalogReading(uint8_t pin) : GeneralPurposeMeasurement(nullptr, false), pin(pin), min(0), max(0), minDesired(0), maxDesired(0) {
  }

#ifdef ARDUINO_ARCH_ESP32
  adc_channel_t get_ADC_channel(uint8_t pin) {
    adc_channel_t chan;
    switch (pin) {
      case 32:
        chan = ADC_CHANNEL_4;
        break;
#ifndef CONFIG_IDF_TARGET_ESP32C3
      case 33:
        chan = ADC_CHANNEL_5;
        break;
      case 34:
        chan = ADC_CHANNEL_6;
        break;
      case 35:
        chan = ADC_CHANNEL_7;
        break;
#endif
      case 36:
        chan = ADC_CHANNEL_0;
        break;
      case 37:
        chan = ADC_CHANNEL_1;
        break;
      case 38:
        chan = ADC_CHANNEL_2;
        break;
      case 39:
        chan = ADC_CHANNEL_3;
        break;
    }
    return chan;
  }
#endif
  void onInit() {
    pinMode(pin, INPUT);
    channel.setNewValue(getValue());
    this->setRefreshIntervalMs(1000);

    // Initialize ADC with new driver
#ifdef ARDUINO_ARCH_ESP32
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,  // Używamy ADC1
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t chan_config = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
    adc_oneshot_config_channel(adc_handle, get_ADC_channel(pin), &chan_config);

    // Initialize ADC calibration
    adc_cali_line_fitting_config_t cali_config = {.unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};
    adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
#endif
  }

  uint16_t readValuesFromDevice() {
    uint16_t average = 0;
    int raw_value = 0;

#ifdef ARDUINO_ARCH_ESP32
    int calibrated_value = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++) {
      adc_oneshot_read(adc_handle, get_ADC_channel(pin), &raw_value);

      // Convert raw value to calibrated voltage
      adc_cali_raw_to_voltage(cali_handle, raw_value, &calibrated_value);

      average += calibrated_value;
    }
#else
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
      raw_value = analogRead(pin);
      average += raw_value;
    }
#endif

    average /= NO_OF_SAMPLES;
    return average;
  }

  double getValue() {
    double value;

    if (min == max || minDesired == maxDesired)
      return NAN;

    value = mapDouble(readValuesFromDevice(), min, max, minDesired, maxDesired);
    value = constrain(value, minDesired, maxDesired);

    return value;
  }

  void onSaveState() {
    Supla::Storage::WriteState((unsigned char *)&min, sizeof(min));
    Supla::Storage::WriteState((unsigned char *)&max, sizeof(max));
    Supla::Storage::WriteState((unsigned char *)&minDesired, sizeof(minDesired));
    Supla::Storage::WriteState((unsigned char *)&maxDesired, sizeof(maxDesired));
  }

  void onLoadState() {
    Supla::Storage::ReadState((unsigned char *)&min, sizeof(min));
    Supla::Storage::ReadState((unsigned char *)&max, sizeof(max));
    Supla::Storage::ReadState((unsigned char *)&minDesired, sizeof(minDesired));
    Supla::Storage::ReadState((unsigned char *)&maxDesired, sizeof(maxDesired));
  }

  double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  void calibrateMinValue() {
    setMinValue(readValuesFromDevice());
    Serial.print(F("Calibrate - write MIN value: "));
    Serial.println(min);
    Supla::Storage::ScheduleSave(1000);
  }

  void calibrateMaxValue() {
    setMaxValue(readValuesFromDevice());
    Serial.print(F("Calibrate - write MAX value: "));
    Serial.println(max);
    Supla::Storage::ScheduleSave(1000);
  }

  void setMinValue(float value) {
    min = value;
    Supla::Storage::ScheduleSave(1000);
  }

  float getMinValue() {
    return min;
  }

  void setMaxValue(float value) {
    max = value;
    Supla::Storage::ScheduleSave(1000);
  }

  float getMaxValue() {
    return max;
  }

  void setMinDesiredValue(float value) {
    minDesired = value;
    Supla::Storage::ScheduleSave(1000);
  }

  float getMinDesiredValue() {
    return minDesired;
  }

  void setMaxDesiredValue(float value) {
    maxDesired = value;
    Supla::Storage::ScheduleSave(1000);
  }

  float getMaxDesiredValue() {
    return maxDesired;
  }

 protected:
  uint8_t pin;

  float min;
  float max;
  float minDesired;
  float maxDesired;

#ifdef ARDUINO_ARCH_ESP32
  adc_oneshot_unit_handle_t adc_handle;  // ADC unit handle for oneshot mode
  adc_cali_handle_t cali_handle;         // Handle for calibration
#endif
};

};  // namespace Sensor
};  // namespace Supla

#endif
#endif
