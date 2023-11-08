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

#if SUPLA_THERMOSTAT
#include "ThermostatGUI.h"

namespace Supla {
namespace Control {
namespace GUI {

ThermostatGUI::ThermostatGUI(uint8_t nr, SuplaDeviceClass *sdc)
    : Supla::Control::HvacBase(
          new Supla::Control::InternalPinOutput(ConfigESP->getGpio(nr, FUNCTION_RELAY), ConfigESP->getLevel(ConfigESP->getGpio(nr, FUNCTION_RELAY)))),
      Supla::Protocol::ProtocolLayer(sdc),
      nr(nr) {
  uint8_t pinLED = ConfigESP->getGpio(nr, FUNCTION_LED);
  bool levelLed = ConfigESP->getLevel(pinLED);

  uint8_t mainThermometr = ConfigManager->get(KEY_THERMOSTAT_MAIN_THERMOMETER_CHANNEL)->getElement(nr).toInt();
  uint8_t auxThermometr = ConfigManager->get(KEY_THERMOSTAT_AUX_THERMOMETER_CHANNEL)->getElement(nr).toInt();
  double histeresis = ConfigManager->get(KEY_THERMOSTAT_HISTERESIS)->getElement(nr).toDouble();

  new Supla::Clock;

  HvacBase::setTemperatureHisteresis(histeresis * 10);

  if (mainThermometr != THERMOSTAT_NO_TEMP_CHANNEL) {
    HvacBase::setMainThermometerChannelNo(mainThermometr);
  }

  if (auxThermometr != THERMOSTAT_NO_TEMP_CHANNEL) {
    HvacBase::setAuxThermometerChannelNo(auxThermometr);

    HvacBase::setAuxThermometerType(SUPLA_HVAC_AUX_THERMOMETER_TYPE_FLOOR);
    HvacBase::setTemperatureAuxMin(500);   // 5 degrees
    HvacBase::setTemperatureAuxMax(7500);  // 75 degrees
  }

  // Configure thermostat parameters
  HvacBase::setTemperatureHisteresisMin(20);    // 0.2 degree
  HvacBase::setTemperatureHisteresisMax(4000);  // 10 degree
  HvacBase::setTemperatureAutoOffsetMin(200);   // 2 degrees
  HvacBase::setTemperatureAutoOffsetMax(1000);  // 10 degrees
  HvacBase::addAvailableAlgorithm(SUPLA_HVAC_ALGORITHM_ON_OFF_SETPOINT_MIDDLE);

  if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_HEAT) {
    HvacBase::getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_HVAC_THERMOSTAT);
    HvacBase::setDefaultSubfunction(SUPLA_HVAC_SUBFUNCTION_HEAT);
  }
  else if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_COOL) {
    HvacBase::getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_HVAC_THERMOSTAT);
    HvacBase::setDefaultSubfunction(SUPLA_HVAC_SUBFUNCTION_COOL);
  }
  else if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_AUTO) {
    HvacBase::getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_AUTO);
  }
  else if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_DOMESTIC_HOT_WATER) {
    HvacBase::enableDomesticHotWaterFunctionSupport();
    HvacBase::getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_HVAC_DOMESTIC_HOT_WATER);
  }
  else if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_DIFFERENTIAL) {
    HvacBase::getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_HVAC_THERMOSTAT_DIFFERENTIAL);
  }

  if (pinLED != OFF_GPIO) {
    auto statusLed = new Supla::Control::InternalPinOutput(pinLED, levelLed);

    if (ConfigManager->get(KEY_THERMOSTAT_TYPE)->getElement(nr).toInt() == Supla::GUI::THERMOSTAT_COOL) {
      HvacBase::addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_STANDBY, true);
      HvacBase::addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_HEATING, true);
      HvacBase::addAction(Supla::TURN_ON, statusLed, Supla::ON_HVAC_COOLING, true);
    }
    else {
      HvacBase::addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_STANDBY, true);
      HvacBase::addAction(Supla::TURN_OFF, statusLed, Supla::ON_HVAC_COOLING, true);
      HvacBase::addAction(Supla::TURN_ON, statusLed, Supla::ON_HVAC_HEATING, true);
    }
  }

  HvacBase::setButtonTemperatureStep(10);

#ifdef SUPLA_BUTTON
#ifdef SUPLA_OLED
  Supla::GUI::addButtonToRelay(0, this, this);
#else
  Supla::GUI::addButtonToRelay(nr, this, this);

#endif
#endif
}

void ThermostatGUI::notifyConfigChange(int channelNumber) {
  if (HvacBase::getChannelNumber() == channelNumber) {
    ConfigManager->setElement(KEY_THERMOSTAT_MAIN_THERMOMETER_CHANNEL, nr, static_cast<int>(HvacBase::getMainThermometerChannelNo()));
    ConfigManager->setElement(KEY_THERMOSTAT_AUX_THERMOMETER_CHANNEL, nr, static_cast<int>(HvacBase::getAuxThermometerChannelNo()));
    ConfigManager->setElement(KEY_THERMOSTAT_HISTERESIS, nr, static_cast<double>(HvacBase::getTemperatureHisteresis() / 100.0));
    ConfigManager->save();
  }
}

void ThermostatGUI::handleAction(int event, int action) {
#ifdef SUPLA_OLED
  if (!getHandleActionBlocked()) {
    if (getNrActiveThermostat() == getNrThermostat()) {
      HvacBase::handleAction(event, action);
    }
  }
#else
  HvacBase::handleAction(event, action);
#endif
}

};  // namespace GUI
};  // namespace Control
};  // namespace Supla
#endif