/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_hydrodigit.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Hydrodigit: Driver
{
  Hydrodigit() : Driver(std::string("hydrodigit")) {};
  virtual std::map<std::string, float> get_values(std::vector<unsigned char> &telegram) override {
    std::map<std::string, float> ret_val{};

    float total_water = this->get_0C14(telegram);
    if (total_water == 0.0f) {
      total_water = this->get_0C13(telegram);
    }
    if (total_water == 0.0f) {
      total_water = this->get_0413(telegram);
    }
    add_to_map(ret_val, "total_water_m3", total_water);
    this->add_manufacturer_values(telegram, ret_val);

    if (ret_val.size() > 0) {
      return ret_val;
    }
    else {
      return {};
    }
  };

private:
  static constexpr uint8_t MASK_BATTERY_VOLTAGE_PRESENT = 1 << 0;
  static constexpr uint8_t MASK_FRAUD_DATE_PRESENT = 1 << 1;
  static constexpr uint8_t MASK_BACKWARD_FLOW_PRESENT = 1 << 2;
  static constexpr uint8_t MASK_DATA_HISTORY_PRESENT = 1 << 4;
  static constexpr uint8_t MASK_WATER_LOSS_DATE_PRESENT = 1 << 7;

  float decode_voltage(uint8_t voltage_byte) {
    switch (voltage_byte & 0x0F) {
      case 0x01:
        return 1.9f;
      case 0x02:
        return 2.1f;
      case 0x03:
        return 2.2f;
      case 0x04:
        return 2.3f;
      case 0x05:
        return 2.4f;
      case 0x06:
        return 2.5f;
      case 0x07:
        return 2.65f;
      case 0x08:
        return 2.8f;
      case 0x09:
        return 2.9f;
      case 0x0A:
        return 3.05f;
      case 0x0B:
        return 3.2f;
      case 0x0C:
        return 3.35f;
      case 0x0D:
        return 3.5f;
      default:
        return 3.7f;
    }
  }

  float decode_backflow(const std::vector<unsigned char> &telegram, size_t index) {
    uint32_t raw = ((uint32_t)telegram[index + 3] << 24) |
                   ((uint32_t)telegram[index + 2] << 16) |
                   ((uint32_t)telegram[index + 1] << 8) |
                   ((uint32_t)telegram[index + 0]);
    return raw / 1000.0f;
  }

  float decode_monthly(const std::vector<unsigned char> &telegram, size_t index) {
    uint32_t raw = ((uint32_t)telegram[index + 2] << 16) |
                   ((uint32_t)telegram[index + 1] << 8) |
                   ((uint32_t)telegram[index + 0]);
    float value = raw / 100.0f;
    if (value >= 100000.0f) {
      return 0.0f;
    }
    return value;
  }

  size_t find_mfct_block(const std::vector<unsigned char> &telegram) {
    for (size_t i = 11; i < telegram.size(); i++) {
      if (telegram[i] == 0x0F && i + 1 < telegram.size()) {
        return i;
      }
    }
    return telegram.size();
  }

  void add_manufacturer_values(std::vector<unsigned char> &telegram,
                               std::map<std::string, float> &ret_val) {
    static const char *const monthly_keys[] = {
        "January_total_m3",   "February_total_m3", "March_total_m3",
        "April_total_m3",     "May_total_m3",      "June_total_m3",
        "July_total_m3",      "August_total_m3",   "September_total_m3",
        "October_total_m3",   "November_total_m3", "December_total_m3"};

    size_t mfct_index = find_mfct_block(telegram);
    if (mfct_index >= telegram.size() || mfct_index + 1 >= telegram.size()) {
      return;
    }

    uint8_t frame_identifier = telegram[mfct_index + 1];
    size_t i = mfct_index + 2;

    if (frame_identifier & MASK_BATTERY_VOLTAGE_PRESENT) {
      if (i >= telegram.size()) {
        return;
      }
      add_to_map(ret_val, "battery_voltage_v", decode_voltage(telegram[i]));
      i++;
    }

    if (frame_identifier & MASK_FRAUD_DATE_PRESENT) {
      if (i + 2 >= telegram.size()) {
        return;
      }
      i += 3;
    }

    if (frame_identifier & MASK_WATER_LOSS_DATE_PRESENT) {
      if (i + 2 >= telegram.size()) {
        return;
      }
      i += 3;
    }

    if (frame_identifier & MASK_BACKWARD_FLOW_PRESENT) {
      if (i + 3 >= telegram.size()) {
        return;
      }
      add_to_map(ret_val, "backflow_m3", decode_backflow(telegram, i));
      i += 4;
    }

    if (frame_identifier & MASK_DATA_HISTORY_PRESENT) {
      for (size_t month = 0; month < 12; month++) {
        if (i + 2 >= telegram.size()) {
          return;
        }
        add_to_map(ret_val, monthly_keys[month], decode_monthly(telegram, i));
        i += 3;
      }
    }
  }
};