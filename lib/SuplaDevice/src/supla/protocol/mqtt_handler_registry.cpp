/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

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

#include <supla/protocol/mqtt_handler_registry.h>

namespace Supla {
namespace Protocol {

MqttHandlerRegistry &MqttHandlerRegistry::instance() {
  static MqttHandlerRegistry registry;
  return registry;
}

bool MqttHandlerRegistry::registerHandler(MqttChannelHandler *handler) {
  if (handler == nullptr) {
    return false;
  }

  for (size_t i = 0; i < handlerCount; i++) {
    if (handlers[i] == handler || handlers[i]->mqttHandledChannelType() ==
                                      handler->mqttHandledChannelType()) {
      return true;
    }
  }

  if (handlerCount >= SUPLA_MQTT_HANDLER_REGISTRY_MAX_HANDLERS) {
    return false;
  }

  handlers[handlerCount++] = handler;
  return true;
}

MqttChannelHandler *MqttHandlerRegistry::findHandler(int channelType) const {
  for (size_t i = 0; i < handlerCount; i++) {
    if (handlers[i]->mqttHandledChannelType() == channelType) {
      return handlers[i];
    }
  }
  return nullptr;
}

#if SUPLA_TEST
void MqttHandlerRegistry::clearForTests() {
  handlerCount = 0;
}

size_t MqttHandlerRegistry::sizeForTests() const {
  return handlerCount;
}
#endif

}  // namespace Protocol
}  // namespace Supla
