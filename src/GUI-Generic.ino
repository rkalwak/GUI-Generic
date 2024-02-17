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
#include "SuplaDeviceGUI.h"

#ifdef SUPLA_PZEM_V_3
#include "src/sensor/PzemV3.h"
#include "src/sensor/three_phase_PzemV3.h"
#endif

#ifdef SUPLA_MPX_5XXX
#include <supla/sensor/percentage.h>
#endif

#ifdef ARDUINO_ARCH_ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#ifdef SUPLA_DIRECT_LINKS_SENSOR_THERMOMETR
#include <supla/sensor/direct_link_sensor_thermometer.h>
#endif

#ifdef SUPLA_DIRECT_LINKS_MULTI_SENSOR
#include "src/sensor/DirectLinks.h"
#endif

#ifdef SUPLA_CC1101
#include "src/sensor/WmbusMeter.h"
#endif

#include "src/boneIO/boneIO.h"
#include "src/display/OledButtonController.h"

uint32_t last_loop{0};
#define LOOP_INTERVAL 16
uint16_t packetSize1 (uint8_t lField)
{
  uint16_t nrBytes;
  uint8_t  nrBlocks;

  // The 2 first blocks contains 25 bytes when excluding CRC and the L-field
  // The other blocks contains 16 bytes when excluding the CRC-fields
  // Less than 26 (15 + 10) 
  if ( lField < 26 ) {
    nrBlocks = 2;
  }
  else { 
    nrBlocks = (((lField - 26) / 16) + 3);
  }

  // Add all extra fields, excluding the CRC fields
  nrBytes = lField + 1;

  // Add the CRC fields, each block is contains 2 CRC bytes
  nrBytes += (2 * nrBlocks);

  return (nrBytes);
}
void setup() {
  uint8_t nr, gpio;

  Serial.begin(115200);
  eeprom.setStateSavePeriod(5000);

  ConfigManager = new SuplaConfigManager();
  ConfigESP = new SuplaConfigESP();

  ImprovSerialComponent *improvSerialComponent = new ImprovSerialComponent();
  improvSerialComponent->enable();

#if defined(GUI_SENSOR_SPI) || defined(GUI_SENSOR_I2C) || defined(GUI_SENSOR_1WIRE) || defined(GUI_SENSOR_OTHER) || defined(GUI_SENSOR_I2C_2)
  ThermHygroMeterCorrectionHandler &correctionHandler = ThermHygroMeterCorrectionHandler::getInstance();
#endif

#ifdef SUPLA_BONEIO
  new Supla::boneIO();
#endif

#ifdef GUI_SENSOR_I2C_EXPENDER
  Expander = new Supla::Control::ConfigExpander();
#endif

#ifdef SUPLA_CONDITIONS
  Supla::GUI::Conditions::addConditionsElement();
#endif

#if defined(GUI_SENSOR_I2C) || defined(GUI_SENSOR_I2C_ENERGY_METER)
  if (ConfigESP->getGpio(FUNCTION_SDA) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_SCL) != OFF_GPIO) {
    bool force400khz = false;

    Wire.begin(ConfigESP->getGpio(FUNCTION_SDA), ConfigESP->getGpio(FUNCTION_SCL));
  }

#ifdef ARDUINO_ARCH_ESP32
  if (ConfigESP->getGpio(FUNCTION_SDA_2) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_SCL_2) != OFF_GPIO) {
    Wire1.begin(ConfigESP->getGpio(FUNCTION_SDA_2), ConfigESP->getGpio(FUNCTION_SCL_2));
    Wire1.setClock(100000);
  }
#endif
#endif

#ifdef SUPLA_ACTION_TRIGGER
  Supla::GUI::actionTrigger = new Supla::GUI::ActionTrigger[Supla::GUI::calculateElementCountActionTrigger()];
#endif

#if defined(SUPLA_RELAY) || defined(SUPLA_ROLLERSHUTTER)
  uint8_t rollershutters = ConfigManager->get(KEY_MAX_ROLLERSHUTTER)->getValueInt();

  for (nr = 0; nr < ConfigManager->get(KEY_MAX_RELAY)->getValueInt(); nr++) {
    if (rollershutters > 0) {
#ifdef SUPLA_ROLLERSHUTTER
      Supla::GUI::addRolleShutter(nr);
#endif
      rollershutters--;
      nr++;
    }
    else {
#ifdef SUPLA_RF_BRIDGE
      if (ConfigESP->getGpio(FUNCTION_RF_BRIDGE_TRANSMITTER) != OFF_GPIO &&
          ConfigManager->get(KEY_RF_BRIDGE_TYPE)->getElement(nr).toInt() == Supla::GUI::RFBridgeType::TRANSMITTER &&
          (strcmp(ConfigManager->get(KEY_RF_BRIDGE_CODE_ON)->getElement(nr).c_str(), "") != 0 ||
           strcmp(ConfigManager->get(KEY_RF_BRIDGE_CODE_OFF)->getElement(nr).c_str(), "") != 0)) {
        Supla::GUI::addRelayBridge(nr);
      }
      else {
        Supla::GUI::addRelayOrThermostat(nr);
      }
#else
      Supla::GUI::addRelayOrThermostat(nr);
#endif

      if (ConfigESP->getGpio(nr, FUNCTION_RELAY) != OFF_GPIO) {
#ifdef SUPLA_RF_BRIDGE
        if (ConfigManager->get(KEY_RF_BRIDGE_TYPE)->getElement(nr).toInt() == Supla::GUI::RFBridgeType::RECEIVER &&
            (strcmp(ConfigManager->get(KEY_RF_BRIDGE_CODE_ON)->getElement(nr).c_str(), "") != 0 ||
             strcmp(ConfigManager->get(KEY_RF_BRIDGE_CODE_OFF)->getElement(nr).c_str(), "") != 0)) {
          Supla::GUI::addButtonBridge(nr);
        }
#endif

#ifdef SUPLA_DIRECT_LINKS
        Supla::GUI::addDirectLinks(nr);
#endif
      }
    }
    delay(0);
  }
