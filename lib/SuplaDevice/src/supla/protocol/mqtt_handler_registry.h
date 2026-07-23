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

#ifndef SRC_SUPLA_PROTOCOL_MQTT_HANDLER_REGISTRY_H_
#define SRC_SUPLA_PROTOCOL_MQTT_HANDLER_REGISTRY_H_

#include <stddef.h>

#include <supla/protocol/mqtt_channel_handler.h>

#ifndef SUPLA_MQTT_HANDLER_REGISTRY_MAX_HANDLERS
#define SUPLA_MQTT_HANDLER_REGISTRY_MAX_HANDLERS 8
#endif

namespace Supla {
namespace Protocol {

class MqttHandlerRegistry {
 public:
  static MqttHandlerRegistry &instance();

  bool registerHandler(MqttChannelHandler *handler);
  MqttChannelHandler *findHandler(int channelType) const;

#if SUPLA_TEST
  void clearForTests();
  size_t sizeForTests() const;
#endif

 private:
  MqttChannelHandler *handlers[SUPLA_MQTT_HANDLER_REGISTRY_MAX_HANDLERS] = {};
  size_t handlerCount = 0;
};

}  // namespace Protocol
}  // namespace Supla

#endif  // SRC_SUPLA_PROTOCOL_MQTT_HANDLER_REGISTRY_H_
