/*
 * Unit tests for Apator162 wM-Bus driver register parsing.
 *
 * Reference telegram (after CRC removal - from wmbusmeters):
 *   3e4401068798710205077a22003085e6912a1aff1edc7063564753b86b86b0b03bf9cd01bac65720ae4219999dff16d3969bb49f841fa8fac61ba8e488dede
 *
 * Frame structure (after CRC removal):
 *   Byte 10: 0x7A = CI field (TPL layer, AES encrypted)
 *   Byte 11: 0x22 = Access number
 *   Byte 12: 0x00 = Status
 *   Bytes 13-14: 0x30 0x85 = Config (AES_CBC_IV)
 *   After decryption starts at byte 15: should be 0x2F 0x2F
 *
 * Meter ID: 02719887
 * Key: 00000000000000000000000000000000
 *
 * Expected decoded values:
 *   total_water_m3: 242.956
 */

#include <unity.h>
#include "../../lib/wmbus/src/Drivers/driver_apator_16_2.h"
#include "../../lib/wmbus/src/crc.hpp"
#include "../../lib/wmbus/src/utils.hpp"
#include "../../lib/wmbus/src/SensorBase.h"

#define SUPLA_CC1101
#ifdef SUPLA_CC1101
#include "../../src/src/sensor/WmbusMeter.h"
#endif
#include <mbus_packet.hpp>

// ---------------------------------------------------------------------------
// Reference frame from wmbusmeters (already has CRCs stripped)
// This is the frame as it appears after CC1101 receive + CRC removal
// ---------------------------------------------------------------------------
//
static uint8_t* build_apator162_frame_no_crc() {
  static uint8_t frame[] = {0x3E, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07, 0x7A, 0x22, 0x00, 0x30, 0x85, 0xE6,
                            0x91, 0x2A, 0x1A, 0xFF, 0x1E, 0xDC, 0x70, 0x63, 0x56, 0x47, 0x53, 0xB8, 0x6B, 0x86, 0xB0, 0xB0,
                            0x3B, 0xF9, 0xCD, 0x01, 0xBA, 0xC6, 0x57, 0x20, 0xAE, 0x42, 0x19, 0x99, 0x9D, 0xFF, 0x16, 0xD3,
                            0x96, 0x9B, 0xB4, 0x9F, 0x84, 0x1F, 0xA8, 0xFA, 0xC6, 0x1B, 0xA8, 0xE4, 0x88, 0xDE, 0xDE, 0x00};

  Serial.println("!!! FRAME WITHOUT CRCs (as from wmbusmeters) !!!");
  Serial.print("Frame size: ");
  Serial.println(sizeof(frame));
  Serial.print("Byte[10] (CI field): 0x");
  Serial.println(frame[10], HEX);

  return frame;
}

// 3e4401068798710205077a22003085e6912a1aff1edc7063564753b86b86b0b03bf9cd01bac65720ae4219999dff16d3969bb49f841fa8fac61ba8e488dede
static uint8_t* build_apator162_frame_with_crc() {
  static uint8_t frame[] = {0x3E, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07, 0x7A, 0x22, 0x00, 0x30, 0x85, 0xE6,
                            0x91, 0x2A, 0x1A, 0xFF, 0x1E, 0xDC, 0x70, 0x63, 0x56, 0x47, 0x53, 0xB8, 0x6B, 0x86, 0xB0, 0xB0,
                            0x3B, 0xF9, 0xCD, 0x01, 0xBA, 0xC6, 0x57, 0x20, 0xAE, 0x42, 0x19, 0x99, 0x9D, 0xFF, 0x16, 0xD3,
                            0x96, 0x9B, 0xB4, 0x9F, 0x84, 0x1F, 0xA8, 0xFA, 0xC6, 0x1B, 0xA8, 0xE4, 0x88, 0xDE, 0xDE, 0x00};

  Serial.println("!!! FRAME WITH CRCs (as from wmbusmeters) !!!");
  Serial.print("Frame size: ");
  Serial.println(sizeof(frame));
  Serial.print("Byte[10] (CI field): 0x");
  Serial.println(frame[10], HEX);

  return frame;
}

// ---------------------------------------------------------------------------
// Helpers that replicate WmbusMeter::parse_frame meter-ID extraction
// ---------------------------------------------------------------------------

static std::string extract_meter_id_number_string(const std::vector<unsigned char>& frame) {
  char raw[9] = {};
  snprintf(raw, sizeof(raw), "%02X%02X%02X%02X", frame[4], frame[5], frame[6], frame[7]);
  return std::string(raw, 8);
}

