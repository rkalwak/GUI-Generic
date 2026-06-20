/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_apator162.cc
  Copyright (C) 2017-2022 Fredrik Öhrström (gpl-3.0-or-later)
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

struct Apator162 : Driver
{
  Apator162() : Driver(std::string("apator162")){};
  virtual std::map<std::string, float> get_values(std::vector<unsigned char> &telegram) override
  {
    std::map<std::string, float> ret_val{};

    add_to_map(ret_val, "total_water_m3", this->get_total_water_m3(telegram));
    add_to_map(ret_val, "battery_voltage_v", this->get_battery_voltage(telegram));

    // Byte [24]: real-time active-tamper FLAGS (bitmask).
    //   bit0 (0x01) = magnetic tamper ACTIVE  — CONFIRMED 2026-06-17/18: set while magnet held
    //   bit2 (0x04) = cover tamper ACTIVE     — CONFIRMED 2026-06-18: set while cover removed
    // Both bits return to 0x00 immediately when the condition is removed.
    // When any bit is set, register 0x80 DISAPPEARS from the frame rotation.
    {
      uint8_t flags = (telegram.size() > 24) ? telegram[24] : 0;
      add_to_map(ret_val, "tamper_active",          (flags != 0)   ? 1.0f : 0.0f);
      add_to_map(ret_val, "magnetic_tamper_active", (flags & 0x01) ? 1.0f : 0.0f);
      add_to_map(ret_val, "cover_tamper_active",    (flags & 0x04) ? 1.0f : 0.0f);
    }

    // Register 0x80: cumulative tamper record (10 bytes).
    // Present in the frame rotation ONLY when byte[24]==0x00 (no active tamper).
    // When byte[24]!=0x00 (active tamper), this register DISAPPEARS from the frame
    // and only the normal 0x81/0x82 rotation is transmitted.
    //
    // evt[0] = tamper occurrence counter (increments with each new tamper event).
    // Returns 0.0 when the register is absent (i.e. during an active tamper event).
    {
      float tamper_count = this->get_event_count(telegram, 0x80);
      add_to_map(ret_val, "tamper_count", tamper_count >= 0.0f ? tamper_count : 0.0f);
    }

    // Register 0x85 (illumination / optical tamper, 11 bytes): NOT observed in hardware.
    // A 4-minute illumination test (2026-06-17 17:27-17:31) produced zero register changes;
    // only the normal 0x81/0x82 rotation was present throughout. Not decoded.

    // add_to_map(ret_val, "total_volume_m3",              this->get_random_value(telegram));
    // add_to_map(ret_val, "power_kw",                     this->get_random_value(telegram));
    // add_to_map(ret_val, "flow_temperature_c",           this->get_random_value(telegram));
    // add_to_map(ret_val, "return_temperature_c",         this->get_random_value(telegram));
    // add_to_map(ret_val, "operating_time_h",             this->get_random_value(telegram));
    // add_to_map(ret_val, "volume_flow_m3h",              this->get_random_value(telegram));

    if (ret_val.size() > 0)
    {
      return ret_val;
    }
    else
    {
      return {};
    }
  };

private:
  float get_total_water_m3(std::vector<unsigned char> &telegram)
  {
    float ret_val{};
    uint32_t usage = 0;
    size_t i = 25; // proprietary register stream starts at byte 25
    if (telegram[10] == 0xb6)
    {
      // Very old telegram format (CI=0xB6) — proprietary layout unknown, skip.
    }
    else
    {
      while (i < telegram.size())
      {
        int c = telegram[i];
        int size = this->registerSize(c);
        if (c == 0xff)
          break; // 0xFF = end-of-data marker; remaining bytes are AES padding
        i++;
        if (size == -1 || i + size >= telegram.size())
        {
          break;
        }
        if (c == 0x10 && size == 4 && i + size < telegram.size())
        {
          // Register 0x10: total volume, uint32_t little-endian, unit = litres
          usage = ((uint32_t)telegram[i + 3] << 24) | ((uint32_t)telegram[i + 2] << 16) |
                  ((uint32_t)telegram[i + 1] << 8) | ((uint32_t)telegram[i + 0]);
          ret_val = usage / 1000.0;
          break;
        }
        i += size;
      }
    }
    return ret_val;
  };

