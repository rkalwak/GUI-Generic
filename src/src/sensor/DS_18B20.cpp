#include "DS_18B20.h"
#include "../../SuplaDeviceGUI.h"

DS18B20::DS18B20(uint8_t pin, uint8_t *deviceAddress)
    : oneWire(pin), sensors(&oneWire), lastValidValue(TEMPERATURE_NOT_AVAILABLE), retryCounter(0), lastReadTime(0) {
  if (deviceAddress == nullptr) {
    Serial.println("Device address not provided. Using device from index 0");
    address[0] = 0;
  }
  else {
    memcpy(address, deviceAddress, 8);
  }
}

void DS18B20::iterateAlways() {
  const unsigned long conversionInterval = 10000;
  const unsigned long updateInterval = 5000;

  unsigned long currentTime = millis();
  unsigned long timeSinceLastRead = currentTime - lastReadTime;

  if (timeSinceLastRead >= conversionInterval) {
    sensors.setWaitForConversion(true);
    sensors.requestTemperatures();
    sensors.setWaitForConversion(false);
    lastReadTime = currentTime;
  }

  if (timeSinceLastRead >= updateInterval) {
    channel.setNewValue(getValue());
  }
}

double DS18B20::getValue() {
  double value = TEMPERATURE_NOT_AVAILABLE;

  if (sensors.isConversionComplete()) {
    if (address[0] == 0) {
      value = sensors.getTempCByIndex(0);
    }
    else {
      value = sensors.getTempC(address);
    }

    if (value == DEVICE_DISCONNECTED_C || value == 85.0) {
      value = TEMPERATURE_NOT_AVAILABLE;
    }

    if (value != TEMPERATURE_NOT_AVAILABLE || retryCounter > 3) {
      retryCounter = 0;
    }
    else {
      retryCounter++;

      sensors.setWaitForConversion(true);
      sensors.requestTemperatures();
      sensors.setWaitForConversion(false);

      value = lastValidValue;
    }
  }

  lastValidValue = value;
  return value;
}

void DS18B20::onInit() {
  channel.setNewValue(getValue());
}

void DS18B20::setDeviceAddress(uint8_t *deviceAddress) {
  if (deviceAddress == nullptr) {
    supla_log(LOG_DEBUG, "Device address not provided. Using device from index 0");
  }
  else {
    memcpy(address, deviceAddress, 8);
  }
}

void findAndSaveDS18B20Addresses() {
  uint8_t pin = ConfigESP->getGpio(FUNCTION_DS18B20);
  uint8_t maxDevices = ConfigManager->get(KEY_MULTI_MAX_DS18B20)->getValueInt();
  OneWire ow(pin);
  DallasTemperature sensors(&ow);

  sensors.begin();

  Serial.print("Szukanie urządzeń DS18B20...");

  int deviceCount = 0;

  for (int i = 0; i < maxDevices; ++i) {
    DeviceAddress devAddr;

    if (sensors.getAddress(devAddr, i)) {
      deviceCount++;

      char devAddrStr[17];
      for (uint8_t j = 0; j < 8; j++) {
        sprintf(devAddrStr + j * 2, "%02X", devAddr[j]);
      }
      devAddrStr[16] = '\0';

      ConfigManager->setElement(KEY_ADDR_DS18B20, i, devAddrStr);

      Serial.print("Znaleziono urządzenie na adresie: ");
      Serial.print(devAddrStr);
      Serial.println();
    }
    else {
      break;
    }
  }

  Serial.print("Znaleziono łącznie ");
  Serial.print(deviceCount);
  Serial.println(" urządzeń DS18B20.");

  ConfigManager->save();
}
