/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_vario451.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct Vario451 : Driver
{
  Vario451() : Driver(std::string("vario451")){};
  virtual std::map<std::string, float> get_values(std::vector<unsigned char> &telegram) override
  {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_heating_kwh", this->total_heating_kwh(telegram));
    add_to_map(ret_val, "total_heating_gj", this->total_heating_gj(telegram));

    if (ret_val.size() > 0)
    {
      return ret_val;
    }
    else
    {
      return {};
    }
  };

private:
  float total_heating_kwh(std::vector<unsigned char> &telegram)
  {
    // in kWh
    float total_heating_in_kwh = get_total_heating_in_gj(telegram) * 277.777;
    return total_heating_in_kwh;
  }

  float total_heating_gj(std::vector<unsigned char> &telegram)
  {
    // in GJ
    return get_total_heating_in_gj(telegram);
  }

  float get_total_heating_in_gj(const std::vector<unsigned char> &telegram) const
  {
    float usage = 0;
    size_t i = 11;

    usage = ((((uint32_t)telegram[i + 4] << 8) + (uint32_t)telegram[i + 3]) / 1000.0) +
            ((((uint32_t)telegram[i + 8] << 8) + (uint32_t)telegram[i + 7]) / 1000.0);
    return usage;
  };
};
