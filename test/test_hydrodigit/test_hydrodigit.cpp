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

static const char hydrodigit_0c13_raw_frame_hex[] =
    "4e44B4096449612615077a92004005c5e28279d0d06c4ea596Bf6ca43f006227"
    "a84932592B71992B52f0429943e5a5ea220dddff0B954d44Bf377909Ba46a6416f"
    "c525637a8663e3a3ae2dad0ef947";

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

static std::vector<unsigned char> decrypt_hydrodigit_0c13_frame() {
    auto frame = hex_to_vec(hydrodigit_0c13_raw_frame_hex);
    auto key = hex_to_vec(hydrodigit_key_hex);
    Supla::Sensor::WmbusMeter meter(1, 2, 3, 4, 6, 7, true);

    TEST_ASSERT_TRUE_MESSAGE(meter.decrypt_telegram(frame, key), "hydrodigit 0C13 raw frame decryption failed");

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
    TEST_ASSERT_TRUE_MESSAGE(vals.count("total_m3") > 0, "total_m3 alias missing");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.43f, vals["total_water_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.43f, vals["total_m3"]);
}

void test_hydrodigit_total_water_m3_from_0c13() {
    Hydrodigit drv;
    auto frame = decrypt_hydrodigit_0c13_frame();
    auto vals = drv.get_values(frame);
    TEST_ASSERT_TRUE_MESSAGE(vals.count("total_water_m3") > 0, "total_water_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("total_m3") > 0, "total_m3 alias missing for 0C13 telegram");
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.008f, vals["total_water_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.008f, vals["total_m3"]);
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

void test_hydrodigit_0c13_sample_fields() {
    Hydrodigit drv;
    auto frame = decrypt_hydrodigit_0c13_frame();
    auto vals = drv.get_values(frame);

    TEST_ASSERT_TRUE_MESSAGE(vals.count("battery_voltage_v") > 0, "battery_voltage_v missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("backflow_m3") > 0, "backflow_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("January_total_m3") > 0, "January_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("February_total_m3") > 0, "February_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("March_total_m3") > 0, "March_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("April_total_m3") > 0, "April_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("May_total_m3") > 0, "May_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("June_total_m3") > 0, "June_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("July_total_m3") > 0, "July_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("August_total_m3") > 0, "August_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("September_total_m3") > 0, "September_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("October_total_m3") > 0, "October_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("November_total_m3") > 0, "November_total_m3 missing for 0C13 telegram");
    TEST_ASSERT_TRUE_MESSAGE(vals.count("December_total_m3") > 0, "December_total_m3 missing for 0C13 telegram");

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.2f, vals["battery_voltage_v"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["backflow_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["January_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["February_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["March_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["April_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["May_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["June_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["July_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["August_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["September_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["October_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["November_total_m3"]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, vals["December_total_m3"]);
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

void test_wmbusimeter_hydrodigit_parse_total_water_from_0c13() {
    auto m = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
    m->add_driver(new Hydrodigit());
    m->add_sensor(new Supla::Sensor::SensorBase(
        "26614964", "hydrodigit", "total_water_m3", hydrodigit_key_hex));
    auto frame = hex_to_vec(hydrodigit_0c13_raw_frame_hex);
    float result = m->parse_frame(frame);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.008f, result);
    delete m;
}

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_hydrodigit_driver_name);
    RUN_TEST(test_hydrodigit_total_water_m3);
    RUN_TEST(test_hydrodigit_total_water_m3_from_0c13);
    RUN_TEST(test_hydrodigit_manufacturer_fields);
    RUN_TEST(test_hydrodigit_0c13_sample_fields);
    RUN_TEST(test_wmbusimeter_hydrodigit_parse_total_water);
    RUN_TEST(test_wmbusimeter_hydrodigit_backflow_property);
    RUN_TEST(test_wmbusimeter_hydrodigit_parse_total_water_from_0c13);

    UNITY_END();
}

void loop() {}