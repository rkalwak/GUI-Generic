#ifdef SUPLA_CC1101

#include "WmbusMeter.h"
#include <string>
namespace Supla
{
  namespace Sensor
  {   
    WmbusMeter::WmbusMeter(uint8_t mosi, uint8_t miso, uint8_t clk, uint8_t cs, uint8_t gdo0, uint8_t gdo2, bool debugMode)
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
      if(debugMode)
      {
        Serial.println("wMBus-lib: Debug mode enabled.");
        return;
      }
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
      this->sensors_.insert({sensor->get_meter_id(), sensor});
      Serial.println("------");
      Serial.print("Type:");
      Serial.println(sensor->get_type().c_str());
      Serial.print("Id:");
      Serial.println(sensor->get_meter_id().c_str());
      Serial.print("Key:");
      Serial.println(sensor->get_key_string().c_str());
      Serial.print("Property:");
      Serial.println(sensor->get_property_to_send().c_str());
      Serial.println("------");
    }
    
     std::string WmbusMeter::extract_meter_id_string(const std::vector<unsigned char> &frame) {
    char raw[9] = {};
    snprintf(raw, sizeof(raw), "%02X%02X%02X%02X",
             frame[4], frame[5], frame[6], frame[7]);
    // pair-swap: [6][7][4][5][2][3][0][1]
    char id[9] = {};
    id[0] = raw[6]; id[1] = raw[7];
    id[2] = raw[4]; id[3] = raw[5];
    id[4] = raw[2]; id[5] = raw[3];
    id[6] = raw[0]; id[7] = raw[1];
    return std::string(id, 8);
}