#endif

#ifdef SUPLA_LIMIT_SWITCH
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_LIMIT_SWITCH)->getValueInt(); nr++) {
    gpio = ConfigESP->getGpio(nr, FUNCTION_LIMIT_SWITCH);

    Supla::Sensor::Binary *binary = nullptr;

    if (gpio != OFF_GPIO) {
      binary = Supla::Control::GUI::Binary(gpio, ConfigESP->getPullUp(gpio), false, nr);
    }

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_BINARY, S_LIMIT_SWITCH, binary, nr);
#endif
  }
#endif

#ifdef SUPLA_PUSHOVER
  for (uint8_t nr = 0; nr < MAX_PUSHOVER_MESSAGE; nr++) {
    Supla::GUI::addPushover(nr);
  }
#endif

#ifdef SUPLA_WAKE_ON_LAN
  for (nr = 0; nr < ConfigManager->get(KEY_WAKE_ON_LAN_MAX)->getValueInt(); nr++) {
    if (strcmp(ConfigManager->get(KEY_WAKE_ON_LAN_MAC)->getElement(nr).c_str(), "") != 0) {
      new Supla::Control::WakeOnLanRelay(ConfigManager->get(KEY_WAKE_ON_LAN_MAC)->getElement(nr).c_str());
    }
  }
#endif

#ifdef SUPLA_CONFIG
  Supla::GUI::addConfigESP(ConfigESP->getGpio(FUNCTION_CFG_BUTTON), ConfigESP->getGpio(FUNCTION_CFG_LED));
#endif

#ifdef SUPLA_DS18B20
  if (ConfigESP->getGpio(FUNCTION_DS18B20) != OFF_GPIO) {
    Supla::GUI::addDS18B20MultiThermometer(ConfigESP->getGpio(FUNCTION_DS18B20));
  }
#endif

#ifdef SUPLA_DIRECT_LINKS_MULTI_SENSOR
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_DIRECT_LINKS_SENSOR)->getValueInt(); nr++) {
    if (strcmp(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(), "") != 0) {
      switch (ConfigManager->get(KEY_DIRECT_LINKS_TYPE)->getElement(nr).toInt()) {
        case DIRECT_LINKS_TYPE_TEMP: {
          auto directLinkSensorThermometer = new Supla::Sensor::DirectLinksThermometer(
              ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(), ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_DIRECT_LINKS_SENSOR_THERMOMETR, S_DIRECT_LINKS_SENSOR_THERMOMETR,
                                                      directLinkSensorThermometer, nr);
#endif
          break;
        }
        case DIRECT_LINKS_TYPE_TEMP_HYGR:
          new Supla::Sensor::DirectLinksThermHygroMeter(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(),
                                                        ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
          break;

        case DIRECT_LINKS_TYPE_PRESS:
          new Supla::Sensor::DirectLinksPressMeter(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(),
                                                   ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
          break;

        case DIRECT_LINKS_TYPE_ONE_PHASE_ELECTRICITY_METER:
          new Supla::Sensor::DirectLinksOnePhaseElectricityMeter(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(),
                                                                 ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
          break;

        case DIRECT_LINKS_TYPE_ELECTRICITY_METER:
          new Supla::Sensor::DirectLinksElectricityMeter(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(),
                                                         ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
          break;

        case DIRECT_LINKS_TYPE_DISTANCE:
          new Supla::Sensor::DirectLinksDistance(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(),
                                                 ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
          break;

        case DIRECT_LINKS_TYPE_DEPTH:
          new Supla::Sensor::DirectLinksDepth(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(),
                                              ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
          break;
      }
    }
  }
#endif

#ifdef SUPLA_DIRECT_LINKS_SENSOR_THERMOMETR
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_DIRECT_LINKS_SENSOR)->getValueInt(); nr++) {
    if (strcmp(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str(), "") != 0) {
      auto directLinkSensorThermometer = new Supla::Sensor::DirectLinksSensorThermometer(ConfigManager->get(KEY_SUPLA_SERVER)->getValue());
      directLinkSensorThermometer->setUrl(ConfigManager->get(KEY_DIRECT_LINKS_SENSOR)->getElement(nr).c_str());

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_DIRECT_LINKS_SENSOR_THERMOMETR, S_DIRECT_LINKS_SENSOR_THERMOMETR,
                                                  directLinkSensorThermometer, nr);
#endif
    }
  }
#endif

#ifdef SUPLA_DHT11
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_DHT11)->getValueInt(); nr++) {
    if (ConfigESP->getGpio(nr, FUNCTION_DHT11) != OFF_GPIO) {
      auto dht11 = new Supla::Sensor::DHT(ConfigESP->getGpio(nr, FUNCTION_DHT11), DHT11);
      correctionHandler.addThermHygroMeter(dht11);

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_DHT11, S_DHT11, dht11, nr);
#endif
    }
  }
#endif

#ifdef SUPLA_DHT22
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_DHT22)->getValueInt(); nr++) {
    if (ConfigESP->getGpio(nr, FUNCTION_DHT22) != OFF_GPIO) {
      auto dht22 = new Supla::Sensor::DHT(ConfigESP->getGpio(nr, FUNCTION_DHT22), DHT22);
      correctionHandler.addThermHygroMeter(dht22);

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_DHT22, S_DHT22, dht22, nr);
#endif
    }
  }
