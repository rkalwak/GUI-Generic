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
#ifdef SUPLA_OLED
#include "OledButtonController.h"

#include <supla/events.h>
#include <supla/actions.h>
#include <supla/control/button.h>
#include "../../SuplaCommonPROGMEM.h"

namespace Supla {
namespace Control {
namespace GUI {
#ifdef SUPLA_THERMOSTAT
OledButtonController::OledButtonController(SuplaOled* oled, std::array<Supla::Control::GUI::ThermostatGUI*, MAX_THERMOSTAT>& thermostat)
    : oled(oled), thermostat(thermostat) {
  initializeThermostatButtons();
}
#endif

OledButtonController::OledButtonController(SuplaOled* oled) : oled(oled) {
  initializeThermostatButtons();
}

void OledButtonController::initializeThermostatButtons() {
  for (uint8_t nr = 0; nr < ConfigManager->get(KEY_MAX_BUTTON)->getValueInt(); nr++) {
    uint8_t pinButton = ConfigESP->getGpio(nr, FUNCTION_BUTTON);

    if (pinButton != OFF_GPIO) {
      Supla::Control::Button* button = Supla::Control::GUI::Button(pinButton, ConfigESP->getPullUp(pinButton), ConfigESP->getInversed(pinButton), nr);

      button->addAction(OLED_NEXT_FRAME, this, Supla::Event::ON_HOLD);
      button->addAction(OLED_NEXT_FRAME, this, Supla::Event::ON_RELEASE);

      int muliclickTimeMs = ConfigManager->get(KEY_AT_MULTICLICK_TIME)->getValueFloat() * 1000;
      int holdTimeMs = ConfigManager->get(KEY_AT_HOLD_TIME)->getValueFloat() * 1000;

      button->setHoldTime(holdTimeMs);
      button->setMulticlickTime(muliclickTimeMs);
      button->repeatOnHoldEvery(250);
    }
  }
}

void OledButtonController::handleAction(int event, int action) {
  if (!oled->isDisplayEnabled()) {
    oled->enableDisplay(true);
  }
  else {
    oled->enableDisplay(true);

    if (action == OLED_NEXT_FRAME && event == Supla::Event::ON_HOLD) {
      holdCounter++;
    }

#ifdef SUPLA_THERMOSTAT
    bool skipThermostatAction = false;

    if (holdCounter < 8 && event == Supla::Event::ON_HOLD) {
      skipThermostatAction = true;
    }

    int activeCurrentFrame = oled->getCurrentFrame();

    if (thermostat[activeCurrentFrame] != nullptr && !skipThermostatAction) {
      if (action == Supla::GUI::Action::TOGGLE_MANUAL_WEEKLY_SCHEDULE_MODES_HOLD_OFF && holdCounter == 8) {
        thermostat[activeCurrentFrame]->handleAction(Supla::Event::ON_CLICK_1, Supla::Action::TOGGLE);
      }
      thermostat[activeCurrentFrame]->handleAction(event, action);
    }
#endif

    if (event == Supla::Event::ON_RELEASE) {
      if (holdCounter > 0 && holdCounter < 8 && oled->getFrameCount() > 1) {
        oled->handleAction(Supla::Event::ON_RELEASE, OLED_NEXT_FRAME);
      }

      holdCounter = 0;
    }
  }
}
}  // namespace GUI
}  // namespace Control
}  // namespace Supla
#endif