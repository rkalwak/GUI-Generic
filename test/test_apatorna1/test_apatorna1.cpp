/*
 * Unit tests for ApatorNA1 (APT-WMBUS-NA-1) wM-Bus driver.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * Reference telegram (wmbusmeters test vector, already without CRCs):
 *   1C440106813591041407A0B000266A705474DDB80D9A0EB9AE2EF29D96
 *   Meter ID : 04913581   Key : 00000000000000000000000000000000
 *   Expected : total_water_m3 = 345.312
 *
 * User telegram (raw from CC1101, WITH CRCs, 35 bytes):
 *   1C44010672598007140732D4A0CC05FB6A7D2A1F7A511ED73A8043A36A2F7710D92301
 *   Meter ID : 07805972   Key : 00000000000000000000000000000000
 *   Expected : total_water_m3 = 1.861
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * Telegram structure (after CRC removal, 29 bytes):
 *   [0]     Length field (0x1C = 28)
 *   [1]     C-field      (0x44 = SND_NR)
 *   [2-3]   M-field      (0x0601 = APA / Apator)
 *   [4-7]   A-field      (meter address in BCD LE)
 *   [8]     Version      (0x14)
 *   [9]     Device type  (0x07 = water meter)
 *   [10]    CI field     (0xA0 or 0xA1 = manufacturer specific)
 *   [11]    Access counter (used as AES IV byte repeated ×8)
 *   [12]    Flags        (plaintext, ignored by driver)
 *   [13-28] AES-128-CBC encrypted payload (16 bytes)
 *
 * After decryption the 16-byte block contains:
 *   plain[0] = telegram[13]  (marker, typically 0x2E)
 *   plain[1] = telegram[14]  bits[5:4]=multiplier-exponent, bits[3:0]=reading low nibble
 *   plain[2] = telegram[15]  reading bits[11:4]
 *   plain[3] = telegram[16]  reading bits[19:12]
 *   plain[4] = telegram[17]  reading bits[27:20]
 *
 * volume_m3 = (plain[4]<<20 | plain[3]<<12 | plain[2]<<4 | plain[1]&0xF)
 *             * 10^((plain[1]&0x30)>>4) / 1000
 */

#include <unity.h>
#include "../../lib/wmbus/src/Drivers/driver_apatorna1.h"
#include "../../lib/wmbus/src/crc.hpp"
#include "../../lib/wmbus/src/utils.hpp"
#include "../../lib/wmbus/src/SensorBase.h"

#define SUPLA_CC1101
#ifdef SUPLA_CC1101
#include "../../src/src/sensor/WmbusMeter.h"
#endif
#include <mbus_packet.hpp>

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static Supla::Sensor::WmbusMeter *g_meter   = nullptr;
static ApatorNA1                 *g_driver  = nullptr;
static Supla::Sensor::SensorBase *g_sensor  = nullptr;

void setUp(void) {
    g_meter  = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, /*debugMode=*/true);
    g_driver = new ApatorNA1();
    g_meter->add_driver(g_driver);
}