#endif

#ifdef SUPLA_SI7021_SONOFF
  if (ConfigESP->getGpio(FUNCTION_SI7021_SONOFF) != OFF_GPIO) {
    auto si7021sonoff = new Supla::Sensor::Si7021Sonoff(ConfigESP->getGpio(FUNCTION_SI7021_SONOFF));
    correctionHandler.addThermHygroMeter(si7021sonoff);
    improvSerialComponent->disable();

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_SI7021_SONOFF, S_SI7021_SONOFF, si7021sonoff);
#endif
  }
#endif

#ifdef SUPLA_HC_SR04
  if (ConfigESP->getGpio(FUNCTION_TRIG) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_ECHO) != OFF_GPIO) {
    Supla::Sensor::HC_SR04_NewPing *hcsr04;
    if (ConfigManager->get(KEY_HC_SR04_MAX_SENSOR_READ)->getValueInt() > 0) {
      hcsr04 = new Supla::Sensor::HC_SR04_NewPing(ConfigESP->getGpio(FUNCTION_TRIG), ConfigESP->getGpio(FUNCTION_ECHO), 0,
                                                  ConfigManager->get(KEY_HC_SR04_MAX_SENSOR_READ)->getValueInt(),
                                                  ConfigManager->get(KEY_HC_SR04_MAX_SENSOR_READ)->getValueInt(), 0);
    }
    else {
      hcsr04 = new Supla::Sensor::HC_SR04_NewPing(ConfigESP->getGpio(FUNCTION_TRIG), ConfigESP->getGpio(FUNCTION_ECHO));
    }

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_HC_SR04, S_HC_SR04, hcsr04);
#endif
  }
#endif

#ifdef GUI_SENSOR_SPI
  if (ConfigESP->getGpio(FUNCTION_CLK) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_CS) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_MISO) != OFF_GPIO) {
#ifdef SUPLA_MAX6675
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_SPI_MAX6675).toInt()) {
      auto thermocouple =
          new Supla::Sensor::MAX6675_K(ConfigESP->getGpio(FUNCTION_CLK), ConfigESP->getGpio(FUNCTION_CS), ConfigESP->getGpio(FUNCTION_MISO));
#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_MAX6675, S_MAX6675, thermocouple);
#endif
    }
#endif

#ifdef SUPLA_MAX31855
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_SPI_MAX31855).toInt()) {
      auto thermocouple =
          new Supla::Sensor::MAX31855(ConfigESP->getGpio(FUNCTION_CLK), ConfigESP->getGpio(FUNCTION_CS), ConfigESP->getGpio(FUNCTION_MISO));
#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_MAX31855, S_MAX31855, thermocouple);
#endif
    }
#endif

#ifdef SUPLA_CC1101
 Serial.println("wMBus-lib: TESTS:");
 Serial.print("Apator08");
uint8_t packet1[254] = { 0x73,0x44,0x14,0x86,0xDD,0x44,0x44,0x00,0x03,0x03,0xA0,0xB9,0xE5,0x27,0x00,0x4C,0x40,0x34,0xB3,0x1C,0xED,0x01,0x06,0xFF,0x01,0xD0,0x93,0x27,0x00,0x65,0xF0,0x22,0x00,0x96,0x61,0x23,0x00,0x54,0xD0,0x23,0x00,0xEC,0x49,0x24,0x00,0x18,0xB4,0x24,0x00,0x5F,0x01,0x25,0x00,0x93,0x6D,0x25,0x00,0xFF,0xD5,0x25,0x00,0x0E,0x3D,0x26,0x00,0x1E,0xAC,0x26,0x00,0x0B,0x20,0x27,0x00,0x03,0x00,0x00,0x00,0x00,0x37,0x1D,0x0B,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x2C,0x00,0x33,0x15,0x0C,0x01,0x0D,0x2F,0x00,0x00,0x00,0x00,0x00,0x00, };
std::vector<unsigned char> frame1(packet1, packet1 + packetSize1(packet1[0]));
Supla::Sensor::WmbusMeter* meter1 = new Supla::Sensor::WmbusMeter();
meter1->add_sensor(new Supla::Sensor::SensorBase("004444DD", "apator08", "total", ""));
float val1 = meter1->parse_frame(frame1);
 Serial.print("value: ");
 Serial.println(val1);

 Serial.print("Apator16-2");
uint8_t packet2[254] = { 0x6E,0x44,0x01,0x06,0x20,0x20,0x20,0x20,0x05,0x07,0x7A,0x9A,0x00,0x60,0x85,0x2F,0x2F,0x0F,0x0A,0x73,0x43,0x93,0xCC,0x00,0x00,0x43,0x5B,0x01,0x83,0x00,0x1A,0x54,0xE0,0x6F,0x63,0x02,0x91,0x34,0x25,0x10,0x03,0x0F,0x00,0x00,0x7B,0x01,0x3E,0x0B,0x00,0x00,0x3E,0x0B,0x00,0x00,0x3E,0x0B,0x00,0x00,0x3E,0x0B,0x00,0x00,0x3E,0x0B,0x00,0x00,0x3E,0x0B,0x00,0x00,0x3E,0x0B,0x00,0x00,0x65,0x00,0x00,0x00,0x3D,0x00,0x00,0x00,0x3D,0x00,0x00,0x00,0x3D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA0,0x91,0x0C,0xB0,0x03,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xA6,0x2B, };