  float get_battery_voltage(std::vector<unsigned char> &telegram)
  {
    // Scan the manufacturer-specific payload for register 0x41 (battery voltage).
    // Register 0x41: 2 bytes, uint16_t little-endian, unit = millivolts.
    // Payload starts at byte 18 (byte 17 = 0x0F DIF marker).
    // Returns -1.0f when the register is absent (not all frame types include it).
    size_t i = 18;
    while (i < telegram.size())
    {
      int c = telegram[i];
      if (c == 0xFF)
        break;
      int size = this->registerSize(c);
      i++;
      if (size == -1 || i + (size_t)size > telegram.size())
        break;
      if (c == 0x41 && size == 2)
      {
        uint16_t raw = (uint16_t)telegram[i] | ((uint16_t)telegram[i + 1] << 8);
        return raw / 1000.0f; // mV → V
      }
      i += size;
    }
    return -1.0f; // register not present in this telegram
  };

  // Scan the proprietary register stream (starting at byte 25) for target_reg.
  // Returns evt[0] — the first byte of the event record (cumulative counter/ID).
  // Returns -1.0f if the register is absent in this telegram.
  float get_event_count(std::vector<unsigned char> &telegram, int target_reg)
  {
    size_t i = 25;
    while (i < telegram.size())
    {
      int c = telegram[i];
      if (c == 0xFF) break;
      int size = this->registerSize(c);
      i++;
      if (size == -1 || i + (size_t)size > telegram.size()) break;
      if (c == target_reg)
        return (float)telegram[i];
      i += size;
    }
    return -1.0f;
  };

  // Return the byte at position `offset` within the event data of target_reg.
  float get_random_value(std::vector<unsigned char> &telegram)
  {
    static bool seeded = false;
    if (!seeded)
    {
      std::srand(static_cast<unsigned int>(std::time(nullptr)));
      seeded = true;
    }
    // Generate random float between 0.0 and 100.0
    return static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 100.0f));
  };

  int registerSize(int c)
  {
    switch (c)
    {
      // 0x0F is a DIF marker, not a register — handled separately by the caller.

    case 0x00:
      return 4; // Date
    case 0x01:
      return 3; // Faults (e.g. 0F 09 4D A1 97 18 02 00 → payload 18 02 00)

    case 0xA1:
    case 0x10:
      return 4; // Total volume (litres, uint32_t LE)

    case 0x11:
      return 2; // Flow

    case 0x40:
      return 6; // Detectors
    case 0x41:
      return 2; // Battery voltage (mV, uint16_t LE)
    case 0x42:
      return 4; // Energy
    case 0x43:
      return 2; // Life days since factory start (uint16_t LE)

    case 0x44:
      return 3;

    case 0x71:
      return 1 + 2 * 4;  // Historical data: 1 count byte + 2 entries × 4 bytes
    case 0x72:
      return 1 + 3 * 4;  // Historical data: 1 + 3×4
    case 0x73:
      return 1 + 4 * 4;  // Historical data: 1 + 4×4
    case 0x75:
      return 1 + 6 * 4;  // Historical data: 1 + 6×4
    case 0x7B:
      return 1 + 12 * 4; // Historical data: 1 + 12×4 (long/6-block frame)

    // Event registers (10 bytes each):
    //   evt[0]     cumulative counter / tamper ID
    //   evt[1-2]   date of first occurrence  (bits[4:0]=day, bits[3:0]=month)
    //   evt[3]     time of first occurrence
    //   evt[4-5]   date of last occurrence
    //   evt[6]     tamper-type bitmap (0x80 only): bit1=magnetic, bit0=cover
    //   evt[7]     time of last occurrence
    //   evt[8-9]   padding / flags
    case 0x80: // Physical tamper record (per-type: see byte[24] bitmask)
    case 0x81: // Type-A normal transmission record (not an alarm)
    case 0x82: // Type-B normal transmission record (not an alarm)
    case 0x83: // Backflow — register ABSENT in 3-block frames even during real backflow events
               // (CONFIRMED DENIED 2026-06-20: volume decreases but 0x83 never appears)
               // Backflow only detectable via total_water_m3 decreasing.
    case 0x84: // Unconfirmed — never observed in hardware
    case 0x86:
    case 0x87:
      return 10;

    case 0x85: // Illumination / optical tamper (11 bytes) — NOT observed triggering
    case 0x88:
    case 0x8F:
      return 11;

    case 0x8A:
      return 9;

    case 0x8B:
    case 0x8C:
      return 6;

    case 0x8E:
      return 7;

    case 0xA0:
      return 4;

    case 0xA2:
      return 1;

    case 0xA3:
      return 7;

    case 0xA4:
      return 4;

    case 0xA5:
    case 0xA9:
    case 0xAF:
      return 1;

    case 0xA6:
      return 3;

    case 0xA7:
    case 0xA8:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
      return 2;

    case 0xB0:
      return 5;
    case 0xB1:
      return 8;
    case 0xB2:
      return 16;
    case 0xB3:
      return 8;
    case 0xB4:
      return 2;
    case 0xB5:
      return 16;

    // Unknown
    case 0xB6:
      return 3;
    case 0xB7:
      return 3;
    case 0xB8:
      return 3;
    case 0xB9:
      return 3;
    case 0xBA:
      return 3;
    case 0xBB:
      return 3;
    case 0xBC:
      return 3;
    case 0xBD:
      return 3;
    case 0xBE:
      return 3;
    case 0xBF:
      return 3;

    case 0xC0:
      return 3;
    case 0xC1:
      return 3;
    case 0xC2:
      return 3;
    case 0xC3:
      return 3;
    case 0xC4:
      return 3;
    case 0xC5:
      return 3;
    case 0xC6:
      return 3;
    case 0xC7:
      return 3;

    case 0xD0:
      return 3;
    case 0xD3:
      return 3;

    case 0xF0:
      return 4;
    }
    return -1;
  }
};