static std::map<std::string, float> values;
static Supla::Sensor::WmbusMeter* wmbus_meter = nullptr;
static Apator162* wmbus_driver = nullptr;
static Supla::Sensor::SensorBase* wmbus_sensor = nullptr;
static std::vector<unsigned char> full_frame;      // CRC-stripped, ready for driver
static std::vector<unsigned char> full_frame_raw;  // original bytes as received
static std::map<std::string, float> full_frame_values;

void setUp() {
  auto raw = build_apator162_frame_no_crc();
  // Use real WmbusMeter::decrypt_telegram method
  wmbus_meter = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);  // debugMode=true to skip actual hardware init
  wmbus_driver = new Apator162();
  wmbus_meter->add_driver(wmbus_driver);

  // Meter ID: 02719887
  wmbus_sensor = new Supla::Sensor::SensorBase("02719887", "apator162", "total_water_m3", "00000000000000000000000000000000");
  wmbus_meter->add_sensor(wmbus_sensor);
}

void tearDown() {
  values.clear();
  if (wmbus_meter) {
    delete wmbus_meter;
    wmbus_meter = nullptr;
  }
}

static void setUp_full_frame() {
  uint8_t* frame_ptr = build_apator162_frame_no_crc();
  // Frame is already without CRCs, use it directly
  std::vector<unsigned char> telegram(frame_ptr, frame_ptr + 63);  // 0x3F = 63 bytes
  full_frame = telegram;
}

void test_driver_name() {
  TEST_ASSERT_EQUAL_STRING("apator162", wmbus_driver->get_name().c_str());
}

void test_full_frame_crc_strip_length() {
  // Frame is already stripped of CRCs, length should be 63 (0x3F)
  setUp_full_frame();
  size_t stripped_len = full_frame.size();
  TEST_ASSERT_EQUAL(63u, (unsigned)stripped_len);  // 0x3F = 63
}

void test_full_frame_ci_field_after_strip() {
  // After CRC stripping byte[10] must be the CI field = 0x7A
  setUp_full_frame();
  unsigned char ci = full_frame[10];
  TEST_ASSERT_EQUAL_HEX8(0x7A, ci);
}

void test_full_frame_meter_id_string() {
  // Meter-ID bytes live in the DLL header (bytes 4-7), which are never
  // encrypted and survive CRC stripping unchanged.
  // WmbusMeter::parse_frame swaps pairs → "02719887".
  setUp_full_frame();
  std::string id = wmbus_meter->extract_meter_id_string(full_frame);
  TEST_ASSERT_EQUAL_STRING("02719887", id.c_str());
}

void test_full_frame_meter_id_number_string() {
  // Raw BCD-LE bytes 4-7 as uppercase hex → "87987102" ("Meter id as number").
  setUp_full_frame();
  std::string id = extract_meter_id_number_string(full_frame);
  TEST_ASSERT_EQUAL_STRING("87987102", id.c_str());
}

void test_full_frame_driver_name() {
  // Confirm the driver associated with this meter type.
  TEST_ASSERT_EQUAL_STRING("apator162", wmbus_driver->get_name().c_str());
}

void test_full_frame_device_type_byte() {
  // Byte 9 of the DLL is the device-type field; 0x07 = water meter (EN 13757).
  setUp_full_frame();
  unsigned char device_type = full_frame[9];
  TEST_ASSERT_EQUAL_HEX8(0x07, device_type);
}

void text_crc_removal() {
      String withoutCRC = "3e4401068798710205077a7c0030851667fdf92e43ffe2e3aad3145a28bb7fe73dd1c405a7971f6e69b527346439736d130000000000000000000000000000";
   // 3E4401068798710205074CDB7A7C0030851667FDF92E43FFE2E3AAD33649145A28BB7FE73DD1C405A7971F6E69B5E05227346439736D131303F537F06417593E4FC3C142D51716C5730000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
  uint8_t apatorminarray2[254] = {0x3E, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07, 0x4C, 0xDB, 0x7A, 0x7C, 0x00, 0x30, 0x85, 0x16, 0x67, 0xFD, 0xF9, 0x2E,
                                  0x43, 0xFF, 0xE2, 0xE3, 0xAA, 0xD3, 0x36, 0x49, 0x14, 0x5A, 0x28, 0xBB, 0x7F, 0xE7, 0x3D, 0xD1, 0xC4,
                                  0x05, 0xA7, 0x97, 0x1F, 0x6E, 0x69, 0xB5, 0xE0, 0x52, 0x27, 0x34, 0x64, 0x39, 0x73, 0x6D, 0x13,
                                  };
  uint8_t len_without_crc = crcRemove(apatorminarray2, packetSize(apatorminarray2[0]));
  std::vector<unsigned char> frame(apatorminarray2, apatorminarray2 + len_without_crc);
  // convert to hex string for easy comparison
  String frame_hex;
  for (size_t i = 0; i < frame.size(); i++) {
    if (frame[i] < 0x10)
      frame_hex += "0";
    frame_hex += String(frame[i], HEX);
  }
  TEST_ASSERT_EQUAL_STRING(withoutCRC.c_str(), frame_hex.c_str());
}