std::vector<unsigned char> frame2(packet2, packet2 + packetSize1(packet2[0]));
Supla::Sensor::WmbusMeter* meter2 = new Supla::Sensor::WmbusMeter();
meter2->add_sensor(new Supla::Sensor::SensorBase("20202020", "apator162", "total", ""));

float val2 = meter2->parse_frame(frame2);
 Serial.print("value: ");
 Serial.println(val2);

 Serial.print("Iperl");
uint8_t packet3[254] = { 0x1E,0x44,0xAE,0x4C,0x99,0x56,0x34,0x12,0x68,0x07,0x7A,0x36,0x00,0x10,0x00,0x2F,0x2F,0x04,0x13,0x18,0x1E,0x00,0x00,0x02,0x3B,0x00,0x00,0x2F,0x2F,0x2F,0x2F, };
std::vector<unsigned char> frame3(packet3, packet3 + packetSize1(packet3[0]));
Supla::Sensor::WmbusMeter* meter3 = new Supla::Sensor::WmbusMeter();
meter3->add_sensor(new Supla::Sensor::SensorBase("12345699", "iperl", "total", ""));
float val3 = meter3->parse_frame(frame3);
 Serial.print("value: ");
 Serial.println(val3);

 Serial.print("mkradio4");
uint8_t packet4[254] = { 0x1E,0x44,0xAE,0x4C,0x99,0x56,0x34,0x12,0x68,0x07,0x7A,0x36,0x00,0x10,0x00,0x2F,0x2F,0x04,0x13,0x18,0x1E,0x00,0x00,0x02,0x3B,0x00,0x00,0x2F,0x2F,0x2F,0x2F, };
std::vector<unsigned char> frame4(packet4, packet4 + packetSize1(packet4[0]));
Supla::Sensor::WmbusMeter* meter4 = new Supla::Sensor::WmbusMeter();
meter4->add_sensor(new Supla::Sensor::SensorBase("02410120", "mkradio4", "total", ""));
float val4 = meter4->parse_frame(frame4);
 Serial.print("value: ");
 Serial.println(val4);


 Serial.println("wMBus-lib: TESTS END");
    if (ConfigManager->get(KEY_ACTIVE_SENSOR_2)->getElement(SENSOR_SPI_CC1101).toInt()) {

      int indexOfSensorType = ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_TYPE1).toInt();
      std::string sensorType = sensors_types[indexOfSensorType];
      std::string sensorId = ConfigManager->get(KEY_WMBUS_SENSOR_ID)->getElement(0).c_str();
      std::string sensorKey = ConfigManager->get(KEY_WMBUS_SENSOR_KEY)->getElement(0).c_str();
      int indexOfSensorProperty = ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_PROPERTY1).toInt();
      std::string sensorProperty = sensors_properties[indexOfSensorProperty];
      meter = new Supla::Sensor::WmbusMeter(ConfigESP->getGpio(FUNCTION_MOSI), ConfigESP->getGpio(FUNCTION_MISO), ConfigESP->getGpio(FUNCTION_CLK), ConfigESP->getGpio(FUNCTION_CS), ConfigESP->getGpio(FUNCTION_GDO0), ConfigESP->getGpio(FUNCTION_GDO2));
      Serial.println("wMBus-lib: Registered sensors:");
      meter->add_sensor(new Supla::Sensor::SensorInfo(sensorId, sensorType, sensorProperty, sensorKey));
      if(ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_ENABLED2).toInt()==1)
      {
          indexOfSensorType = ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_TYPE2).toInt();
          sensorType = sensors_types[indexOfSensorType];
          sensorId = ConfigManager->get(KEY_WMBUS_SENSOR_ID)->getElement(1).c_str();
          sensorKey = ConfigManager->get(KEY_WMBUS_SENSOR_KEY)->getElement(1).c_str();
          indexOfSensorProperty = ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_PROPERTY2).toInt();
          sensorProperty = sensors_properties[indexOfSensorProperty];
          meter->add_sensor(new Supla::Sensor::SensorInfo(sensorId, sensorType, sensorProperty, sensorKey));
      }

      if(ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_ENABLED3).toInt()==1)
      {
        indexOfSensorType = ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_TYPE3).toInt();
        sensorType = sensors_types[indexOfSensorType];
        sensorId = ConfigManager->get(KEY_WMBUS_SENSOR_ID)->getElement(2).c_str();
        sensorKey = ConfigManager->get(KEY_WMBUS_SENSOR_KEY)->getElement(2).c_str();
        indexOfSensorProperty = ConfigManager->get(KEY_WMBUS_SENSOR)->getElement(WMBUS_CFG_SENSOR_PROPERTY3).toInt();
        sensorProperty = sensors_properties[indexOfSensorProperty];
        meter->add_sensor(new Supla::Sensor::SensorInfo(sensorId, sensorType, sensorProperty, sensorKey));
      }
      Serial.println("------");
    }
#endif
  }
#endif

#ifdef SUPLA_NTC_10K
  if (ConfigESP->getGpio(FUNCTION_NTC_10K) != OFF_GPIO) {
    auto ntc10k = new Supla::Sensor::NTC10K(ConfigESP->getGpio(FUNCTION_NTC_10K));
    correctionHandler.addThermHygroMeter(ntc10k);

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_NTC_10K, S_NTC_10K, ntc10k);
#endif
  }
#endif

