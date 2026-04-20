/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_apatorna1.cc
  Copyright (C) 2024 saganos, Fredrik Öhrström (gpl-3.0-or-later)

  APT-WMBUS-NA-1 water meter module (Apator Powogaz).

  Telegram structure (after CRC removal, 29 bytes):
    Bytes  0-9  : wM-Bus DLL header (len, C, M[2], A[4], version, type)
    Byte   10   : CI field = 0xA0 or 0xA1 (manufacturer specific)
    Byte   11   : Access counter (used in IV construction)
    Byte   12   : Flags byte (plaintext)
    Bytes 13-28 : AES-128-CBC encrypted payload (16 bytes)

  Encryption:
    Algorithm : AES-128-CBC
    Key       : user-supplied (defaults to 16 x 0x00 for NOKEY devices)
    IV        : telegram[2..9] (M-field + A-field + version + type)
                + telegram[11] repeated 8 times (access counter)

  After decryption, bytes 13-28 contain:
    plain[0] = telegram[13]  (marker, typically 0x2E)
    plain[1] = telegram[14]  (multiplier exponent in bits[5:4], reading low nibble in bits[3:0])
    plain[2] = telegram[15]  (reading bits [11:4])
    plain[3] = telegram[16]  (reading bits [19:12])
    plain[4] = telegram[17]  (reading bits [27:20])
    ...

  Volume formula:
    multiplier = 10 ^ ((plain[1] & 0x30) >> 4)
    reading    = plain[4]<<20 | plain[3]<<12 | plain[2]<<4 | (plain[1] & 0x0F)
    volume_m3  = reading * multiplier / 1000.0

  Detection: manufacturer=APA (0x0601), device type=0x07 (water), version=0x14
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>
#include <cmath>

struct ApatorNA1 : Driver {
    ApatorNA1() : Driver(std::string("apatorna1")) {
        addDetection(MANFCODE('A', 'P', 'A'), 0x07, 0x14);
    }

    virtual std::map<std::string, float> get_values(std::vector<unsigned char> &telegram) override {
        std::map<std::string, float> ret_val{};

        // Require at least 18 bytes (to access index 17) after DLL header.
        // After decrypt_telegram the payload at telegram[13..28] is plaintext.
        if (telegram.size() < 18) {
            return {};
        }

        // CI must be 0xA0 or 0xA1 (manufacturer specific – Apator NA-1 format)
        if (telegram[10] != 0xA0 && telegram[10] != 0xA1) {
            return {};
        }

        // Decrypted payload starts at byte 13 (= plain[0]).
        // plain[1] = telegram[14] : multiplier exponent (bits[5:4]) and low nibble of reading
        // plain[2] = telegram[15] : reading bits[11:4]
        // plain[3] = telegram[16] : reading bits[19:12]
        // plain[4] = telegram[17] : reading bits[27:20]
        const int exp = (static_cast<int>(telegram[14]) & 0b00110000) >> 4;
        const float multiplier = static_cast<float>(pow(10.0, exp));

        const int reading =
            (static_cast<int>(telegram[17]) << 20) |
            (static_cast<int>(telegram[16]) << 12) |
            (static_cast<int>(telegram[15]) << 4)  |
            (static_cast<int>(telegram[14]) & 0x0F);

        const float volume = static_cast<float>(reading) * multiplier / 1000.0f;

        add_to_map(ret_val, "total_water_m3", volume);

        return ret_val;
    }
};
