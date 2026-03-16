/*
 * Unit tests for Sharky774 wM-Bus driver register parsing.
 *
 * FRAME INTERPRETATION LOGIC (from manufacturer_specificities.cc):
 * 
 * Sharky774 uses Diehl/DME manufacturer (0xA511) with detectDiehlFrameInterpretation():
 *   Checks: c_field, m_field, ci_field, tpl_cfg to determine frame type
 * 
 * Frame Type Detection:
 *   - ci_field=0x7A && ((tpl_cfg >> 8) & 0x10) == 0x10  -> REAL_DATA (LFSR encryption)
 *   - ci_field=0x7A && ((tpl_cfg >> 8) & 0x10) == 0x00  -> OMS (AES-128-CBC encryption)
 *   - ci_field=0x71                                      -> REAL_DATA (alarm, LFSR)
 *   - ci_field=0xA0-0xA7 (manufacturer specific)         -> PRIOS (LFSR encryption)
 * 
 * ENCRYPTION METHODS:
 * 
 * 1. OMS Frame (AES-128-CBC):
 *    - Standard M-Bus encryption with 16-byte AES key
 *    - Default keys for Diehl/PRIOS meters if no key provided:
 *      PRIOS_DEFAULT_KEY2 repeated: "51728910E66D83F851728910E66D83F8"
 *    - addDefaultManufacturerKeyIfAny() adds this automatically
 * 
 * 2. REAL_DATA Frame (LFSR - Linear Feedback Shift Register):
 *    - Custom Diehl encryption using decodeDiehlLfsr()
 *    - Works with 8-byte keys: PRIOS_DEFAULT_KEY1="39BC8A10E66D83F8"
 *                               PRIOS_DEFAULT_KEY2="51728910E66D83F8"
 *    - Key is XORed with frame header fields (mfct, address, ci)
 *    - LFSR polynomial: bits 1,2,11,31 XORed for next bit
 *    - Validation: checksum & 0xEF must match frame[14] & 0xEF
 * 
 * TEST TELEGRAM ANALYSIS:
 * 
 * RAW telegram (encrypted with AES-128-CBC):
5E44A5111828058141046DF17A4900500521BA8F123FDE8B924B3EFB3B7BD143159D913EB08FA29EE2030F73B672E1EC8F4253B98A443DF7051AF17F3AF98E3FEF86677BF5FB674534CCE08BC9BD73137EE513479F405F915848755F7CD08FDAF4153994C741E1CD78246164320000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
*
* Frame structure:
*   [1]    c_field:  0x44 (SND_NR)
*   [2-3]  m_field:  0xA511 (DME manufacturer)
*   [4-7]  address:  0x18280581 (meter ID 81052818 byte-swapped)
*   [8]    version:  0x41
*   [9]    type:     0x04 (Heat meter)
*   [10]   ci_field: 0x7A (EN 13757-3 Application Layer)
*   [13-14] tpl_cfg: 0x0550 (bit 12=0, indicates OMS/AES encryption, not LFSR)
* 
* Frame interpretation: OMS (uses AES-128-CBC, not LFSR)
* 
* Parsed telegram by wmbusmeters (after CRC removal and AES decryption):
telegram=|5E44A5111828058141047A490050052F2F_0C0E189301000C13423955000B3B0000000C2B000000000A5A23030A5E18030B268339020AA6180000C2026C5C32CC020E68890100CC021306885300DB023B910000DC022B751900002F2F2F2F2F|

Auto driver    : sharky774
Similar driver : sharky 32/41
Using driver   : sharky774 00/00
000   : 5e length (94 bytes)
001   : 44 dll-c (from meter SND_NR)
002   : a511 dll-mfct (DME)
004   : 18280581 dll-id (81052818)
008   : 41 dll-version
009   : 04 dll-type (Heat meter)
010   : 7a tpl-ci-field (EN 13757-3 Application Layer (short tplh))
011   : 49 tpl-acc-field
012   : 00 tpl-sts-field (OK)
013   : 5005 tpl-cfg 0550 (AES_CBC_IV nb=5 cntn=0 ra=0 hc=0 )
015   : 2f2f decrypt check bytes (OK)
017   : 0C dif (8 digit BCD Instantaneous value)
018   : 0E vif (Energy MJ)
019 C!: 18930100 ("total_energy_consumption_kwh":5366.111111)
023   : 0C dif (8 digit BCD Instantaneous value)
024   : 13 vif (Volume l)
025 C!: 42395500 ("total_volume_m3":553.942)
029   : 0B dif (6 digit BCD Instantaneous value)
030   : 3B vif (Volume flow l/h)
031 C!: 000000 ("volume_flow_m3h":0)
034   : 0C dif (8 digit BCD Instantaneous value)
035   : 2B vif (Power W)
036 C!: 00000000 ("power_kw":0)
040   : 0A dif (4 digit BCD Instantaneous value)
041   : 5A vif (Flow temperature 10⁻¹ °C)
042 C!: 2303 ("flow_temperature_c":32.3)
044   : 0A dif (4 digit BCD Instantaneous value)
045   : 5E vif (Return temperature 10⁻¹ °C)
046 C!: 1803 ("return_temperature_c":31.8)
048   : 0B dif (6 digit BCD Instantaneous value)
049   : 26 vif (Operating time hours)
050 C!: 833902 ("operating_time_h":23983)
053   : 0A dif (4 digit BCD Instantaneous value)
054   : A6 vif (Operating time hours)
055   : 18 combinable vif (RecordErrorCodeMeterToController)
056 C!: 0000 ("operating_time_in_error_h":0)
058   : C2 dif (16 Bit Integer/Binary Instantaneous value storagenr=1)
059   : 02 dife (subunit=0 tariff=0 storagenr=5)
060   : 6C vif (Date type G)
061 C?: 5C32
063   : CC dif (8 digit BCD Instantaneous value storagenr=1)
064   : 02 dife (subunit=0 tariff=0 storagenr=5)
065   : 0E vif (Energy MJ)
066 C?: 68890100
070   : CC dif (8 digit BCD Instantaneous value storagenr=1)
071   : 02 dife (subunit=0 tariff=0 storagenr=5)
072   : 13 vif (Volume l)
073 C?: 06885300
077   : DB dif (6 digit BCD Maximum value storagenr=1)
078   : 02 dife (subunit=0 tariff=0 storagenr=5)
079   : 3B vif (Volume flow l/h)
080 C?: 910000
083   : DC dif (8 digit BCD Maximum value storagenr=1)
084   : 02 dife (subunit=0 tariff=0 storagenr=5)
085   : 2B vif (Power W)
086 C?: 75190000
090   : 2F skip
091   : 2F skip
092   : 2F skip
093   : 2F skip
094   : 2F skip

{
    "_":"telegram",
    "media":"heat",
    "meter":"sharky774",
    "name":"",
    "id":"81052818",
    "flow_temperature_c":32.3,
    "operating_time_h":23983,
    "operating_time_in_error_h":0,
    "power_kw":0,
    "return_temperature_c":31.8,
    "total_energy_consumption_kwh":5366.111111,
    "total_volume_m3":553.942,
    "volume_flow_m3h":0,
    "timestamp":"2026-03-16T19:19:52Z"
}
 */