/*
 * ============================================================================
 * Apator 162 — Decrypted Frame Structure (reverse-engineered, 2026-06)
 * Meter: 00489912, Manufacturer: APA (0x0106), Type: 0x07 (water), Version: 0x05
 * ============================================================================
 *
 * Two variants observed in logs:
 *   Short (3-block, 63 bytes):  L=0x3E, config=0x3085, historical reg=0x71 (9 B)
 *   Long  (6-block, 111 bytes): L=0x6E, config=0x6085, historical reg=0x7B (49 B)
 *
 * Byte map (0-indexed from start of decrypted frame, including L field):
 * -----------------------------------------------------------------------
 *  [0]      L field
 *  [1]      C field (0x44)
 *  [2-3]    Manufacturer code LE (0x01 0x06 = APA)
 *  [4-7]    Meter ID little-endian BCD
 *  [8]      Version
 *  [9]      Device type (0x07 = water)
 *  [10]     CI field (0x7A = short TPL header, AES-CBC-IV)
 *  [11]     Access number (increments each frame)
 *  [12]     TPL Status byte — 0x00 = OK; non-zero = active alarm
 *  [13-14]  Config word (AES-CBC-IV encryption)
 *  [15-16]  0x2F 0x2F — AES decryption check bytes
 *  [17]     0x0F — DIF marker (manufacturer-specific section start)
 *  [18]     Internal frame counter (monotonically incrementing)
 *  [19]     Date byte 1: bits[4:0]=day (1-31), bits[7:5]=year_low
 *  [20]     Date byte 2: bits[3:0]=month (1-12), bits[7:4]=year_high
 *  [21-22]  0x9A 0x06 — reference bytes, constant per meter
 *  [23-24]  0x02 0x00 — fixed bytes
 *
 * --- Proprietary register stream starts at byte 25 ---
 *
 *  [25]     Register 0x43 (life days)
 *  [26-27]  uint16_t LE — days since factory start
 *
 *  [28]     Event register code (see table below)
 *  [29-38]  Event record data (10 bytes; 11 bytes for reg 0x85)
 *
 *  [39]     Register 0x10 (total volume)  *offset shifts if alarm regs present*
 *  [40-43]  uint32_t LE — volume in litres (divide by 1000 for m³)
 *
 *  [44]     Historical data register (0x71 = 9 B short / 0x7B = 49 B long)
 *  [...]    Historical entries
 *  [...]    Register 0xA0 (4 bytes)
 *  [...]    0xFF — end marker
 *
 * ============================================================================
 * Event register codes and meanings
 * ============================================================================
 *
 *  0x80   10 B   Physical tamper record (combined: both magnetic and cover events)
 *                 -> tamper_count (evt[0], cumulative occurrence counter)
 *                 Per-type discrimination is via byte[24] bitmask (NOT via evt[6]):
 *                   byte[24] bit0 (0x01) = magnetic_tamper_active
 *                   byte[24] bit2 (0x04) = cover_tamper_active
 *                 CONFIRMED 2026-06-18 hardware logs.
 *  0x81   10 B   Type-A frame record     (normal transmission, NOT an alarm)
 *  0x82   10 B   Type-B frame record     (normal transmission, NOT an alarm)
 *  0x83   10 B   wsteczny przepływ       (backflow)
 *                                         [ABSENT in 3-block frames even during real backflow
 *                                          events: CONFIRMED DENIED 2026-06-20. Volume
 *                                          decreases (total_water_m3) but register never
 *                                          appears. Not decoded; may only exist in 6-block.]
 *  0x84   10 B   NOT observed in any hardware log (previously assumed cover removal — DENIED)
 *  0x85   11 B   oświetlanie             (illumination / optical)
 *                                         [NOT observed: 4-min test showed NO register change]
 *
 * Registers 0x81 and 0x82 alternate every frame in normal operation.
 * While byte[24]!=0x00 (active tamper), register 0x80 is ABSENT from the frame;
 * only 0x81/0x82 rotate. After tamper cleared, 0x80 rejoins the rotation.
 * Only ONE alarm register slot exists in the 3-block (63-byte) short frame.
 *
 * Confirmed via real hardware logs (meter 00489912, June 2026):
 *   - Byte [24] bit0 (0x01): magnetic tamper ACTIVE while magnet held; 0x00 after release
 *   - Byte [24] bit2 (0x04): cover tamper ACTIVE while cover removed; 0x00 after replace
 *   - Register 0x80 ABSENT whenever byte[24]!=0x00 (any active tamper)
 *   - Register 0x84 NEVER appeared in any captured frame
 *   - Register 0x85 NEVER triggered despite 4-minute optical illumination test
 *   - evt[6] inside 0x80 is a time byte, NOT a tamper-type bitmap
 *
 * ============================================================================
 * 10-byte event record structure (bytes relative to register data start)
 * ============================================================================
 *
 *  [0]    Occurrence count (uint8_t); 0x00 = no events recorded
 *  [1]    First-occurrence date byte 1: bits[4:0]=day, bits[7:5]=year_low
 *  [2]    First-occurrence date byte 2: bits[3:0]=month, bits[7:4]=year_high
 *  [3]    First-occurrence time byte
 *  [4]    Last-occurrence date byte 1:  bits[4:0]=day, bits[7:5]=year_low
 *  [5]    Last-occurrence date byte 2:  bits[3:0]=month, bits[7:4]=year_high
 *  [6]    Time byte of last occurrence (NOT a tamper-type bitmap — disproved 2026-06-18)
 *  [7]    Time byte of last occurrence
 *  [8]    Additional flags/accumulated counter (0x00 in normal frames)
 *  [9]    Padding (always 0x00); register 0x85 has an extra byte here
 *
 * NOTE: evt[0] (occurrence count) is what this driver exposes as *_count fields.
 * ============================================================================
 */