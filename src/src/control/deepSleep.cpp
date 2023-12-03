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
#include "deepSleep.h"
#include "../../SuplaDeviceGUI.h"

namespace Supla {
namespace Control {

DeepSleep::DeepSleep(unsigned _supla_int_t sleepTimeSec) : sleepTimeSec(sleepTimeSec), lastUpdateCheckTime(millis()) {
}

void DeepSleep::iterateAlways() {
  static bool updateFound = false;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateCheckTime > 5000 && ConfigESP->configModeESP == Supla::DEVICE_MODE_NORMAL) {
    Serial.println(F("Checking for updates..."));

    if (Supla::Protocol::ProtocolLayer::IsAnyUpdatePending()) {
      updateFound = true;
      Serial.println(F("Update found"));
    }

    if (currentTime - lastUpdateCheckTime > 10000 || (updateFound && currentTime - lastUpdateCheckTime > 5000)) {
      if (!updateFound) {
        Serial.println(F("Update check timeout"));
      }

      Serial.println(F("Preparing for ESP deep sleep"));

      WiFi.disconnect(true);
      delay(200);

      Serial.println(F("Putting ESP into deep sleep"));

#ifdef ESP8266
      ESP.deepSleep(sleepTimeSec * 1000000);
#elif defined(ESP32)
      esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000);
      esp_deep_sleep_start();
#endif
      lastUpdateCheckTime = currentTime;
    }
  }
}

void DeepSleep::onInit() {
  SuplaDevice.addFlags(SUPLA_DEVICE_FLAG_SLEEP_MODE_ENABLED);
  SuplaDevice.setActivityTimeout(sleepTimeSec * 2);

  for (auto element = Supla::Element::begin(); element != nullptr; element = element->next()) {
    if (element->getChannel()) {
      auto channel = element->getChannel();
      channel->setValidityTimeSec(sleepTimeSec * 2);
    }

    if (element->getSecondaryChannel()) {
      auto channel = element->getSecondaryChannel();
      channel->setValidityTimeSec(sleepTimeSec * 2);
    }
  }
}

}  // namespace Control
}  // namespace Supla