    bool WmbusMeter::decrypt_telegram(std::vector<unsigned char> &telegram, std::vector<unsigned char> key)
    {
      bool ret_val = false;
      std::vector<unsigned char>::iterator pos;
      // CI
      pos = telegram.begin() + 10;
      Serial.print("wMBus-lib: CI field value: 0x");
      Serial.println(*pos, HEX);
      // data offset
      int offset{0};

      unsigned char iv[16];
      int i = 0;

      if ((*pos == 0x67) || (*pos == 0x6E) || (*pos == 0x74) ||
          (*pos == 0x7A) || (*pos == 0x7D) || (*pos == 0x7F) || (*pos == 0x9E))
      {
        Serial.println("wMBus-lib: CI matched Format A (short header)");
        offset = 15;
        Serial.print("wMBus-lib: Data offset set to: ");
        Serial.println(offset);

        // dll-mfct + dll-id + dll-version + dll-type
        for (int j = 0; j < 8; ++j)
        {
          iv[i++] = telegram[2 + j];
        }
        // tpl-acc
        for (int j = 0; j < 8; ++j)
        {
          iv[i++] = telegram[11];
        }
      }
      else if ((*pos == 0x68) || (*pos == 0x6F) || (*pos == 0x72) ||
               (*pos == 0x75) || (*pos == 0x7C) || (*pos == 0x7E) || (*pos == 0x9F))
      {
        Serial.println("wMBus-lib: CI matched Format B (long header)");
        offset = 23;
        Serial.print("wMBus-lib: Data offset set to: ");
        Serial.println(offset);

        // tpl-mfct
        for (int j = 0; j < 2; ++j)
        {
          iv[i++] = telegram[15 + j];
        }
        // tpl-id
        for (int j = 0; j < 4; ++j)
        {
          iv[i++] = telegram[11 + j];
        }
        // tpl-version + tpl-type
        for (int j = 0; j < 2; ++j)
        {
          iv[i++] = telegram[17 + j];
        }
        // tpl-acc
        for (int j = 0; j < 8; ++j)
        {
          iv[i++] = telegram[19];
        }
      }
      else if (*pos == 0xA0 || *pos == 0xA1)
      {
        // Apator NA-1 manufacturer-specific format.
        // Encrypted payload starts at byte 13; IV construction identical to Format A.
        Serial.println("wMBus-lib: CI matched Apator NA-1 manufacturer-specific format");
        offset = 13;
        Serial.print("wMBus-lib: Data offset set to: ");
        Serial.println(offset);

        // dll-mfct + dll-id + dll-version + dll-type (bytes 2..9)
        for (int j = 0; j < 8; ++j)
        {
          iv[i++] = telegram[2 + j];
        }
        // access counter at byte 11, repeated 8 times
        for (int j = 0; j < 8; ++j)
        {
          iv[i++] = telegram[11];
        }
      }
      else if (*pos == 0x8C)
      {
        // ELL I (Extended Link Layer I): 3 bytes at 10-12 (CI, CC, ACC).
        // TPL CI follows at byte 13; only short header (0x7A) is supported here.
        unsigned char tpl_ci = (telegram.size() > 13) ? telegram[13] : 0x00;
        if (tpl_ci == 0x7A)
        {
          Serial.println("wMBus-lib: CI matched ELL I + TPL short header (0x8C + 0x7A)");
          // Encrypted payload starts at byte 18 (after 0x2F 0x2F check bytes).
          offset = 18;
          Serial.print("wMBus-lib: Data offset set to: ");
          Serial.println(offset);

          // IV: dll-mfct + dll-id + dll-version + dll-type (bytes 2..9)
          for (int j = 0; j < 8; ++j)
          {
            iv[i++] = telegram[2 + j];
          }
          // tpl-acc is at byte 14, repeated 8 times
          for (int j = 0; j < 8; ++j)
          {
            iv[i++] = telegram[14];
          }
        }
        else
        {
          Serial.print("wMBus-lib: ELL I with unsupported TPL CI: 0x");
          Serial.println(tpl_ci, HEX);
          return false;
        }
      }
      else
      {
        Serial.print("wMBus-lib: CI unknown - value 0x");
        Serial.print(*pos, HEX);
        Serial.println(" not recognized");
        // Return false without modifying the telegram to prevent data corruption.
        return false;
      }

      // Parse tpl-cfg to get number of encrypted blocks
      // tpl-cfg is at bytes 13-14 (after CRC removal) for Format A
      // tpl-cfg format: bits 4-7 contain nb (number of encrypted blocks)
      uint16_t tpl_cfg = 0;
      uint8_t num_encr_blocks = 0;
      
      if (offset == 15) { // Format A
        if (telegram.size() >= 15) {
          tpl_cfg = ((uint16_t)telegram[14] << 8) | telegram[13];  // Little-endian
          num_encr_blocks = (tpl_cfg >> 4) & 0x0F;
          Serial.print("wMBus-lib: tpl-cfg = 0x");
          Serial.print(tpl_cfg, HEX);
          Serial.print(", num_encr_blocks = ");
          Serial.println(num_encr_blocks);
        }
      } else if (offset == 23) { // Format B
        if (telegram.size() >= 23) {
          tpl_cfg = ((uint16_t)telegram[22] << 8) | telegram[21];  // Little-endian
          num_encr_blocks = (tpl_cfg >> 4) & 0x0F;
          Serial.print("wMBus-lib: tpl-cfg = 0x");
          Serial.print(tpl_cfg, HEX);
          Serial.print(", num_encr_blocks = ");
          Serial.println(num_encr_blocks);
        }
      } else if (offset == 18) { // ELL I + TPL short header
        if (telegram.size() >= 18) {
          tpl_cfg = (uint16_t)telegram[16] | ((uint16_t)telegram[17] << 8);  // Little-endian
          num_encr_blocks = (tpl_cfg >> 4) & 0x0F;
          Serial.print("wMBus-lib: tpl-cfg = 0x");
          Serial.print(tpl_cfg, HEX);
          Serial.print(", num_encr_blocks = ");
          Serial.println(num_encr_blocks);
        }
      }

      pos = telegram.begin() + offset;
      int num_encrypted_bytes = 0;
      int num_not_encrypted_at_end = 0;

      if (decrypt_TPL_AES_CBC_IV(telegram, pos, key, iv,
                                 &num_encrypted_bytes, &num_not_encrypted_at_end, num_encr_blocks))
      {
        Serial.println("wMBus-lib: decrypt_TPL_AES_CBC_IV returned true");
        // For Apator NA-1 (offset=13, manufacturer-specific), skip the standard 0x2F2F check
        // because the proprietary plaintext does not begin with those bytes.
        if (offset == 13)
        {
          Serial.println("wMBus-lib: Decryption successful (Apator NA-1 manufacturer-specific)!");
          ret_val = true;
        }
        else
        {
          uint32_t decrypt_check = 0x2F2F;
          uint32_t dc = (((uint32_t)telegram[offset] << 8) | ((uint32_t)telegram[offset + 1]));
          Serial.print("wMBus-lib: Decrypt check value: 0x");
          Serial.print(dc, HEX);
          Serial.print(" (expected: 0x");
          Serial.print(decrypt_check, HEX);
          Serial.println(")");
          if (dc == decrypt_check)
          {
            Serial.println("wMBus-lib: Decryption successful!");
            ret_val = true;
          }
          else
          {
            Serial.println("wMBus-lib: Decryption failed, wrong key?");
          }
        }
      }
      else
      {
        Serial.println("wMBus-lib: decrypt_TPL_AES_CBC_IV returned false");
      }
      return ret_val;
    }