#include <unity.h>
#include "../../lib/wmbus/src/Drivers/driver_sharky774.h"
#include "../../lib/wmbus/src/crc.hpp"
#include "../../lib/wmbus/src/utils.hpp"
#include "../../lib/wmbus/src/SensorBase.h"
#define SUPLA_CC1101
#ifdef SUPLA_CC1101
#include "../../src/src/sensor/WmbusMeter.h"
#endif
#include <mbus_packet.hpp>

void setUp() {
}

void tearDown() {
}


void test_full_frame_full_key() {
  // Test AES-128-CBC decryption with full 16-byte key (reversed order)
  // Key format: PRIOS_DEFAULT_KEY2 + PRIOS_DEFAULT_KEY1 concatenated
  // Frame type: OMS (ci_field=0x7A, tpl_cfg=0x0550, bit 12=0)
  // Encryption: AES-128-CBC
  auto wmbus_meter1 = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
  auto wmbus_driver1 = new Sharky774();
  wmbus_meter1->add_driver(wmbus_driver1);

  // Meter ID: 81052818 (pair-swapped from frame bytes 4-7: 18280581)
  auto wmbus_sensor1 = new Supla::Sensor::SensorBase("81052818", "sharky774", "total_energy_consumption_kwh", "51728910E66D83F851728910E66D83F8");
  wmbus_meter1->add_sensor(wmbus_sensor1);
  // 5E44A5111828058141046DF17A4900500521BA8F123FDE8B924B3EFB3B7BD143159D913EB08FA29EE2030F73B672E1EC8F4253B98A443DF7051AF17F3AF98E3FEF86677BF5FB674534CCE08BC9BD73137EE513479F405F915848755F7CD08FDAF4153994C741E1CD7824616432
  static uint8_t sharky[109] = {0x5E, 0x44, 0xA5, 0x11, 0x18, 0x28, 0x05, 0x81, 0x41, 0x04, 0x6D, 0xF1, 0x7A, 0x49, 0x00, 0x50,
                                0x05, 0x21, 0xBA, 0x8F, 0x12, 0x3F, 0xDE, 0x8B, 0x92, 0x4B, 0x3E, 0xFB, 0x3B, 0x7B, 0xD1, 0x43,
                                0x15, 0x9D, 0x91, 0x3E, 0xB0, 0x8F, 0xA2, 0x9E, 0xE2, 0x03, 0x0F, 0x73, 0xB6, 0x72, 0xE1, 0xEC,
                                0x8F, 0x42, 0x53, 0xB9, 0x8A, 0x44, 0x3D, 0xF7, 0x05, 0x1A, 0xF1, 0x7F, 0x3A, 0xF9, 0x8E, 0x3F,
                                0xEF, 0x86, 0x67, 0x7B, 0xF5, 0xFB, 0x67, 0x45, 0x34, 0xCC, 0xE0, 0x8B, 0xC9, 0xBD, 0x73, 0x13,
                                0x7E, 0xE5, 0x13, 0x47, 0x9F, 0x40, 0x5F, 0x91, 0x58, 0x48, 0x75, 0x5F, 0x7C, 0xD0, 0x8F, 0xDA,
                                0xF4, 0x15, 0x39, 0x94, 0xC7, 0x41, 0xE1, 0xCD, 0x78, 0x24, 0x61, 0x64, 0x32};

  auto calculatedPacketSize = packetSize(sharky[0]);
  Serial.print("Calculated packet size: ");
  Serial.println(calculatedPacketSize);
  uint8_t len_without_crc = crcRemove(sharky, calculatedPacketSize);
    Serial.print("Length without CRC: ");
  Serial.println(len_without_crc);
  std::vector<unsigned char> frame(sharky, sharky + len_without_crc);
  float result = wmbus_meter1->parse_frame(frame);

  // parse_frame returns the value for the configured property (total_energy_consumption_kwh)
  // Expected value from wmbusmeters decode: 5366.111111 kWh (see comment at top of file)
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5366.111f, result);
}