void tearDown(void) {
    delete g_meter;
    g_meter  = nullptr;
    g_driver = nullptr;
    g_sensor = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic driver tests
// ─────────────────────────────────────────────────────────────────────────────

void test_driver_name() {
    TEST_ASSERT_EQUAL_STRING("apatorna1", g_driver->get_name().c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Reference telegram test (no-CRC, from wmbusmeters test vector)
//   1C440106813591041407A0B000266A705474DDB80D9A0EB9AE2EF29D96
//   Expected: 345.312 m³
// ─────────────────────────────────────────────────────────────────────────────
void test_reference_telegram_no_crc() {
    // 29 bytes, already without CRCs
    static uint8_t raw[] = {
        0x1C, 0x44, 0x01, 0x06, 0x81, 0x35, 0x91, 0x04, 0x14, 0x07,
        0xA0, 0xB0, 0x00,
        0x26, 0x6A, 0x70, 0x54, 0x74, 0xDD, 0xB8, 0x0D,
        0x9A, 0x0E, 0xB9, 0xAE, 0x2E, 0xF2, 0x9D, 0x96
    };

    g_sensor = new Supla::Sensor::SensorBase(
        "04913581", "apatorna1", "total_water_m3",
        "00000000000000000000000000000000");
    g_meter->add_sensor(g_sensor);

    std::vector<unsigned char> frame(raw, raw + sizeof(raw));
    float result = g_meter->parse_frame(frame);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 345.312f, result);
}

// ─────────────────────────────────────────────────────────────────────────────
// User telegram test (raw with CRCs, 35 bytes)
//   1C44010672598007140732D4A0CC05FB6A7D2A1F7A511ED73A8043A36A2F7710D92301
//   Meter ID : 07805972
//   Expected : 1.861 m³
// ─────────────────────────────────────────────────────────────────────────────
void test_user_telegram_with_crc() {
    // 35 bytes = packetSize(0x1C); CRCs at positions 10-11, 28-29, 33-34.
    static uint8_t raw[254] = {
        0x1C, 0x44, 0x01, 0x06, 0x72, 0x59, 0x80, 0x07, 0x14, 0x07,
        0x32, 0xD4,                                    // CRC block 1
        0xA0, 0xCC, 0x05, 0xFB, 0x6A, 0x7D, 0x2A, 0x1F,
        0x7A, 0x51, 0x1E, 0xD7, 0x3A, 0x80, 0x43, 0xA3,
        0x6A, 0x2F,                                    // CRC block 2
        0x77, 0x10, 0xD9,
        0x23, 0x01                                     // CRC block 3
    };

    g_sensor = new Supla::Sensor::SensorBase(
        "07805972", "apatorna1", "total_water_m3",
        "00000000000000000000000000000000");
    g_meter->add_sensor(g_sensor);

    uint8_t len_no_crc = crcRemove(raw, packetSize(raw[0]));
    std::vector<unsigned char> frame(raw, raw + len_no_crc);

    // After CRC removal the telegram should be 29 bytes.
    TEST_ASSERT_EQUAL(29u, (unsigned)frame.size());

    // Byte 10 must be CI = 0xA0 (after stripping the two CRC bytes at [10-11]).
    TEST_ASSERT_EQUAL_HEX8(0xA0, frame[10]);

    // Meter ID extraction: bytes 4-7 pair-swapped → "07805972".
    std::string meter_id = g_meter->extract_meter_id_string(frame);
    TEST_ASSERT_EQUAL_STRING("07805972", meter_id.c_str());

    float result = g_meter->parse_frame(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.861f, result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test that an unsupported CI field is rejected gracefully.
// ─────────────────────────────────────────────────────────────────────────────
void test_wrong_ci_field_returns_zero() {
    // Swap byte 10 to 0xB0 (unsupported CI) – after CRC removal.
    static uint8_t raw[] = {
        0x1C, 0x44, 0x01, 0x06, 0x81, 0x35, 0x91, 0x04, 0x14, 0x07,
        0xB0, 0xB0, 0x00,   // CI = 0xB0 (unknown)
        0x26, 0x6A, 0x70, 0x54, 0x74, 0xDD, 0xB8, 0x0D,
        0x9A, 0x0E, 0xB9, 0xAE, 0x2E, 0xF2, 0x9D, 0x96
    };

    g_sensor = new Supla::Sensor::SensorBase(
        "04913581", "apatorna1", "total_water_m3",
        "00000000000000000000000000000000");
    g_meter->add_sensor(g_sensor);

    std::vector<unsigned char> frame(raw, raw + sizeof(raw));
    float result = g_meter->parse_frame(frame);

    // Unknown CI → decrypt_telegram returns false → parse_frame returns 0.
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test runner
// ─────────────────────────────────────────────────────────────────────────────
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_driver_name);
    RUN_TEST(test_reference_telegram_no_crc);
    RUN_TEST(test_user_telegram_with_crc);
    RUN_TEST(test_wrong_ci_field_returns_zero);

    UNITY_END();
}

void loop() {}