#ifdef SUPLA_MPX_5XXX
  if (ConfigESP->getGpio(FUNCTION_MPX_5XXX) != OFF_GPIO) {
    Supla::GUI::mpx = new Supla::Sensor::MPX_5XXX(ConfigESP->getGpio(FUNCTION_MPX_5XXX));
#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_MPX_5XXX, S_MPX_5XXX, Supla::GUI::mpx);
#endif

    Supla::Sensor::Percentage *mpxPercent;
    if (Supla::GUI::mpx->getThankHeight() != 0) {
      mpxPercent = new Supla::Sensor::Percentage(Supla::GUI::mpx, 0, Supla::GUI::mpx->getThankHeight() * 0.01);
    }
    else {
      mpxPercent = new Supla::Sensor::Percentage(Supla::GUI::mpx, 0, 100.0);
    }

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_MPX_5XXX_PERCENT, S_MPX_5XXX_PERCENT, mpxPercent);
#endif
  }
#endif

#ifdef SUPLA_ANALOG_READING_MAP
#ifdef ARDUINO_ARCH_ESP8266
  Supla::GUI::analog = new Supla::Sensor::AnalogRedingMap *[ConfigManager->get(KEY_MAX_ANALOG_READING)->getValueInt()];
  gpio = ConfigESP->getGpio(FUNCTION_ANALOG_READING);

  if (gpio != OFF_GPIO) {
    for (nr = 0; nr < ConfigManager->get(KEY_MAX_ANALOG_READING)->getValueInt(); nr++) {
      Supla::GUI::analog[nr] = new Supla::Sensor::AnalogRedingMap(gpio);
#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_ANALOG_READING_MAP, S_ANALOG_READING_MAP, Supla::GUI::analog[nr], nr);
#endif
    }
  }
#endif
#ifdef ARDUINO_ARCH_ESP32
  Supla::GUI::analog = new Supla::Sensor::AnalogRedingMap *[ConfigManager->get(KEY_MAX_ANALOG_READING)->getValueInt()];

  for (nr = 0; nr < ConfigManager->get(KEY_MAX_ANALOG_READING)->getValueInt(); nr++) {
    gpio = ConfigESP->getGpio(nr, FUNCTION_ANALOG_READING);
    if (gpio != OFF_GPIO) {
      Supla::GUI::analog[nr] = new Supla::Sensor::AnalogRedingMap(gpio);
#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_ANALOG_READING_MAP, S_ANALOG_READING_MAP, Supla::GUI::analog[nr], nr);
#endif
    }
  }
#endif
#endif

#ifdef SUPLA_VINDRIKTNING_IKEA
  if (ConfigESP->getGpio(FUNCTION_VINDRIKTNING_IKEA) != OFF_GPIO) {
#ifdef ARDUINO_ARCH_ESP32
    auto vindriktningIkea = new Supla::Sensor::VindriktningIkea(ConfigESP->getHardwareSerial(ConfigESP->getGpio(FUNCTION_VINDRIKTNING_IKEA)));
#else
    auto vindriktningIkea = new Supla::Sensor::VindriktningIkea(ConfigESP->getGpio(FUNCTION_VINDRIKTNING_IKEA));
#endif

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_VINDRIKTNING_IKEA, S_VINDRIKTNING_IKEA, vindriktningIkea);
#endif
    improvSerialComponent->disable();
  }
#endif

#ifdef SUPLA_PMSX003
  if (ConfigESP->getGpio(FUNCTION_PMSX003_RX) != OFF_GPIO) {
    Supla::Sensor::PMSx003 *pms;

    if (ConfigESP->getGpio(FUNCTION_PMSX003_TX) != OFF_GPIO) {
      pms = new Supla::Sensor::PMSx003(ConfigESP->getGpio(FUNCTION_PMSX003_RX), ConfigESP->getGpio(FUNCTION_PMSX003_TX));
    }
    else {
      pms = new Supla::Sensor::PMSx003(ConfigESP->getGpio(FUNCTION_PMSX003_RX));
    }

    new Supla::Sensor::PMS_PM01(pms);
    auto pmsPM25 = new Supla::Sensor::PMS_PM25(pms);
    new Supla::Sensor::PMS_PM10(pms);

#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_PMSX003, S_PMSX003_PM25, pmsPM25);
#endif
  }
#endif

#ifdef SUPLA_RGBW
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_RGBW)->getValueInt(); nr++) {
    Supla::GUI::addRGBWLeds(nr);
  }
#endif

#ifdef SUPLA_IMPULSE_COUNTER
  for (nr = 0; nr < ConfigManager->get(KEY_MAX_IMPULSE_COUNTER)->getValueInt(); nr++) {
    if (ConfigESP->getGpio(nr, FUNCTION_IMPULSE_COUNTER) != OFF_GPIO) {
      Supla::GUI::addImpulseCounter(nr);
    }
  }
#endif

#ifdef SUPLA_HLW8012
  if (ConfigESP->getGpio(FUNCTION_CF) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_CF1) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_SEL) != OFF_GPIO) {
    Supla::GUI::addHLW8012(ConfigESP->getGpio(FUNCTION_CF), ConfigESP->getGpio(FUNCTION_CF1), ConfigESP->getGpio(FUNCTION_SEL));
    improvSerialComponent->disable();
  }
#endif