void test_wmbus_meter_parse_all_values() {
  // Test complete parsing of all Sharky774 fields from AES-encrypted telegram
  // Frame type: OMS (ci_field=0x7A, tpl_cfg bit 12=0)
  // Encryption: AES-128-CBC with full 16-byte key
  // Validates: energy, volume, flow, power, temperatures, operating times
  auto wmbus_meter = new Supla::Sensor::WmbusMeter(1, 2, 3, 4, 6, 7, true);
  auto wmbus_driver = new Sharky774();
  wmbus_meter->add_driver(wmbus_driver);

  // Meter ID: 81052818 (pair-swapped from frame bytes 4-7: 18280581)
  auto wmbus_sensor = new Supla::Sensor::SensorBase("81052818", "sharky774", "total_energy_consumption_kwh", "51728910E66D83F839BC8A10E66D83F8");
  wmbus_meter->add_sensor(wmbus_sensor);
  // 5E44A5111828058141046DF17A4900500521BA8F123FDE8B924B3EFB3B7BD143159D913EB08FA29EE2030F73B672E1EC8F4253B98A443DF7051AF17F3AF98E3FEF86677BF5FB674534CCE08BC9BD73137EE513479F405F915848755F7CD08FDAF4153994C741E1CD78246164320000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

  static uint8_t sharky[254] = {0x5E, 0x44, 0xA5, 0x11, 0x18, 0x28, 0x05, 0x81, 0x41, 0x04, 0x6D, 0xF1, 0x7A, 0x49, 0x00, 0x50,
                                0x05, 0x21, 0xBA, 0x8F, 0x12, 0x3F, 0xDE, 0x8B, 0x92, 0x4B, 0x3E, 0xFB, 0x3B, 0x7B, 0xD1, 0x43,
                                0x15, 0x9D, 0x91, 0x3E, 0xB0, 0x8F, 0xA2, 0x9E, 0xE2, 0x03, 0x0F, 0x73, 0xB6, 0x72, 0xE1, 0xEC,
                                0x8F, 0x42, 0x53, 0xB9, 0x8A, 0x44, 0x3D, 0xF7, 0x05, 0x1A, 0xF1, 0x7F, 0x3A, 0xF9, 0x8E, 0x3F,
                                0xEF, 0x86, 0x67, 0x7B, 0xF5, 0xFB, 0x67, 0x45, 0x34, 0xCC, 0xE0, 0x8B, 0xC9, 0xBD, 0x73, 0x13,
                                0x7E, 0xE5, 0x13, 0x47, 0x9F, 0x40, 0x5F, 0x91, 0x58, 0x48, 0x75, 0x5F, 0x7C, 0xD0, 0x8F, 0xDA,
                                0xF4, 0x15, 0x39, 0x94, 0xC7, 0x41, 0xE1, 0xCD, 0x78, 0x24, 0x61, 0x64, 0x32, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  uint8_t len_without_crc = crcRemove(sharky, packetSize(sharky[0]));
  std::vector<unsigned char> frame(sharky, sharky + len_without_crc);
  float result = wmbus_meter->parse_frame(frame);

  // parse_frame returns the value for the configured property (total_energy_consumption_kwh)
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5269.722f, result);

  auto values = wmbus_driver->get_values(frame);
  TEST_ASSERT_TRUE(values.count("total_energy_consumption_kwh") > 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5269.722f, values["total_energy_consumption_kwh"]);

  TEST_ASSERT_TRUE(values.count("total_volume_m3") > 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 538.96f, values["total_volume_m3"]);

  TEST_ASSERT_TRUE(values.count("power_kw") > 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.8f, values["power_kw"]);

  TEST_ASSERT_TRUE(values.count("flow_temperature_c") > 0);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 44.6f, values["flow_temperature_c"]);

  TEST_ASSERT_TRUE(values.count("return_temperature_c") > 0);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 31.5f, values["return_temperature_c"]);

  TEST_ASSERT_TRUE(values.count("operating_time_h") > 0);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 23662.0f, values["operating_time_h"]);

  TEST_ASSERT_TRUE(values.count("operating_time_in_error_h") > 0);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 0.0f, values["operating_time_in_error_h"]);

  TEST_ASSERT_TRUE(values.count("volume_flow_m3h") > 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.053f, values["volume_flow_m3h"]);
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------
#include <Arduino.h>
void setup() {
  Serial.begin(115200);
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_full_frame_full_key);

  UNITY_END();
}
void loop() {
}