void test_wmbus_meter_parse_all_values() {
  uint8_t* frame_ptr = build_apator162_frame_no_crc();

  Serial.println("=== FRAME READY FOR PARSING ===");
  Serial.print("Frame size: 63 bytes (no CRCs)");
  Serial.println();

  std::vector<unsigned char> frame(frame_ptr, frame_ptr + 63);

  // Use parse_frame method which handles decryption and value extraction
  float result = wmbus_meter->parse_frame(frame);

  // parse_frame returns the value for the configured property (total_water_m3)
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 242.956f, result);
}

void test_long_frame(){
    auto meterT = new Supla::Sensor::WmbusMeter(1,2,3,4,6,7,true); 
  meterT->add_driver(new Apator162());
  // 00000000000000000000000000000000
  std::vector<unsigned char> keyT{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // 6E4401068798710205070F977AF70060852585B973EF73E80EB54F6FE2A6344641920366F1FA4E23813A35D994D0BE59372C9FCCD117B03200E9B825C56065AC3FD2F648D935EFF04D7E1DFA72C0803AC667FB1F5A5DAB4387643A22E68894C7356D2A9D49A28B1A86235ED4C7E367D2DE9F98BCB7954E10C7F770B8DFC1610000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
  uint8_t apatorminarray[254] = {0x6E, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07, 0x0F, 0x97, 0x7A, 0xF7, 0x00, 0x60, 0x85, 0x25, 0x85, 0xB9, 0x73, 0xEF, 0x73, 0xE8, 0x0E, 0xB5, 0x4F, 0x6F, 0xE2, 0xA6, 0x34, 0x46, 0x41, 0x92, 0x03, 0x66, 0xF1, 0xFA, 0x4E, 0x23, 0x81, 0x3A, 0x35, 0xD9, 0x94, 0xD0, 0xBE, 0x59, 0x37, 0x2C, 0x9F, 0xCC, 0xD1, 0x17, 0xB0, 0x32, 0x00, 0xE9, 0xB8, 0x25, 0xC5, 0x60, 0x65, 0xAC, 0x3F, 0xD2, 0xF6, 0x48, 0xD9, 0x35, 0xEF, 0xF0, 0x4D, 0x7E, 0x1D, 0xFA, 0x72, 0xC0, 0x80, 0x3A, 0xC6, 0x67, 0xFB, 0x1F, 0x5A, 0x5D, 0xAB, 0x43, 0x87, 0x64, 0x3A, 0x22, 0xE6, 0x88, 0x94, 0xC7, 0x35, 0x6D, 0x2A, 0x9D, 0x49, 0xA2, 0x8B, 0x1A, 0x86, 0x23, 0x5E, 0xD4, 0xC7, 0xE3, 0x67, 0xD2, 0xDE, 0x9F, 0x98, 0xBC, 0xB7, 0x95, 0x4E, 0x10, 0xC7, 0xF7, 0x70, 0xB8, 0xDF, 0xC1, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  meterT->add_sensor(new Supla::Sensor::SensorBase("02719887", "apator162", "total_water_m3", "00000000000000000000000000000000"));
  uint8_t len_without_crc = crcRemove(apatorminarray, packetSize(apatorminarray[0]));
  std::vector<unsigned char> frameApatorMine(apatorminarray, apatorminarray + len_without_crc);
  float readValue = meterT->parse_frame(frameApatorMine);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 123.291f, readValue);
}

//
void test_short_frame() {
      std::vector<unsigned char> keyT{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // 3E4401068798710205074CDB7A7C0030851667FDF92E43FFE2E3AAD33649145A28BB7FE73DD1C405A7971F6E69B5E05227346439736D131303F537F06417593E4FC3C142D51716C5730000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
  uint8_t apatorminarray2[254] = {0x3E, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07, 0x4C, 0xDB, 0x7A, 0x7C, 0x00, 0x30, 0x85, 0x16, 0x67, 0xFD, 0xF9, 0x2E,
                                  0x43, 0xFF, 0xE2, 0xE3, 0xAA, 0xD3, 0x36, 0x49, 0x14, 0x5A, 0x28, 0xBB, 0x7F, 0xE7, 0x3D, 0xD1, 0xC4,
                                  0x05, 0xA7, 0x97, 0x1F, 0x6E, 0x69, 0xB5, 0xE0, 0x52, 0x27, 0x34, 0x64, 0x39, 0x73, 0x6D, 0x13,
                                  };

  auto meterA = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
  meterA->add_driver(new Apator162());
  meterA->add_sensor(new Supla::Sensor::SensorBase("02719887", "apator162", "total_water_m3","00000000000000000000000000000000"));
  auto len_without_crc = crcRemove(apatorminarray2, packetSize(apatorminarray2[0]));
  std::vector<unsigned char> frameApatorShort(apatorminarray2, apatorminarray2 + len_without_crc);
  auto readValue = meterA->parse_frame(frameApatorShort);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 246.328f, readValue);
}

void test_wmbus_meter_parse_all_values_with_crc() {
  uint8_t* frame_ptr = build_apator162_frame_with_crc();

  auto wmbus_meter1 = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);  // debugMode=true to skip actual hardware init
  auto wmbus_driver1 = new Apator162();
  wmbus_meter1->add_driver(wmbus_driver1);

  // Meter ID: 02719887
  auto wmbus_sensor1 = new Supla::Sensor::SensorBase("02719887", "apator162", "total_water_m3", "00000000000000000000000000000000");
  wmbus_meter1->add_sensor(wmbus_sensor1);

  uint8_t len_without_crc = crcRemove(frame_ptr, packetSize(frame_ptr[0]));
  std::vector<unsigned char> frame(frame_ptr, frame_ptr + len_without_crc);

  // Use parse_frame method which handles decryption and value extraction
  float result = wmbus_meter1->parse_frame(frame);

  // parse_frame returns the value for the configured property (total_water_m3)
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 242.956f, result);
}