#ifdef SUPLA_PZEM_V_3
  Supla::Sensor::ElectricityMeter *PZEMv3 = nullptr;
  int8_t pinRX1 = ConfigESP->getGpio(1, FUNCTION_PZEM_RX);
  int8_t pinTX1 = ConfigESP->getGpio(1, FUNCTION_PZEM_TX);
  int8_t pinRX2 = ConfigESP->getGpio(2, FUNCTION_PZEM_RX);
  int8_t pinTX2 = ConfigESP->getGpio(2, FUNCTION_PZEM_TX);
  int8_t pinRX3 = ConfigESP->getGpio(3, FUNCTION_PZEM_RX);
  int8_t pinTX3 = ConfigESP->getGpio(3, FUNCTION_PZEM_TX);

  if (pinRX1 != OFF_GPIO && pinTX1 != OFF_GPIO && pinRX2 != OFF_GPIO && pinTX2 != OFF_GPIO && pinRX3 != OFF_GPIO && pinTX3 != OFF_GPIO) {
#ifdef ARDUINO_ARCH_ESP32
    PZEMv3 = new Supla::Sensor::CustomThreePhasePZEMv3(&Serial, pinRX1, pinTX1, &Serial1, pinRX2, pinTX2, &Serial2, pinRX3, pinTX3);
#else
    PZEMv3 = new Supla::Sensor::CustomThreePhasePZEMv3(pinRX1, pinTX1, pinRX2, pinTX2, pinRX3, pinTX3);
#endif
  }
  else if (pinRX1 != OFF_GPIO && pinTX1 != OFF_GPIO && pinTX2 != OFF_GPIO && pinTX3 != OFF_GPIO) {
#ifdef ARDUINO_ARCH_ESP32
    PZEMv3 = new Supla::Sensor::CustomThreePhasePZEMv3(&Serial, pinRX1, pinTX1, &Serial1, pinRX1, pinTX2, &Serial2, pinRX1, pinTX3);
#else
    PZEMv3 = new Supla::Sensor::CustomThreePhasePZEMv3(pinRX1, pinTX1, pinRX1, pinTX2, pinRX1, pinTX3);
#endif
  }
  else if (pinRX1 != OFF_GPIO && pinTX1 != OFF_GPIO) {
#ifdef ARDUINO_ARCH_ESP32
    PZEMv3 = new Supla::Sensor::PZEMv3(&Serial, pinRX1, pinTX1);
#else
    PZEMv3 = new Supla::Sensor::PZEMv3(pinRX1, pinTX1);
#endif
  }

  if (PZEMv3) {
    improvSerialComponent->disable();
#ifdef SUPLA_CONDITIONS
    Supla::GUI::Conditions::addConditionsSensor(SENSOR_PZEM_V3, S_PZEM_V3, PZEMv3);
#endif
  }
#endif

#ifdef SUPLA_CSE7766
  if (ConfigESP->getGpio(FUNCTION_CSE7766_RX) != OFF_GPIO) {
    Supla::GUI::addCSE7766(ConfigESP->getGpio(FUNCTION_CSE7766_RX));
    improvSerialComponent->disable();
  }
#endif

#if defined(SUPLA_MODBUS_SDM) || defined(SUPLA_MODBUS_SDM_ONE_PHASE)
  if (ConfigESP->getGpio(FUNCTION_SDM_RX) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_SDM_TX) != OFF_GPIO) {
#if defined(SUPLA_MODBUS_SDM)
#ifdef ARDUINO_ARCH_ESP32
    Supla::GUI::smd = new Supla::Sensor::SDM630(
        ConfigESP->getHardwareSerial(ConfigESP->getGpio(FUNCTION_SDM_RX), ConfigESP->getGpio(FUNCTION_SDM_TX)), ConfigESP->getGpio(FUNCTION_SDM_RX),
        ConfigESP->getGpio(FUNCTION_SDM_TX), ConfigESP->getBaudRateSpeed(ConfigESP->getGpio(FUNCTION_SDM_RX)));
#else
    Supla::GUI::smd = new Supla::Sensor::SDM630(ConfigESP->getGpio(FUNCTION_SDM_RX), ConfigESP->getGpio(FUNCTION_SDM_TX),
                                                ConfigESP->getBaudRateSpeed(ConfigESP->getGpio(FUNCTION_SDM_RX)));

#endif
    if (ConfigESP->getBaudRate(ConfigESP->getGpio(FUNCTION_SDM_RX)) == BAUDRATE_38400) {
      Supla::GUI::smd->setRefreshRate(30);
    }
    else {
      Supla::GUI::smd->setRefreshRate(60);
    }
#endif

#if defined(SUPLA_MODBUS_SDM_ONE_PHASE)
#ifdef ARDUINO_ARCH_ESP32
    Supla::GUI::smd120 = new Supla::Sensor::SDM120(
        ConfigESP->getHardwareSerial(ConfigESP->getGpio(FUNCTION_SDM_RX), ConfigESP->getGpio(FUNCTION_SDM_TX)), ConfigESP->getGpio(FUNCTION_SDM_RX),
        ConfigESP->getGpio(FUNCTION_SDM_TX), ConfigESP->getBaudRateSpeed(ConfigESP->getGpio(FUNCTION_SDM_RX)));
#else
    Supla::GUI::smd120 = new Supla::Sensor::SDM120(ConfigESP->getGpio(FUNCTION_SDM_RX), ConfigESP->getGpio(FUNCTION_SDM_TX),
                                                   ConfigESP->getBaudRateSpeed(ConfigESP->getGpio(FUNCTION_SDM_RX)));

#endif
    if (ConfigESP->getBaudRate(ConfigESP->getGpio(FUNCTION_SDM_RX)) == BAUDRATE_38400) {
      Supla::GUI::smd120->setRefreshRate(10);
    }
    else {
      Supla::GUI::smd120->setRefreshRate(60);
    }

#endif
    improvSerialComponent->disable();
  }
#endif

