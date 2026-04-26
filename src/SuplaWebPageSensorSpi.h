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

#ifndef SuplaWebPageSensorSpi_h
#define SuplaWebPageSensorSpi_h

#include "SuplaDeviceGUI.h"

#if defined(SUPLA_MAX6675) || defined(SUPLA_MAX31855) || defined (SUPLA_CC1101) || defined(SUPLA_INA229) || defined(SUPLA_INA239)
#define GUI_SENSOR_SPI
#endif

#ifdef GUI_SENSOR_SPI

#define PATH_SPI "spi"

#define INPUT_MAX6675  "max6675"
#define INPUT_MAX31855 "imax31855"
#define INPUT_CLK_GPIO "clk"
#define INPUT_CS_GPIO  "cs"
#define INPUT_MISO_GPIO  "miso"
#define INPUT_MOSI_GPIO "mosi"


#ifdef SUPLA_CC1101
#define INPUT_CC1101 "cc1101"
#define INPUT_GDO0_GPIO  "gdo0"
#define INPUT_GDO2_GPIO  "gdo2"
#define INPUT_WMBUS_SENSOR_TYPE1 "s_type1"
#define INPUT_WMBUS_SENSOR_TYPE2 "s_type2"
#define INPUT_WMBUS_SENSOR_TYPE3 "s_type3"
#define INPUT_WMBUS_SENSOR_TYPE4 "s_type4"
#define INPUT_WMBUS_SENSOR_TYPE5 "s_type5"
#define INPUT_WMBUS_SENSOR_TYPE6 "s_type6"
#define INPUT_WMBUS_SENSOR_TYPE7 "s_type7"
#define INPUT_WMBUS_SENSOR_TYPE8 "s_type8"
#define INPUT_WMBUS_SENSOR_TYPE9 "s_type9"
#define INPUT_WMBUS_SENSOR_TYPE10 "s_type10"
#define INPUT_WMBUS_SENSOR_ID1 "s_id1"
#define INPUT_WMBUS_SENSOR_ID2 "s_id2"
#define INPUT_WMBUS_SENSOR_ID3 "s_id3"
#define INPUT_WMBUS_SENSOR_ID4 "s_id4"
#define INPUT_WMBUS_SENSOR_ID5 "s_id5"
#define INPUT_WMBUS_SENSOR_ID6 "s_id6"
#define INPUT_WMBUS_SENSOR_ID7 "s_id7"
#define INPUT_WMBUS_SENSOR_ID8 "s_id8"
#define INPUT_WMBUS_SENSOR_ID9 "s_id9"
#define INPUT_WMBUS_SENSOR_ID10 "s_id10"
#define INPUT_WMBUS_SENSOR_KEY1 "s_key1"
#define INPUT_WMBUS_SENSOR_KEY2 "s_key2"
#define INPUT_WMBUS_SENSOR_KEY3 "s_key3"
#define INPUT_WMBUS_SENSOR_KEY4 "s_key4"
#define INPUT_WMBUS_SENSOR_KEY5 "s_key5"
#define INPUT_WMBUS_SENSOR_KEY6 "s_key6"
#define INPUT_WMBUS_SENSOR_KEY7 "s_key7"
#define INPUT_WMBUS_SENSOR_KEY8 "s_key8"
#define INPUT_WMBUS_SENSOR_KEY9 "s_key9"
#define INPUT_WMBUS_SENSOR_KEY10 "s_key10"
#define INPUT_WMBUS_SENSOR_PROP1 "s_prop1"
#define INPUT_WMBUS_SENSOR_PROP2 "s_prop2"
#define INPUT_WMBUS_SENSOR_PROP3 "s_prop3"
#define INPUT_WMBUS_SENSOR_PROP4 "s_prop4"
#define INPUT_WMBUS_SENSOR_PROP5 "s_prop5"
#define INPUT_WMBUS_SENSOR_PROP6 "s_prop6"
#define INPUT_WMBUS_SENSOR_PROP7 "s_prop7"
#define INPUT_WMBUS_SENSOR_PROP8 "s_prop8"
#define INPUT_WMBUS_SENSOR_PROP9 "s_prop9"
#define INPUT_WMBUS_SENSOR_PROP10 "s_prop10"
#define INPUT_WMBUS_SENSOR_ENABLED1 "s_enabled1"
#define INPUT_WMBUS_SENSOR_ENABLED2 "s_enabled2"
#define INPUT_WMBUS_SENSOR_ENABLED3 "s_enabled3"
#define INPUT_WMBUS_SENSOR_ENABLED4 "s_enabled4"
#define INPUT_WMBUS_SENSOR_ENABLED5 "s_enabled5"
#define INPUT_WMBUS_SENSOR_ENABLED6 "s_enabled6"
#define INPUT_WMBUS_SENSOR_ENABLED7 "s_enabled7"
#define INPUT_WMBUS_SENSOR_ENABLED8 "s_enabled8"
#define INPUT_WMBUS_SENSOR_ENABLED9 "s_enabled9"
#define INPUT_WMBUS_SENSOR_ENABLED10 "s_enabled10"
#define INPUT_WMBUS_SENSOR_CHANNEL1 "s_channel1"
#define INPUT_WMBUS_SENSOR_CHANNEL2 "s_channel2"
#define INPUT_WMBUS_SENSOR_CHANNEL3 "s_channel3"
#define INPUT_WMBUS_SENSOR_CHANNEL4 "s_channel4"
#define INPUT_WMBUS_SENSOR_CHANNEL5 "s_channel5"
#define INPUT_WMBUS_SENSOR_CHANNEL6 "s_channel6"
#define INPUT_WMBUS_SENSOR_CHANNEL7 "s_channel7"
#define INPUT_WMBUS_SENSOR_CHANNEL8 "s_channel8"
#define INPUT_WMBUS_SENSOR_CHANNEL9 "s_channel9"
#define INPUT_WMBUS_SENSOR_CHANNEL10 "s_channel10"
const char* const sensors_types[] PROGMEM = {
    "amiplus", 
    "apator08", 
    "apator162", 
    "apatoreitn", 
    "bmeters",
    "c5isf", 
    "compact5",
    "dme07",
    "elf",
    "evo868",
    "fhkvdataiii",
    "hydrocalm3",
    "hydrodigit",
    "hydrus",
    "iperl",
    "itron",
    "izar",
    "mkradio3",
    "mkradio4",
    "qheat",
    "qwater",
    "sharky774",
    "topaseskr",
    "ultrimis",
    "unismart",
    "vario451",
    "apatorna1",
    "apatorop04"
  };

  const char* const sensors_properties[] PROGMEM = {
    "total_water_m3",
    "total_energy_consumption_kwh",
    "current_power_consumption_kw",
    "total_energy_production_kwh",
    "current_power_production_kw",
    "voltage_at_phase_1_v",
    "voltage_at_phase_2_v",
    "voltage_at_phase_3_v",
    "current_hca",
    "previous_hca",
    "temp_room_avg_c",
    "total_heating_kwh",
    "flow_temperature_c",
    "return_temperature_c",
    "current_heating_kwh",
    "previous_heating_kwh",
    "total_gas_m3",
    "total_heating_gj",
    "last_month_total_water_m3",
    "current_month_total_water_l",
    "transmit_period_s",
    "remaining_battery_life_y",
    "current_alarms",
    "previous_alarms",
    "total_volume_m3",
    "power_kw",
    "operating_time_h",
    "operating_time_in_error_h",
    "volume_flow_m3h",
    "battery_voltage_v",
    "backflow_m3",
    "January_total_m3",
    "February_total_m3",
    "March_total_m3",
    "April_total_m3",
    "May_total_m3",
    "June_total_m3",
    "July_total_m3",
    "August_total_m3",
    "September_total_m3",
    "October_total_m3",
    "November_total_m3",
    "December_total_m3"
  };

  const char* const sensors_channels[] PROGMEM = {
    "Impulse counter",
    "General purpose measurement",
    "General purpose meter",
  };