// ---------------------------------------------------------------------------
// Synthetic frames for battery-voltage register (0x41) tests
//
// Frame layout (35 bytes, already decrypted, no CRC):
//   [0..14]  : standard DLL + TPL short header  (meter 02719887, APA, CI=0x7A)
//   [15..16] : 0x2F 0x2F  decryption marker
//   [17]     : 0x0F  DIF (manufacturer specific)
//   [18..20] : reg 0x41  voltage  = 0x0E10 = 3600 mV → 3.6 V
//   [21..22] : reg 0xA2  (1-byte padding)
//   [23..24] : reg 0xA5  (1-byte padding)
//   [25..29] : reg 0x10  total water = 0x89 = 137 → 0.137 m³
//   [30..34] : 0xFF  end padding
// ---------------------------------------------------------------------------
static const uint8_t apator162_battery_frame[] = {
    0x22, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07, // DLL header
    0x7A, 0x22, 0x00, 0x30, 0x85,                                 // TPL short header
    0x2F, 0x2F,                                                    // decryption marker
    0x0F,                                                          // DIF: manufacturer specific
    0x41, 0x10, 0x0E,                                             // reg 0x41: 0x0E10=3600 mV → 3.6 V
    0xA2, 0x00,                                                    // reg 0xA2: 1-byte filler
    0xA5, 0x00,                                                    // reg 0xA5: 1-byte filler
    0x10, 0x89, 0x00, 0x00, 0x00,                                 // reg 0x10: 137 → 0.137 m³
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF                                   // end padding
};

