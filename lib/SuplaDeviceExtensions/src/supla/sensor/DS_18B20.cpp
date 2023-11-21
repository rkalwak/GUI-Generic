#include "DS_18B20.h"

OneWireBus::OneWireBus(uint8_t pinNumberConfig)
  : oneWire(pinNumberConfig), pin(pinNumberConfig), nextBus(nullptr), lastReadTime(0) {
  supla_log(LOG_DEBUG, "Initializing OneWire bus at pin %d", pinNumberConfig);
  sensors.setOneWire(&oneWire);
  sensors.begin();
  if (sensors.isParasitePowerMode()) {
    supla_log(LOG_DEBUG, "OneWire(pin %d) Parasite power is ON", pinNumberConfig);
  } else {
    supla_log(LOG_DEBUG, "OneWire(pin %d) Parasite power is OFF", pinNumberConfig);
  }

  supla_log(LOG_DEBUG, "OneWire(pin %d) Found %d devices:", pinNumberConfig,
            sensors.getDeviceCount());

  DeviceAddress address;
  for (int i = 0; i < sensors.getDeviceCount(); i++) {
    if (!sensors.getAddress(address, i)) {
      supla_log(LOG_DEBUG, "Unable to find address for Device %d", i);
    } else {
      // Zmiana: Ustawianie rozdzielczości na 12 bitów
      sensors.setResolution(address, 12);
    }
    // Zmiana: Dodanie krótkiego opóźnienia przed pomiarem
    delay(100);
  }
  sensors.setWaitForConversion(true);
  sensors.requestTemperatures();
  sensors.setWaitForConversion(false);
}

int8_t OneWireBus::getIndex(uint8_t *deviceAddress) {
  DeviceAddress address;
  for (int i = 0; i < sensors.getDeviceCount(); i++) {
    if (sensors.getAddress(address, i)) {
      bool found = true;
      for (int j = 0; j < 8; j++) {
        if (deviceAddress[j] != address[j]) {
          found = false;
        }
      }
      if (found) {
        return i;
      }
    }
  }
  return -1;
}

DS18B20::DS18B20(uint8_t pin, uint8_t *deviceAddress) {
  OneWireBus *bus = oneWireBus;
  OneWireBus *prevBus = nullptr;
  address[0] = 0;
  lastValidValue = TEMPERATURE_NOT_AVAILABLE;
  retryCounter = 0;

  if (bus) {
    while (bus) {
      if (bus->pin == pin) {
        myBus = bus;
        break;
      }
      prevBus = bus;
      bus = bus->nextBus;
    }
  }

  // Brak utworzonego magistrali OneWire dla tego pinu
  if (!bus) {
    supla_log(LOG_DEBUG, "Creating OneWire bus for pin: %d", pin);
    myBus = new OneWireBus(pin);
    if (prevBus) {
      prevBus->nextBus = myBus;
    } else {
      oneWireBus = myBus;
    }
  }
  if (deviceAddress == nullptr) {
    supla_log(LOG_DEBUG, "Device address not provided. Using device from index 0");
  } else {
    memcpy(address, deviceAddress, 8);
  }
}

void DS18B20::iterateAlways() {
  if (myBus->lastReadTime + 10000 < millis()) {
    // Zmiana: Ustawienie waitForConversion przed pomiarami
    myBus->sensors.setWaitForConversion(true);
    myBus->sensors.requestTemperatures();
    myBus->sensors.setWaitForConversion(false);
    myBus->lastReadTime = millis();
  }
  if (myBus->lastReadTime + 5000 < millis() && (lastReadTime != myBus->lastReadTime)) {
    channel.setNewValue(getValue());
    lastReadTime = myBus->lastReadTime;
  }
}

double DS18B20::getValue() {
  double value = TEMPERATURE_NOT_AVAILABLE;

  // Zmiana: Sprawdzenie dostępności konwersji przed odczytem
if (myBus->sensors.isConversionComplete()) {
    value = myBus->sensors.getTempC(address);
    
    if (value == DEVICE_DISCONNECTED_C || value == 85.0) {
      value = TEMPERATURE_NOT_AVAILABLE;
    }

    if (value == TEMPERATURE_NOT_AVAILABLE) {
      retryCounter++;
      if (retryCounter > 3) {
        retryCounter = 0;
        // Zmiana: Ponowne zapytanie o temperatury z opóźnieniem
        myBus->sensors.setWaitForConversion(true);
        myBus->sensors.requestTemperatures();
        myBus->sensors.setWaitForConversion(false);
      } else {
        value = lastValidValue;
      }
    } else {
      retryCounter = 0;
    }
  }

  lastValidValue = value;
  return value;
}

void DS18B20::onInit() {
  channel.setNewValue(getValue());
}

uint8_t DS18B20::getPin() {
  return myBus->pin;
}

void DS18B20::setDeviceAddress(uint8_t *deviceAddress) {
  if (deviceAddress == nullptr) {
    supla_log(LOG_DEBUG, "Device address not provided. Using device from index 0");
  } else {
    memcpy(address, deviceAddress, 8);
  }
}

OneWireBus *DS18B20::oneWireBus = nullptr;