#if defined(GUI_SENSOR_I2C) || defined(GUI_SENSOR_I2C_ENERGY_METER)
  if (ConfigESP->getGpio(FUNCTION_SDA) != OFF_GPIO && ConfigESP->getGpio(FUNCTION_SCL) != OFF_GPIO) {
    bool force400khz = false;

#ifdef SUPLA_BME280
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_BME280).toInt()) {
      Supla::Sensor::BME280 *bme280 = nullptr;

      switch (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_BME280).toInt()) {
        case BMx280_ADDRESS_0X76:
          bme280 = new Supla::Sensor::BME280(0x76, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());

#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BME280, S_BME280, bme280);
#endif
          break;
        case BMx280_ADDRESS_0X77:
          bme280 = new Supla::Sensor::BME280(0x77, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());

#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BME280, S_BME280, bme280);
#endif
          break;
        case BMx280_ADDRESS_0X76_AND_0X77:
          bme280 = new Supla::Sensor::BME280(0x76, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());
          Supla::Sensor::BME280 *bme280_1 = new Supla::Sensor::BME280(0x77, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());

#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BME280, S_BME280, bme280);
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BME280, S_BME280, bme280_1, 1);
#endif
          break;
      }
      if (bme280) {
        correctionHandler.addThermHygroMeter(bme280);
      }
    }
#endif

#ifdef SUPLA_BMP280
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_BMP280).toInt()) {
      Supla::Sensor::BMP280 *bmp280 = nullptr;

      switch (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_BMP280).toInt()) {
        case BMx280_ADDRESS_0X76:
          bmp280 = new Supla::Sensor::BMP280(0x76, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());

#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BMP280, S_BMP280, bmp280);
#endif
          break;
        case BMx280_ADDRESS_0X77:
          bmp280 = new Supla::Sensor::BMP280(0x77, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());

#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BMP280, S_BMP280, bmp280);
#endif
          break;
        case BMx280_ADDRESS_0X76_AND_0X77:
          bmp280 = new Supla::Sensor::BMP280(0x76, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());
          Supla::Sensor::BMP280 *bmp280_1 = new Supla::Sensor::BMP280(0x77, ConfigManager->get(KEY_ALTITUDE_BMX280)->getValueInt());

#ifdef SUPLA_CONDITIONS
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BMP280, S_BMP280, bmp280);
          Supla::GUI::Conditions::addConditionsSensor(SENSOR_BMP280, S_BMP280, bmp280_1, 1);
#endif
          break;
      }
      if (bmp280) {
        correctionHandler.addThermHygroMeter(bmp280);
      }
    }
#endif

#ifdef SUPLA_SHT3x
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_SHT3x).toInt()) {
      Supla::Sensor::SHT3x *sht3x = nullptr;
      Supla::Sensor::SHT3x *sht3x_1 = nullptr;

      switch (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_SHT3x).toInt()) {
        case SHT3x_ADDRESS_0X44:
          sht3x = new Supla::Sensor::SHT3x(0x44);
          break;
        case SHT3x_ADDRESS_0X45:
          sht3x_1 = new Supla::Sensor::SHT3x(0x45);
          break;
        case SHT3x_ADDRESS_0X44_AND_0X45:
          sht3x = new Supla::Sensor::SHT3x(0x44);
          sht3x_1 = new Supla::Sensor::SHT3x(0x45);
          break;
      }

      if (sht3x) {
#ifdef SUPLA_CONDITIONS
        Supla::GUI::Conditions::addConditionsSensor(SENSOR_SHT3x, S_SHT3X, sht3x);
#endif
        correctionHandler.addThermHygroMeter(sht3x);
      }

      if (sht3x_1) {
#ifdef SUPLA_CONDITIONS
        Supla::GUI::Conditions::addConditionsSensor(SENSOR_SHT3x, S_SHT3X, sht3x_1, 1);
#endif
        correctionHandler.addThermHygroMeter(sht3x_1);
      }
    }
#endif

#ifdef SUPLA_SHT_AUTODETECT
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_SHT3x).toInt()) {
      Supla::Sensor::SHTAutoDetect *shtAutoDetect = new Supla::Sensor::SHTAutoDetect();

      correctionHandler.addThermHygroMeter(shtAutoDetect);
#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_SHT3x, S_SHT3X, shtAutoDetect);
#endif
    }
#endif

#ifdef SUPLA_SI7021
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_SI7021).toInt()) {
      auto si7021 = new Supla::Sensor::Si7021();
      correctionHandler.addThermHygroMeter(si7021);

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_SI7021, S_SI702, si7021);
#endif
    }
#endif

#ifdef SUPLA_VL53L0X
    Supla::Sensor::VL_53L0X *vl53l0x = nullptr;

    switch (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_VL53L0X).toInt()) {
      case STATE_VL53L0X::STATE_VL53L0X_SENSE_DEFAULT:
        vl53l0x = new Supla::Sensor::VL_53L0X(VL53L0X_I2C_ADDR, Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
        break;
      case STATE_VL53L0X::STATE_VL53L0X_SENSE_LONG_RANGE:
        vl53l0x = new Supla::Sensor::VL_53L0X(VL53L0X_I2C_ADDR, Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
        break;
      case STATE_VL53L0X::STATE_VL53L0X_SENSE_HIGH_SPEED:
        vl53l0x = new Supla::Sensor::VL_53L0X(VL53L0X_I2C_ADDR, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
        break;
      case STATE_VL53L0X::STATE_VL53L0X_SENSE_HIGH_ACCURACY:
        vl53l0x = new Supla::Sensor::VL_53L0X(VL53L0X_I2C_ADDR, Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_ACCURACY);
        break;
    }
    if (vl53l0x) {
      force400khz = true;

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_VL53L0X, S_VL53L0X, vl53l0x);
#endif
    }
#endif

#ifdef SUPLA_HDC1080
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_HDC1080).toInt()) {
      auto hdc1080 = new Supla::Sensor::HDC1080();
      force400khz = true;

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_HDC1080, S_HDC1080, hdc1080);
#endif
    }
