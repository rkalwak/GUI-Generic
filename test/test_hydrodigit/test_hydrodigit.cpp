/*
 * Unit tests for Hydrodigit wM-Bus driver.
 *
 * Uses a real raw Hydrodigit telegram and decryption key with battery
 * voltage, backflow, and monthly history values.
 */

#include <unity.h>
#include "../../lib/wmbus/src/Drivers/driver_hydrodigit.h"
#include "../../lib/wmbus/src/crc.hpp"
#include "../../lib/wmbus/src/utils.hpp"
#include "../../lib/wmbus/src/SensorBase.h"

#include <cstdlib>
#include <string>

#define SUPLA_CC1101
#ifdef SUPLA_CC1101
#include "../../src/src/sensor/WmbusMeter.h"
#endif
#include <mbus_packet.hpp>

static const char hydrodigit_raw_frame_hex[] =
    "4e44B4091787662515077a620040055ea5505c0556a3Bdf70B10581247Bf1c9a"
    "2aB74827e099B91d6909B16B96eae14868e7f8e3d9d1f80ef3c9a255e2596914c9"
    "132B60B6a80f65Bc71B25dc8ddd3";

static const char hydrodigit_key_hex[] = "00000000000000000000000000000000";

static std::vector<unsigned char> hex_to_vec(const char *hex) {
    std::vector<unsigned char> data;
    std::string value(hex);

    for (size_t index = 0; index + 1 < value.size(); index += 2) {
        std::string byte_string = value.substr(index, 2);
        data.push_back(static_cast<unsigned char>(std::strtoul(byte_string.c_str(), nullptr, 16)));
    }

    return data;
}

static std::vector<unsigned char> decrypt_hydrodigit_frame() {
    auto frame = hex_to_vec(hydrodigit_raw_frame_hex);
    auto key = hex_to_vec(hydrodigit_key_hex);
    Supla::Sensor::WmbusMeter meter(1, 2, 3, 4, 6, 7, true);

    TEST_ASSERT_TRUE_MESSAGE(meter.decrypt_telegram(frame, key), "hydrodigit raw frame decryption failed");

    return frame;
}

void test_hydrodigit_driver_name() {
    Hydrodigit drv;
    TEST_ASSERT_EQUAL_STRING("hydrodigit", drv.get_name().c_str());
}

void test_hydrodigit_total_water_m3() {
    Hydrodigit drv;
    auto frame = decrypt_hydrodigit_frame();
    auto vals = drv.get_values(frame);
    TEST_ASSERT_TRUE_MESSAGE(vals.count("total_water_m3") > 0, "total_water_m3 missing");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.43f, vals["total_water_m3"]);
}

void test_hydrodigit_manufacturer_fields() {
    Hydrodigit drv;
    auto frame = decrypt_hydrodigit_frame();
    auto vals = drv.get_values(frame);

    TEST_ASSERT_TRUE_MESSAGE(vals.count("battery_voltage_v") > 0, "battery_voltage_v missing");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("backflow_m3") > 0, "backflow_m3 missing");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("January_total_m3") > 0, "January_total_m3 missing");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("April_total_m3") > 0, "April_total_m3 missing");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("October_total_m3") > 0, "October_total_m3 missing");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("December_total_m3") > 0, "December_total_m3 missing");

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.2f, vals["battery_voltage_v"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["backflow_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.34f, vals["January_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["April_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.34f, vals["October_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.34f, vals["December_total_m3"]);
}

void test_wmbusimeter_hydrodigit_parse_total_water() {
    auto m = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
    m->add_driver(new Hydrodigit());
    m->add_sensor(new Supla::Sensor::SensorBase(
        "25668717", "hydrodigit", "total_water_m3", hydrodigit_key_hex));
    auto frame = hex_to_vec(hydrodigit_raw_frame_hex);
    float result = m->parse_frame(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.43f, result);
    delete m;
}

void test_wmbusimeter_hydrodigit_backflow_property() {
    auto m = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
    m->add_driver(new Hydrodigit());
    m->add_sensor(new Supla::Sensor::SensorBase(
        "25668717", "hydrodigit", "backflow_m3", hydrodigit_key_hex));
    auto frame = hex_to_vec(hydrodigit_raw_frame_hex);
    float result = m->parse_frame(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
    delete m;
}

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_hydrodigit_driver_name);
    RUN_TEST(test_hydrodigit_total_water_m3);
    RUN_TEST(test_hydrodigit_manufacturer_fields);
    RUN_TEST(test_wmbusimeter_hydrodigit_parse_total_water);
    RUN_TEST(test_wmbusimeter_hydrodigit_backflow_property);

    UNITY_END();
}

void loop() {}