/*
 * Unit tests for Bmeters wM-Bus driver.
 *
 * Covers BMETERS IWM-TX3 (uses DIF/VIF 0C 13, BCD volume).
 *
 * Synthetic frames (decrypted, CRC-stripped) are constructed directly so
 * that the tests run natively without real hardware.
 *
 * Frame layout used in the synthetic frames (all 18 bytes):
 *   [0]    L-field
 *   [1]    C-field = 0x44 (SND_NR)
 *   [2-3]  M-field (manufacturer LE)
 *   [4-7]  Device ID (LE)
 *   [8]    Version
 *   [9]    Type = 0x07 (water meter)
 *   [10]   CI = 0x7A (TPL short header, frame already decrypted)
 *   [11-12] any two TPL bytes (access nr, status – ignored by driver)
 *   [13-14] DIF/VIF pair at start of application data
 *   [15-18] value bytes (4 bytes)
 *
 * For 0C13 (BCD, 4 bytes) the driver call bcd_2_int(bytes, i, 4) / 1000.
 *   Value 12.345 m³ → BCD bytes: 45 23 01 00
 * For 0413 (binary 32-bit LE) the driver reads raw uint32_t / 1000.
 *   Value 12.345 m³ → raw 12345 = 0x3039 → bytes: 39 30 00 00
 */

#include <unity.h>
#include "../../lib/wmbus/src/Drivers/driver_bmeters.h"
#include "../../lib/wmbus/src/crc.hpp"
#include "../../lib/wmbus/src/utils.hpp"
#include "../../lib/wmbus/src/SensorBase.h"

#define SUPLA_CC1101
#ifdef SUPLA_CC1101
#include "../../src/src/sensor/WmbusMeter.h"
#endif
#include <mbus_packet.hpp>

// ---------------------------------------------------------------------------
// BMETERS manufacturer bytes (LE): "BME" → MANFCODE = 2469 = 0x09A5
// ---------------------------------------------------------------------------
#define BMETERS_M_LO 0xA5
#define BMETERS_M_HI 0x09

// ---------------------------------------------------------------------------
// Synthetic TX3 frame: 0C 13 (BCD volume), value = 12.345 m³
// BCD bytes LE: 0x45, 0x23, 0x01, 0x00  → bcd_2_int = 12345 → /1000 = 12.345
// ---------------------------------------------------------------------------
static const uint8_t tx3_frame[] = {
    0x11,               // L-field (17 bytes follow)
    0x44,               // C-field
    BMETERS_M_LO, BMETERS_M_HI,  // M-field
    0x01, 0x02, 0x03, 0x04,       // device ID
    0x0A,               // version
    0x07,               // type: water meter
    0x7A,               // CI: TPL short header
    0x00, 0x00,         // TPL acc, status (ignored by driver)
    0x0C, 0x13,         // DIF=0x0C (8-digit BCD), VIF=0x13 (Volume m³ × 10⁻³)
    0x45, 0x23, 0x01, 0x00  // BCD 00012345 → 12.345 m³
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::vector<unsigned char> to_vec(const uint8_t* data, size_t len) {
    return std::vector<unsigned char>(data, data + len);
}

// ---------------------------------------------------------------------------
// Bmeters driver tests
// ---------------------------------------------------------------------------

void test_bmeters_driver_name() {
    Bmeters drv;
    TEST_ASSERT_EQUAL_STRING("bmeters", drv.get_name().c_str());
}

void test_bmeters_tx3_total_water_m3() {
    // IWM-TX3: 0C13 BCD → 12.345 m³
    Bmeters drv;
    auto frame = to_vec(tx3_frame, sizeof(tx3_frame));
    auto vals = drv.get_values(frame);
    TEST_ASSERT_TRUE_MESSAGE(vals.count("total_water_m3") > 0, "total_water_m3 missing");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.345f, vals["total_water_m3"]);
}

void test_wmbusimeter_bmeters_parse_tx3() {
    auto m = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
    m->add_driver(new Bmeters());
    // Meter ID from TX3 frame bytes[4..7]: 01 02 03 04 → pair-swap → "04030201"
    m->add_sensor(new Supla::Sensor::SensorBase(
        "04030201", "bmeters", "total_water_m3", ""));
    auto frame = to_vec(tx3_frame, sizeof(tx3_frame));
    float result = m->parse_frame(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.345f, result);
    delete m;
}

// ---------------------------------------------------------------------------
// Arduino entry points required by PlatformIO's Unity runner
// ---------------------------------------------------------------------------
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_bmeters_driver_name);
    RUN_TEST(test_bmeters_tx3_total_water_m3);
    RUN_TEST(test_wmbusimeter_bmeters_parse_tx3);

    UNITY_END();
}

void loop() {}