#endif

#ifdef SUPLA_BH1750
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_BH1750).toInt()) {
      auto bh1750 = new Supla::Sensor::BH1750();

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_BH1750, S_BH1750, bh1750);
#endif
    }
#endif

#ifdef SUPLA_MS5611
    if (ConfigManager->get(KEY_ACTIVE_SENSOR_2)->getElement(SENSOR_I2C_MS5611).toInt()) {
      auto ms5611 = new Supla::Sensor::MS5611Sensor(ConfigManager->get(KEY_ALTITUDE_MS5611)->getValueInt());
      /*
      #ifdef SUPLA_CONDITIONS
            Supla::GUI::Conditions::addConditionsSensor(SENSOR_MS5611, S_MS5611, ms5611);
      #endif
      */
    }
#endif

#ifdef SUPLA_MAX44009
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_MAX44009).toInt()) {
      auto max4409 = new Supla::Sensor::MAX_44009();

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_MAX44009, S_MAX44009, max4409);
#endif
    }
#endif

#ifdef SUPLA_AHTX0
    if (ConfigManager->get(KEY_ACTIVE_SENSOR_2)->getElement(SENSOR_I2C_AHTX0).toInt()) {
      auto aht = new Supla::Sensor::AHTX0();
      correctionHandler.addThermHygroMeter(aht);

#ifdef SUPLA_CONDITIONS
      Supla::GUI::Conditions::addConditionsSensor(SENSOR_AHTX0, S_AHTX0, aht);
#endif
    }
#endif

#ifdef SUPLA_OLED
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_OLED).toInt()) {
      SuplaOled *oled = new SuplaOled();
#ifdef SUPLA_BUTTON
#ifdef SUPLA_THERMOSTAT
      new Supla::Control::GUI::OledButtonController(oled, Supla::GUI::thermostatArray);
#else
      new Supla::Control::GUI::OledButtonController(oled);
#endif
#endif
    }
#endif

#ifdef SUPLA_LCD_HD44780
    if (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_HD44780).toInt()) {
      uint8_t lcdCols = 16, lcdRows = 2, addressLCD = 0x20;

      switch (ConfigManager->get(KEY_ACTIVE_SENSOR)->getElement(SENSOR_I2C_HD44780).toInt()) {
        case HD44780_ADDRESS_0X20:
          addressLCD = 0x20;
          break;
        case HD44780_ADDRESS_0X21:
          addressLCD = 0x21;
          break;
        case HD44780_ADDRESS_0X22:
          addressLCD = 0x22;
          break;
        case HD44780_ADDRESS_0X23:
          addressLCD = 0x23;
          break;
        case HD44780_ADDRESS_0X24:
          addressLCD = 0x24;
          break;
        case HD44780_ADDRESS_0X25:
          addressLCD = 0x25;
          break;
        case HD44780_ADDRESS_0X26:
          addressLCD = 0x26;
          break;
        case HD44780_ADDRESS_0X27:
          addressLCD = 0x27;
          break;
        case HD44780_ADDRESS_0X38:
          addressLCD = 0x38;
          break;
        case HD44780_ADDRESS_0X3F:
          addressLCD = 0x3F;
          break;
      }

      switch (ConfigManager->get(KEY_HD44780_TYPE)->getValueInt()) {
        case LCD_2X16:
          lcdCols = 16;
          lcdRows = 2;
          break;
        case LCD_2X20:
          lcdCols = 20;
          lcdRows = 2;
          break;
        case LCD_4X16:
          lcdCols = 16;
          lcdRows = 4;
          break;
        case LCD_4X20:
          lcdCols = 20;
          lcdRows = 4;
          break;
      }

      SuplaLCD *lcd = new SuplaLCD(addressLCD, lcdCols, lcdRows);
      lcd->setup(ConfigManager, ConfigESP);
    }
#endif

#ifdef SUPLA_ADE7953
    if (ConfigESP->getGpio(FUNCTION_ADE7953_IRQ) != OFF_GPIO) {
      Supla::GUI::addADE7953(ConfigESP->getGpio(FUNCTION_ADE7953_IRQ));
    }
#endif

    if (force400khz)
      Wire.setClock(400000);
  }
#endif

#ifdef SUPLA_ACTION_TRIGGER
  for (nr = 0; nr < Supla::GUI::calculateElementCountActionTrigger(); nr++) {
    Supla::GUI::addButtonActionTrigger(nr);
  }
  delete Supla::GUI::actionTrigger;
#endif

#ifdef DEBUG_MODE
  new Supla::Sensor::EspFreeHeap();
#endif

#ifdef SUPLA_DEEP_SLEEP
  if (ConfigManager->get(KEY_DEEP_SLEEP_TIME)->getValueInt() > 0) {
    new Supla::Control::DeepSleep(ConfigManager->get(KEY_DEEP_SLEEP_TIME)->getValueInt() * 60);
  }
#endif

#ifdef SUPLA_CONDITIONS
  Supla::GUI::Conditions::addConditions();
#endif

  Supla::GUI::begin();
}

void loop() {
  const uint32_t now = millis();
  SuplaDevice.iterate();

  #ifndef SUPLA_CC1101
  uint32_t delay_time = LOOP_INTERVAL;
  if (now - last_loop < LOOP_INTERVAL)
    delay_time = LOOP_INTERVAL - (now - last_loop);

  delay(delay_time);

  last_loop = now;
  #endif
}