enum _wmbus_config_positions
{
  WMBUS_CFG_SENSOR_TYPE1,
  WMBUS_CFG_SENSOR_PROPERTY1,
  WMBUS_CFG_SENSOR_ENABLED1,
  WMBUS_CFG_SENSOR_CHANNEL1,
  WMBUS_CFG_SENSOR_TYPE2,
  WMBUS_CFG_SENSOR_PROPERTY2,
  WMBUS_CFG_SENSOR_ENABLED2,
  WMBUS_CFG_SENSOR_CHANNEL2,
  WMBUS_CFG_SENSOR_TYPE3,
  WMBUS_CFG_SENSOR_PROPERTY3,
  WMBUS_CFG_SENSOR_ENABLED3,
  WMBUS_CFG_SENSOR_CHANNEL3,
  WMBUS_CFG_SENSOR_TYPE4,
  WMBUS_CFG_SENSOR_PROPERTY4,
  WMBUS_CFG_SENSOR_ENABLED4,
  WMBUS_CFG_SENSOR_CHANNEL4,
  WMBUS_CFG_SENSOR_TYPE5,
  WMBUS_CFG_SENSOR_PROPERTY5,
  WMBUS_CFG_SENSOR_ENABLED5,
  WMBUS_CFG_SENSOR_CHANNEL5,
  WMBUS_CFG_SENSOR_TYPE6,
  WMBUS_CFG_SENSOR_PROPERTY6,
  WMBUS_CFG_SENSOR_ENABLED6,
  WMBUS_CFG_SENSOR_CHANNEL6,
  WMBUS_CFG_SENSOR_TYPE7,
  WMBUS_CFG_SENSOR_PROPERTY7,
  WMBUS_CFG_SENSOR_ENABLED7,
  WMBUS_CFG_SENSOR_CHANNEL7,
  WMBUS_CFG_SENSOR_TYPE8,
  WMBUS_CFG_SENSOR_PROPERTY8,
  WMBUS_CFG_SENSOR_ENABLED8,
  WMBUS_CFG_SENSOR_CHANNEL8,
  WMBUS_CFG_SENSOR_TYPE9,
  WMBUS_CFG_SENSOR_PROPERTY9,
  WMBUS_CFG_SENSOR_ENABLED9,
  WMBUS_CFG_SENSOR_CHANNEL9,
  WMBUS_CFG_SENSOR_TYPE10,
  WMBUS_CFG_SENSOR_PROPERTY10,
  WMBUS_CFG_SENSOR_ENABLED10,
  WMBUS_CFG_SENSOR_CHANNEL10
};
#endif

#ifdef SUPLA_INA229
#define INPUT_INA229 "iina229"
#endif

#ifdef SUPLA_INA239
#define INPUT_INA239 "iina239"
#endif

void createWebPageSensorSpi();
void handleSensorSpi(int save = 0);
void handleSensorSpiSave();
#endif

#endif  // ifndef SuplaWebPageSensorSpi_h
