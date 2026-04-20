/*
  Based on: https://github.com/wmbusmeters/wmbusmeters/blob/master/src/driver_op041a.cc
            https://wmbusmeters.org/drivers/op041a.xmq.html
  Copyright (C) 2026 Fredrik Öhrström (gpl-3.0-or-later)

  Apator NAXOM OP-04-1A water meter module.

  Telegram structure (after CRC removal):
    Bytes  0-9  : wM-Bus DLL header (len, C, M[2], A[4], version, type)
    Bytes 10-12 : ELL – Extended Link Layer I (CI=0x8C, CC, ACC)
    Byte   13   : TPL CI field = 0x7A (short transport layer, AES_CBC_IV)
    Byte   14   : TPL access number
    Byte   15   : TPL status
    Bytes 16-17 : TPL config word
    Bytes 18-19 : 0x2F 0x2F  (decryption-succeeded marker, present when using
                               a pre-decrypted / NOKEY telegram)
    Byte   20+  : Application data (DIF/VIF records)

  Key application-data record:
    04 13 XX XX XX XX  →  total_water_m3 (32-bit integer, unit = m³ × 10⁻³)

  Detection: manufacturer = APA (0x0601), device type = 0x07 (water),
             version = 0x1A.

  Encryption:
    AES-128-CBC with bidirectional IV (nb=7 blocks).
    Use the meter's individual AES key.  When the telegram is already
    decrypted (NOKEY / analyse mode), configure the sensor with an empty key
    so that WmbusMeter::decrypt_telegram() is skipped – the 0x2F2F check
    bytes confirm the data is already in plaintext.
*/

#pragma once

#include "driver.h"

#include <vector>
#include <string>

struct ApatorOP04 : Driver {
    ApatorOP04() : Driver(std::string("apatorop04")) {
        addDetection(MANFCODE('A', 'P', 'A'), 0x07, 0x1A);
    }

    virtual std::map<std::string, float> get_values(std::vector<unsigned char> &telegram) override {
        std::map<std::string, float> ret_val{};

        float vol = this->get_0413(telegram);
        if (vol > 0.0f || telegram.size() > 20) {
            add_to_map(ret_val, "total_water_m3", vol);
        }

        if (ret_val.size() > 0) {
            return ret_val;
        }
        return {};
    }
};