// Same layout but 0x41 replaced by 0x43 (days) — no voltage register.
static const uint8_t apator162_no_battery_frame[] = {
    0x22, 0x44, 0x01, 0x06, 0x87, 0x98, 0x71, 0x02, 0x05, 0x07,
    0x7A, 0x22, 0x00, 0x30, 0x85,
    0x2F, 0x2F,
    0x0F,
    0x43, 0x3E, 0x05,  // reg 0x43: 1342 days  (no 0x41)
    0xA5, 0x00,
    0xA5, 0x00,
    0x10, 0x89, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

void test_battery_voltage_register_found() {
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_battery_frame,
                                     apator162_battery_frame + sizeof(apator162_battery_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_TRUE_MESSAGE(vals.count("battery_voltage_v") > 0,
                             "battery_voltage_v key missing from map");
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.6f, vals["battery_voltage_v"]);
}

void test_battery_voltage_total_water_from_battery_frame() {
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_battery_frame,
                                     apator162_battery_frame + sizeof(apator162_battery_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.137f, vals["total_water_m3"]);
}

void test_battery_voltage_not_found_absent_from_map() {
    // When register 0x41 is absent the key must not appear in the map.
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_no_battery_frame,
                                     apator162_no_battery_frame + sizeof(apator162_no_battery_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_EQUAL_UINT(0u, (unsigned)vals.count("battery_voltage_v"));
}

// ---------------------------------------------------------------------------
// Real decrypted frames from hardware logs (meter 00489912, June 2026).
// All frames already decrypted — pass directly to drv.get_values().
//
// Timeline (2026-06-18):
//   21:30:14  BG_06    — no active tamper, 0x80 present with count=98, byte[24]=0x00
//   21:40:19  HELD_07  — magnet HELD, byte[24]=0x01, 0x80 ABSENT (only 0x81 rotates)
//   22:03:40  HELD_08  — magnet still HELD, byte[24]=0x01, 0x80 ABSENT (only 0x82)
//
// Key confirmed behaviours:
//   - byte[24]=0x01 reliably indicates active tamper
//   - Register 0x80 DISAPPEARS from frame while tamper is active
//   - evt[6] inside 0x80 is a TIME byte, NOT a tamper-type bitmap
//     (0x08 observed day-after confirmed magnetic+cover tampers, not 0x02/0x03)
// ---------------------------------------------------------------------------

// clang-format off
// Real frame BG_06 (21:30:14): no active tamper, 0x80 present, tamper_count=98, byte[24]=0x00
static const uint8_t apator162_tamper_background_frame[] = {
    0x3E,0x44,0x01,0x06,0x12,0x99,0x48,0x00,0x05,0x07,0x7A,0x06,0x00,0x30,0x85,
    0x2F,0x2F,
    0x0F,0x5D,0xB2,0x96,0x9A,0x07,0x02,0x00,  // byte[24]=0x00 — no active tamper
    0x43,0x37,0x0E,
    0x80,0x62,0x91,0x56,0x18,0xB2,0x56,0x08,0xF6,0x01,0x00,  // 0x80 present, evt[0]=98
    0x10,0x8E,0x3A,0x09,0x00,
    0x71,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xA0,0x31,0x57,0xFE,0x03,
    0xFF,0xFF,0xAA,0xD8
};

// Real frame HELD_07 (21:40:19): magnet HELD, byte[24]=0x01, 0x80 ABSENT
static const uint8_t apator162_tamper_active_frame[] = {
    0x3E,0x44,0x01,0x06,0x12,0x99,0x48,0x00,0x05,0x07,0x7A,0x07,0x00,0x30,0x85,
    0x2F,0x2F,
    0x0F,0x67,0xB2,0x96,0x9A,0x07,0x02,0x01,  // byte[24]=0x01 — ACTIVE TAMPER
    0x43,0x37,0x0E,
    0x81,0x20,0x70,0x56,0xCD,0x70,0x56,0x0F,0x66,0x00,0x00,  // only 0x81 — 0x80 absent
    0x10,0x8E,0x3A,0x09,0x00,
    0x71,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xA0,0x31,0x57,0xFE,0x03,
    0xFF,0xFF,0x79,0x5D
};

// Real frame CVR_HELD_05 (21:08:40 June 18): cover HELD REMOVED, byte[24]=0x04.
// byte[24] bit2 (0x04) = cover_tamper_active; 0x80 and 0x81 absent from rotation.
static const uint8_t apator162_cover_removed_frame[] = {
    0x3E,0x44,0x01,0x06,0x12,0x99,0x48,0x00,0x05,0x07,0x7A,0x05,0x00,0x30,0x85,
    0x2F,0x2F,
    0x0F,0x48,0xB2,0x96,0x9A,0x07,0x02,0x04,  // byte[24]=0x04 — COVER TAMPER ACTIVE
    0x43,0x37,0x0E,
    0x82,0x1F,0x70,0x56,0x48,0xB2,0x56,0x0D,0xAC,0x00,0x00,  // 0x80+0x81 absent; only 0x82
    0x10,0x8B,0x3A,0x09,0x00,
    0x71,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xA0,0x31,0x57,0xFE,0x03,
    0xFF,0xFF,0x6F,0x30
};

// Real frame during illumination test (AccNum=0xF2): only 0x81/0x82 in rotation, no alarm registers
static const uint8_t apator162_illumination_frame[] = {
    0x3E,0x44,0x01,0x06,0x12,0x99,0x48,0x00,0x05,0x07,0x7A,0xF2,0x00,0x30,0x85,
    0x2F,0x2F,
    0x0F,0x59,0x91,0x76,0x9A,0x06,0x02,0x00,
    0x43,0x36,0x0E,
    0x81,0x20,0x70,0x56,0xCD,0x70,0x56,0x0F,0x66,0x00,0x00,
    0x10,0x45,0x38,0x09,0x00,
    0x71,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xA0,0x31,0x57,0xFE,0x03,
    0xFF,0xFF,0x3A,0xD4
};

// clang-format on

void test_tamper_count_in_background_frame() {
    // Real June 18 background frame (21:30): no active tamper, 0x80 present.
    // tamper_count=98, tamper_active=0.
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_tamper_background_frame,
                                     apator162_tamper_background_frame + sizeof(apator162_tamper_background_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 98.0f, vals["tamper_count"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f,  vals["tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f,  vals["magnetic_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f,  vals["cover_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 604.814f, vals["total_water_m3"]);
}

void test_tamper_active_flag_set_while_magnet_held() {
    // Real June 18 frame (21:40): magnet held, byte[24]=0x01, 0x80 ABSENT.
    // tamper_active=1.0, magnetic_tamper_active=1.0, cover_tamper_active=0.0.
    // tamper_count=0.0 (0x80 register absent while active).
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_tamper_active_frame,
                                     apator162_tamper_active_frame + sizeof(apator162_tamper_active_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, vals["tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, vals["magnetic_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["cover_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["tamper_count"]); // 0x80 absent during active tamper
}

void test_cover_tamper_active_flag_set_while_cover_removed() {
    // Real June 18 frame (21:08): cover held removed, byte[24]=0x04.
    // cover_tamper_active=1.0, magnetic_tamper_active=0.0, tamper_active=1.0.
    // tamper_count=0.0 (0x80 absent while cover is actively removed).
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_cover_removed_frame,
                                     apator162_cover_removed_frame + sizeof(apator162_cover_removed_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, vals["cover_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["magnetic_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, vals["tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["tamper_count"]); // 0x80 absent during active cover tamper
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 604.811f, vals["total_water_m3"]);
}

void test_no_alarm_registers_during_illumination() {
    // Real illumination frame: no alarm registers present (illumination test had no effect).
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_illumination_frame,
                                     apator162_illumination_frame + sizeof(apator162_illumination_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["tamper_count"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["magnetic_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["cover_tamper_active"]);
}

void test_backflow_register_found_with_count() {}  // REMOVED — 0x83 absent in 3-block frames
void test_backflow_volume_still_parsed() {}

void test_alarm_registers_absent_when_no_tamper() {
    // Battery frame: byte[24]=0x00, no 0x80 register — tamper fields are 0.
    Apator162 drv;
    std::vector<unsigned char> frame(apator162_battery_frame,
                                     apator162_battery_frame + sizeof(apator162_battery_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["tamper_count"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["magnetic_tamper_active"]);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, vals["cover_tamper_active"]);
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

#include <Arduino.h>
void setup() {
  Serial.begin(115200);
  delay(2000);

  UNITY_BEGIN();

  // --- Group 1: decrypt and parse telegram ---
  RUN_TEST(test_short_frame);

  RUN_TEST(text_crc_removal);

  RUN_TEST(test_battery_voltage_register_found);
  RUN_TEST(test_battery_voltage_total_water_from_battery_frame);
  RUN_TEST(test_battery_voltage_not_found_absent_from_map);

  // --- Alarm / tamper event registers (real hardware frames, meter 00489912, June 2026) ---
  RUN_TEST(test_tamper_count_in_background_frame);
  RUN_TEST(test_tamper_active_flag_set_while_magnet_held);
  RUN_TEST(test_cover_tamper_active_flag_set_while_cover_removed);
  RUN_TEST(test_no_alarm_registers_during_illumination);
  RUN_TEST(test_alarm_registers_absent_when_no_tamper);

  /*

      RUN_TEST(test_full_frame_crc_strip_length);
      RUN_TEST(test_full_frame_ci_field_after_strip);

      RUN_TEST(test_full_frame_meter_id_number_string);
      RUN_TEST(test_full_frame_driver_name);
      RUN_TEST(test_full_frame_device_type_byte);




  */
  UNITY_END();
}
void loop() {
}
