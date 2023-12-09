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

#include "SuplaWebCorrection.h"

void createWebCorrection() {
  WebServer->httpServer->on(getURL(PATH_CORRECTION), [&]() {
    if (!WebServer->isLoggedIn()) {
      return;
    }

    if (WebServer->httpServer->method() == HTTP_GET)
      handleCorrection();
    else
      handleCorrectionSave();
  });
}

void handleCorrection(int save) {
  WebServer->sendHeaderStart();

  webContentBuffer += SuplaSaveResult(save);
  webContentBuffer += SuplaJavaScript(PATH_CORRECTION);

  addForm(webContentBuffer, F("post"), PATH_CORRECTION);

  addFormHeader(webContentBuffer, S_CORRECTION_FOR_CH);
  ThermHygroMeterCorrectionHandler& handler = ThermHygroMeterCorrectionHandler::getInstance();
  auto& thermHygroMeters = handler.getThermHygroMeters();

  for (auto& meter : thermHygroMeters) {
    int channelNumber = meter->getChannel()->getChannelNumber();

    if (meter->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_THERMOMETER) {
      double actualTemperatureCorrection = static_cast<double>(meter->getConfiguredTemperatureCorrection()) / 10.0;

      addNumberBox(webContentBuffer, getInput(INPUT_CORRECTION_TEMP, channelNumber),
                   String(channelNumber) + S_SPACE + "-" + S_SPACE + +meter->getTemp() + S_CELSIUS, emptyString, false,
                   String(actualTemperatureCorrection));
    }

    if (meter->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR) {
      double actualTemperatureCorrection = static_cast<double>(meter->getConfiguredTemperatureCorrection()) / 10.0;
      addNumberBox(webContentBuffer, getInput(INPUT_CORRECTION_TEMP, channelNumber),
                   String(channelNumber) + S_SPACE + "-" + S_SPACE + +meter->getTemp() + S_CELSIUS, emptyString, false,
                   String(actualTemperatureCorrection));

      double actualHumidityCorrection = static_cast<double>(meter->getConfiguredHumidityCorrection()) / 10.0;
      addNumberBox(webContentBuffer, getInput(INPUT_CORRECTION_HUMIDITY, channelNumber),
                   String(channelNumber) + S_SPACE + "-" + S_SPACE + +meter->getHumi() + "%", emptyString, false, String(actualHumidityCorrection));
    }
  }
  addFormHeaderEnd(webContentBuffer);

  addButtonSubmit(webContentBuffer, S_SAVE);
  addFormEnd(webContentBuffer);
  addButton(webContentBuffer, S_RETURN, PATH_DEVICE_SETTINGS);

  WebServer->sendHeaderEnd();
}

void handleCorrectionSave() {
  if (!WebServer->isLoggedIn()) {
    return;
  }

  ThermHygroMeterCorrectionHandler& handler = ThermHygroMeterCorrectionHandler::getInstance();
  auto& thermHygroMeters = handler.getThermHygroMeters();

  for (auto& meter : thermHygroMeters) {
    int channelNumber = meter->getChannel()->getChannelNumber();

    if (meter->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_THERMOMETER) {
      int32_t temperatureCorrection = (WebServer->httpServer->arg(getInput(INPUT_CORRECTION_TEMP, channelNumber)).toDouble() * 10.0);
      meter->applyCorrectionsAndStoreIt(temperatureCorrection, 0, true);
    }

    if (meter->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR) {
      int32_t temperatureCorrection = (WebServer->httpServer->arg(getInput(INPUT_CORRECTION_TEMP, channelNumber)).toDouble() * 10.0);
      int32_t humidityCorrection = (WebServer->httpServer->arg(getInput(INPUT_CORRECTION_HUMIDITY, channelNumber)).toDouble() * 10.0);

      meter->applyCorrectionsAndStoreIt(temperatureCorrection, humidityCorrection, true);
    }
  }

  switch (ConfigManager->save()) {
    case E_CONFIG_OK:
      handleCorrection(SaveResult::DATA_SAVE);
      break;
    case E_CONFIG_FILE_OPEN:
      handleCorrection(SaveResult::DATA_SAVED_RESTART_MODULE);
      break;
  }
}

ThermHygroMeterCorrectionHandler::ThermHygroMeterCorrectionHandler() {
}

void ThermHygroMeterCorrectionHandler::addThermHygroMeter(Supla::Sensor::ThermHygroMeter* newMeter) {
  thermHygroMeters.push_back(newMeter);
}

std::vector<Supla::Sensor::ThermHygroMeter*>& ThermHygroMeterCorrectionHandler::getThermHygroMeters() {
  return thermHygroMeters;
}

ThermHygroMeterCorrectionHandler& ThermHygroMeterCorrectionHandler::getInstance() {
  static ThermHygroMeterCorrectionHandler handler;
  return handler;
}
