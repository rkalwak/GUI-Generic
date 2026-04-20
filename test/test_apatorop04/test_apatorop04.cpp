/*
 * Unit tests for Apator OP-04-1A wM-Bus driver (apatorop04).
 *
 * Reference telegram analysis:
 *   Encoded (with CRCs):
 *     81440106694712101a078cc0e47ad1707085c217175b9b8cfb54144c2a6efc1f83096af97753cead3d
 *     484d1b202afa072bf4d5afdc37483f6279e15c0a19af1a36599567b0b2b41d0dab0e682e8faa48dc1
 *     232957bb71caf5b31c39e5ebe8f64c0b15614279ed78a4e74f0b4cbf44f70e0fc7395d35bdbc494c3
 *     bfb77c5f48eaec6c
 *
 *   Decoded (CRC-removed + AES-decrypted, 130 bytes):
 *     81440106694712101A078CC0E47AD17070852F2F
 *     041389000000046D0EAE54340F60200200000858
 *     049241FF3BB75F3301000000010000000100000001
 *     00000001000000010000000000000000000000000000
 *     0000000000000000000000000000000000000000000000
 *     0000000000000000000049C350542D54540900002F2F2F2F2F2F2F2F
 *
 * Frame structure (after CRC removal, 130 bytes total):
 *   Bytes  0-9  : DLL header (L=0x81, C=0x44, M=0x0106 [APA], A[4]=69471210,
 *                             version=0x1A, type=0x07)
 *   Bytes 10-12 : ELL – Extended Link Layer I (CI=0x8C, CC=0xC0, ACC=0xE4)
 *   Byte   13   : TPL CI = 0x7A (short header, AES_CBC_IV)
 *   Byte   14   : TPL access number = 0xD1
 *   Byte   15   : TPL status = 0x70
 *   Bytes 16-17 : TPL config = 0x70 0x85  (bidirectional, nb=7 blocks)
 *   Bytes 18-19 : 0x2F 0x2F  (decryption-succeeded marker)
 *   Byte   20+  : Application data
 *                   04 13 89 00 00 00  → total_water_m3 = 0.137 m³
 *                   04 6D 0E AE 54 34  → meter_datetime = 2026-04-20 14:14
 *                   0F ...             → manufacturer-specific
 *
 * Meter ID : 10124769  (bytes 4-7 = 69 47 12 10, pair-swapped)
 * AES key  : ""  (empty – telegram is pre-decrypted, decryption step skipped)
 *
 * Expected decoded values:
 *   total_water_m3: 0.137 m³
 */

#include <unity.h>
#include "../../lib/wmbus/src/Drivers/driver_apatorop04.h"
#include "../../lib/wmbus/src/crc.hpp"
#include "../../lib/wmbus/src/utils.hpp"
#include "../../lib/wmbus/src/SensorBase.h"

#define SUPLA_CC1101
#ifdef SUPLA_CC1101
#include "../../src/src/sensor/WmbusMeter.h"
#endif
#include <mbus_packet.hpp>

// ---------------------------------------------------------------------------
// Reference decoded frame (CRC-stripped + AES-decrypted), 130 bytes.
// Byte[10] = 0x8C  ELL CI
// Byte[13] = 0x7A  TPL CI (short header)
// Bytes[18-19] = 0x2F 0x2F  already-decrypted marker
// Byte[20+] = application data: 04 13 89 00 00 00 …
// Source: https://wmbusmeters.org/analyze/<decoded_hex>
// ---------------------------------------------------------------------------
static const uint8_t decoded_frame[] = {
    0x81, 0x44, 0x01, 0x06, 0x69, 0x47, 0x12, 0x10, 0x1A, 0x07, // DLL header
    0x8C, 0xC0, 0xE4,                                             // ELL (CI, CC, ACC)
    0x7A, 0xD1, 0x70, 0x70, 0x85, 0x2F, 0x2F,                   // TPL short header + check bytes
    // Application data (byte 20 onwards):
    0x04, 0x13, 0x89, 0x00, 0x00, 0x00,                          // total_water_m3 = 0.137
    0x04, 0x6D, 0x0E, 0xAE, 0x54, 0x34,                          // meter_datetime
    0x0F, 0x60, 0x20, 0x02, 0x00, 0x00, 0x08, 0x58,             // manufacturer-specific
    0x04, 0x92, 0x41, 0xFF, 0x3B, 0xB7, 0x5F, 0x33, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x49, 0xC3, 0x50, 0x54, 0x2D, 0x54, 0x54, 0x09, 0x00, 0x00,
    0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F              // trailing fill bytes
};
static const size_t decoded_frame_len = sizeof(decoded_frame); // 130 bytes

// ---------------------------------------------------------------------------
// Global test fixtures
// ---------------------------------------------------------------------------
static Supla::Sensor::WmbusMeter* wmbus_meter = nullptr;
static ApatorOP04*                wmbus_driver = nullptr;
static Supla::Sensor::SensorBase* wmbus_sensor = nullptr;