    float WmbusMeter::parse_frame(std::vector<unsigned char> &frame)
    {
      std::string telegram = format_hex_pretty(frame);
      telegram.erase(std::remove(telegram.begin(), telegram.end(), '.'), telegram.end());
      Serial.println("wMBus-lib: Telegram after removing dots:");
      Serial.println(telegram.c_str());
      Serial.println("wMBus-lib: Getting meter id.");
      std::string meterIdString = telegram.substr(8, 8);
      char s[9]= {0,0,0,0,0,0,0,0,0};
      s[0]=meterIdString[6];
      s[1]=meterIdString[7];
      s[2]=meterIdString[4];
      s[3]=meterIdString[5];
      s[4]=meterIdString[2];
      s[5]=meterIdString[3];
      s[6]=meterIdString[0];
      s[7]=meterIdString[1];
      std::string meterIdRealString = s;

      // either we have sensors defined or we have just one without ID (like Izar)
      // Determine which sensors match this meter ID
      std::string lookupId = meterIdRealString;
      bool isIzarCase = (sensors_.count(meterIdRealString) == 0 &&
                         sensors_.count("") > 0);
      if (isIzarCase) {
        lookupId = "";
      }

      if(sensors_.count(lookupId) > 0)
      {
        Serial.println("wMBus-lib: Getting sensor config.");
        auto range = sensors_.equal_range(lookupId);
        // Use the first matching sensor to get key and type for decryption
        auto firstSensor = range.first->second;
        bool isOk = true;
        float readValue = 0.0;
        if (firstSensor->get_key().size() > 0)
        {
          Serial.println("wMBus-lib: Key provided, decrypting frame.");
          if (!decrypt_telegram(frame, firstSensor->get_key()))
          {
            isOk = false;
          }
        }
        if (isOk)
        {
          Serial.print("wMBus-lib: decrypted frame: ");
          for(size_t i = 0; i < frame.size(); i++) {
            if(frame[i] < 0x10) Serial.print("0");
            Serial.print(frame[i], HEX);
          }
          Serial.println();
          if(this->drivers_.count(firstSensor->get_type()))
          {
            Serial.println("wMBus-lib: Getting driver.");
            auto driver = this->drivers_[firstSensor->get_type()];
            auto mapValues = driver->get_values(frame);
            Serial.println("wMBus-lib: Available map values:");
            for (auto const &entry : mapValues)
            {
              Serial.print("  ");
              Serial.print(entry.first.c_str());
              Serial.print(" = ");
              Serial.println(entry.second);
            }
            // Iterate over ALL sensors matching this meter ID
            for (auto it = range.first; it != range.second; ++it)
            {
              auto sensor = it->second;
              Serial.print("wMBus-lib: Property: ");
              Serial.println(sensor->get_property_to_send().c_str());
              float value = mapValues[sensor->get_property_to_send()];
              Serial.print("Meter id as string: ");
              Serial.println(meterIdRealString.c_str());
              Serial.print("Value read: ");
              Serial.println(value);
              sensor->setNewValue((uint64_t)(value * 1000));
              sensor->iterateAlways();
              // Return the first sensor's value for backward compatibility
              if (it == range.first) {
                readValue = value;
              }
            }
          }
          else
          {
            Serial.print("wMBus-lib: Driver for sensor: ");
            Serial.print(firstSensor->get_type().c_str());
            Serial.println(" does not exist.");
          }
        }
        else
        {
          Serial.println("wMBus-lib: Failed to decrypt telegram.");
        }
        return readValue;
      }
      else
      {
        Serial.print("wMBus-lib: Config for meter: ");
        Serial.print(meterIdRealString.c_str());
        Serial.println(" does not exist.");
      }
      return 0.0;
    }

    void WmbusMeter::onFastTimer()
    {
      if (receiver.task())
      {
        Serial.println("wMBus-lib: Found telegram.");
        dumpHex(receiver.MBpacket, packetLength);
        WMbusFrame mbus_data = receiver.get_frame();
        lastRssi = mbus_data.rssi;
        lastLqi  = mbus_data.lqi;
        Serial.printf("wMBus-lib: RSSI: %d dBm, LQI: %u\n",
                      (int)lastRssi, (unsigned)lastLqi);
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
#endif