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

#include "ThermostatGUI.h"

#include <SuplaDevice.h>
#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>
#include <supla/control/hvac_base.h>
#include <supla/clock/clock.h>
#include <supla/control/internal_pin_output.h>
#include <supla/events.h>
#include <supla/actions.h>

#include "../../SuplaDeviceGUI.h"

namespace Supla {
namespace Control {
namespace GUI {

ThermostatGUI::ThermostatGUI(uint8_t nr) {
  Serial.print("Dodano termostat dla przekaźnika ");
  Serial.println(nr + 1);

  uint8_t pinRelay = ConfigESP->getGpio(nr, FUNCTION_RELAY);
  bool highIsOn = ConfigESP->getLevel(pinRelay);

  uint8_t pinLED = ConfigESP->getGpio(nr, FUNCTION_LED);
  bool levelLed = ConfigESP->getLevel(pinLED);

  uint8_t mainThermometr = ConfigManager->get(KEY_THERMOSTAT_MAIN_THERMOMETER_CHANNEL)->getElement(nr).toInt();
  uint8_t auxThermometr = ConfigManager->get(KEY_THERMOSTAT_AUX_THERMOMETER_CHANNEL)->getElement(nr).toInt();
  mainThermometr++;
  auxThermometr++;
  double histeresis = ConfigManager->get(KEY_THERMOSTAT_HISTERESIS)->getElement(nr).toDouble();

  // WARNING: using default Clock class. It works only when there is connection
  // with Supla server. In real thermostat application, use RTC HW clock,
  // which will keep time after ESP power reset
  new Supla::Clock;

  auto output = new Supla::Control::InternalPinOutput(pinRelay, highIsOn);
  auto hvac = new Supla::Control::HvacBase(output);

  hvac->setMainThermometerChannelNo(mainThermometr);  // Main Thermometer
  if (mainThermometr != auxThermometr) {
    hvac->setAuxThermometerChannelNo(auxThermometr);  // Aux Thermometer
  }

  hvac->setTemperatureHisteresis(histeresis * 10);

  // Configure thermostat parameters
  hvac->setTemperatureHisteresisMin(20);    // 0.2 degree
  hvac->setTemperatureHisteresisMax(1000);  // 10 degree
  hvac->setTemperatureAutoOffsetMin(200);   // 2 degrees
  hvac->setTemperatureAutoOffsetMax(1000);  // 10 degrees
  hvac->addAvailableAlgorithm(SUPLA_HVAC_ALGORITHM_ON_OFF_SETPOINT_AT_MOST);

  // AUX
  hvac->setAuxThermometerType(SUPLA_HVAC_AUX_THERMOMETER_TYPE_FLOOR);
  hvac->setTemperatureAuxMin(500);   // 5 degrees
  hvac->setTemperatureAuxMax(7500);  // 75 degrees

  // grzanie od 5,00 do 55,00
  hvac->setDefaultTemperatureRoomMin(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_HEAT, 1000);
  hvac->setDefaultTemperatureRoomMax(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_HEAT, 9500);
  // chłodzenie od 0,00 do 10,00
  hvac->setDefaultTemperatureRoomMin(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_COOL, 0);
  hvac->setDefaultTemperatureRoomMax(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_COOL, 4000);

  if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_COOL) {
    hvac->getChannel()->setDefault(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_COOL);
  }

  if (pinLED != OFF_GPIO) {
    auto statusLed = new Supla::Control::InternalPinOutput(pinLED, levelLed);

    if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_COOL) {
      hvac->addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_STANDBY, true);
      hvac->addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_HEATING, true);
      hvac->addAction(Supla::TURN_ON, statusLed, Supla::ON_HVAC_COOLING, true);
    }
    else {
      hvac->addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_STANDBY, true);
      hvac->addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_COOLING, true);
      hvac->addAction(Supla::TURN_ON, statusLed, Supla::ON_HVAC_HEATING, true);
    }
  }
};

};  // namespace GUI
};  // namespace Control
};  // namespace Supla