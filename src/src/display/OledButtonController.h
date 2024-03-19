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

#ifndef OLED_BUTTON_CONTROLLER_H
#define OLED_BUTTON_CONTROLLER_H

#include "SuplaOled.h"
#ifdef SUPLA_THERMOSTAT
#include "../control/ThermostatGUI.h"
#include "../../SuplaConfigManager.h"
#endif
#include <supla/action_handler.h>

namespace Supla {
namespace Control {
namespace GUI {
class OledButtonController : public Supla::ActionHandler {
 private:
  SuplaOled *oled;
#ifdef SUPLA_THERMOSTAT
  std::array<Supla::Control::GUI::ThermostatGUI *, MAX_THERMOSTAT> thermostat;
#endif
  int holdCounter = 0;

  void initializeThermostatButtons();

 public:
#ifdef SUPLA_THERMOSTAT
  OledButtonController(SuplaOled *oled, std::array<Supla::Control::GUI::ThermostatGUI *, MAX_THERMOSTAT> &thermostat);
#endif

  OledButtonController(SuplaOled *oled);
  void handleAction(int event, int action) override;
};
}  // namespace GUI
}  // namespace Control
}  // namespace Supla
#endif
#endif