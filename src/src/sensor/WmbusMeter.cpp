#include "WmbusMeter.h"
#include <string>
#include "DriversWmbusMeters/meters.h"
namespace Supla
{
  namespace Sensor
  {   
    WmbusMeter::WmbusMeter(uint8_t mosi, uint8_t miso, uint8_t clk, uint8_t cs, uint8_t gdo0, uint8_t gdo2)
    {
      Serial.print("wMBus-lib: Initializing with GPIO: ");
      Serial.print(mosi);
      Serial.print(",");
      Serial.print(miso);
      Serial.print(",");
      Serial.print(clk);
      Serial.print(",");
      Serial.print(cs);
      Serial.print(",");
      Serial.print(gdo0);
      Serial.print(",");
      Serial.print(gdo2);
      Serial.println(" GPIO");
      bool isInitialized = receiver.init(mosi, miso, clk, cs, gdo0, gdo2);
      if (isInitialized)
      {
        Serial.println("wMBus-lib: Receiver started.");
      }
    };

    void WmbusMeter::add_driver(Driver *driver)
    {
      this->drivers_[driver->get_name()] = driver;
    }

    void WmbusMeter::add_sensor(SensorBase *sensor)
    {
      this->sensors_[sensor->get_meter_id()] = sensor;
      Serial.println("------");
      Serial.print("Type:");
      Serial.println(sensor->get_type().c_str());
      Serial.print("Id:");
      Serial.println(sensor->get_meter_id().c_str());
      Serial.print("Key:");
      Serial.println(sensor->get_key_string().c_str());
      Serial.print("Property:");
      Serial.println(sensor->get_property_to_send().c_str());
    }

    float WmbusMeter::parse_frame(std::vector<unsigned char> &frame)
    {
      Serial.println("wMBus-lib: Formatting as string.");
      MeterInfo mi;
      mi.parse("apator08", "apator08", "00065158", "");
      auto meter1 = createMeter(&mi);
      AboutTelegram about;
      string id = "00065158";
      bool id_match = false;
      Telegram* tt = new Telegram();
      meter1->handleTelegram(about, frame, false, &id, &id_match, tt);
      double val = meter1->getNumericValue("total", Unit::M3);
      return val;
    }

    void WmbusMeter::onFastTimer()
    {
      if (receiver.task())
      {
        Serial.println("wMBus-lib: Found telegram.");
        dumpHex(receiver.MBpacket, packetLength);
        WMbusFrame mbus_data = receiver.get_frame();
        std::vector<unsigned char> frame = mbus_data.frame;
        Serial.println("----------------");
        Serial.println("wMBus-lib: Parsing frame.");
        parse_frame(frame);
        Serial.println("----------------");
      }
    }

  };
};

Supla::Sensor::WmbusMeter *meter;