void setUp() {
    // debugMode = true → skip actual CC1101 hardware initialisation
    wmbus_meter  = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
    wmbus_driver = new ApatorOP04();
    wmbus_meter->add_driver(wmbus_driver);

    // Empty key: telegram is already decrypted, skip decrypt_telegram().
    wmbus_sensor = new Supla::Sensor::SensorBase(
        "10124769",   // meter ID (pair-swapped from bytes 4-7: 69 47 12 10)
        "apatorop04", // driver name
        "total_water_m3",   // property to read
        ""            // empty key → pre-decrypted frame, no AES step
    );
    wmbus_meter->add_sensor(wmbus_sensor);
}

void tearDown() {
    if (wmbus_meter) {
        delete wmbus_meter;
        wmbus_meter = nullptr;
    }
    // wmbus_driver and wmbus_sensor are owned by wmbus_meter
    wmbus_driver = nullptr;
    wmbus_sensor = nullptr;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_driver_name() {
    ApatorOP04 drv;
    TEST_ASSERT_EQUAL_STRING("apatorop04", drv.get_name().c_str());
}

void test_decoded_frame_length() {
    // L-field = 0x81 (129 bytes), but after CRC removal the frame is 128 bytes
    // (wMBus T-mode: 2 CRC bytes removed from the data block, excluding the L byte itself)
    TEST_ASSERT_EQUAL(128u, (unsigned)decoded_frame_len);
}

void test_decoded_frame_dll_header_bytes() {
    // DLL: C=0x44 (SND_NR from meter), M=0x0106 (APA), version=0x1A, type=0x07
    TEST_ASSERT_EQUAL_HEX8(0x44, decoded_frame[1]);   // C-field
    TEST_ASSERT_EQUAL_HEX8(0x01, decoded_frame[2]);   // M low byte
    TEST_ASSERT_EQUAL_HEX8(0x06, decoded_frame[3]);   // M high byte
    TEST_ASSERT_EQUAL_HEX8(0x1A, decoded_frame[8]);   // version
    TEST_ASSERT_EQUAL_HEX8(0x07, decoded_frame[9]);   // device type (water meter)
}

void test_decoded_frame_ell_ci_byte() {
    // Byte 10 must be ELL CI = 0x8C (Extended Link Layer I, 2 bytes)
    TEST_ASSERT_EQUAL_HEX8(0x8C, decoded_frame[10]);
}

void test_decoded_frame_tpl_ci_byte() {
    // After 3-byte ELL, byte 13 = TPL CI = 0x7A (short header, AES_CBC_IV)
    TEST_ASSERT_EQUAL_HEX8(0x7A, decoded_frame[13]);
}

void test_decoded_frame_check_bytes() {
    // Bytes 18-19 = 0x2F 0x2F  (decryption-succeeded marker)
    TEST_ASSERT_EQUAL_HEX8(0x2F, decoded_frame[18]);
    TEST_ASSERT_EQUAL_HEX8(0x2F, decoded_frame[19]);
}

void test_decoded_frame_volume_dif_vif() {
    // Application data starts at byte 20: DIF=0x04, VIF=0x13
    TEST_ASSERT_EQUAL_HEX8(0x04, decoded_frame[20]);  // DIF: 32-bit integer
    TEST_ASSERT_EQUAL_HEX8(0x13, decoded_frame[21]);  // VIF: Volume m³ × 10⁻³
}

void test_decoded_frame_volume_raw_value() {
    // Raw 32-bit LE value for total_water_m3: 89 00 00 00 = 0x89 = 137 → 0.137 m³
    uint32_t raw = ((uint32_t)decoded_frame[25] << 24) |
                   ((uint32_t)decoded_frame[24] << 16) |
                   ((uint32_t)decoded_frame[23] <<  8) |
                   ((uint32_t)decoded_frame[22]);
    TEST_ASSERT_EQUAL_UINT32(137u, raw);
}

void test_meter_id_extraction() {
    // WmbusMeter::extract_meter_id_string pair-swaps bytes 4-7:
    //   bytes = 69 47 12 10  →  pair-swap → "10" "12" "47" "69" → "10124769"
    std::vector<unsigned char> frame(decoded_frame, decoded_frame + decoded_frame_len);
    std::string id = wmbus_meter->extract_meter_id_string(frame);
    TEST_ASSERT_EQUAL_STRING("10124769", id.c_str());
}

void test_parse_frame_total_water_m3() {
    // Full integration: parse_frame with pre-decrypted frame + empty key
    // Expected: total_water_m3 = 0.137  (tolerance ±0.001 m³)
    std::vector<unsigned char> frame(decoded_frame, decoded_frame + decoded_frame_len);
    float result = wmbus_meter->parse_frame(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.137f, result);
}

// ---------------------------------------------------------------------------
// Arduino entry points required by PlatformIO's Unity runner
// ---------------------------------------------------------------------------
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_driver_name);
    RUN_TEST(test_decoded_frame_length);
    RUN_TEST(test_decoded_frame_dll_header_bytes);
    RUN_TEST(test_decoded_frame_ell_ci_byte);
    RUN_TEST(test_decoded_frame_tpl_ci_byte);
    RUN_TEST(test_decoded_frame_check_bytes);
    RUN_TEST(test_decoded_frame_volume_dif_vif);
    RUN_TEST(test_decoded_frame_volume_raw_value);
    RUN_TEST(test_meter_id_extraction);
    RUN_TEST(test_parse_frame_total_water_m3);

    UNITY_END();
}

void loop() {}
