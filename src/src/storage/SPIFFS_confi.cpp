/*
  Copyright (C) krycha88

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef SUPLA_EXCLUDE_SPIFFS_CONFIG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#if !defined(ARDUINO_ARCH_AVR)
// don't compile it on Arduino Mega

#ifdef ARDUINO_ARCH_ESP8266
#include "FS.h"
#elif ARDUINO_ARCH_ESP32
#include "SPIFFS.h"
#endif

#include "SPIFFS_config.h"
#include <supla/log_wrapper.h>

namespace Supla {
const char ConfigFileName[] = "/supla-dev.cfg";
const char CustomCAFileName[] = "/custom_ca.pem";
};  // namespace Supla

#define SUPLA_SPIFFS_CONFIG_BUF_SIZE 1024

#define BIG_BLOG_SIZE_TO_BE_STORED_IN_FILE 32

Supla::SPIFFSConfig::SPIFFSConfig() {
}

Supla::SPIFFSConfig::~SPIFFSConfig() {
}

bool Supla::SPIFFSConfig::init() {
  if (first) {
    SUPLA_LOG_WARNING("SPIFFSConfig: init called on non empty database. Aborting");
    // init can be done only on empty storage
    return false;
  }

  if (!initSPIFFS()) {
    return false;
  }

  if (SPIFFS.exists(ConfigFileName)) {
    File cfg = SPIFFS.open(ConfigFileName, "r");
    if (!cfg) {
      SUPLA_LOG_ERROR("SPIFFSConfig: failed to open config file");
      SPIFFS.end();
      return false;
    }

    int fileSize = cfg.size();

    SUPLA_LOG_DEBUG("SPIFFSConfig: config file size %d", fileSize);
    if (fileSize > SUPLA_SPIFFS_CONFIG_BUF_SIZE) {
      SUPLA_LOG_ERROR("SPIFFSConfig: config file is too big");
      cfg.close();
      SPIFFS.end();
      return false;
    }

    uint8_t buf[SUPLA_SPIFFS_CONFIG_BUF_SIZE] = {};
    int bytesRead = cfg.read(buf, fileSize);

    cfg.close();
    SPIFFS.end();
    if (bytesRead != fileSize) {
      SUPLA_LOG_DEBUG("SPIFFSConfig: read bytes %d, while file is %d bytes", bytesRead, fileSize);
      return false;
    }

    SUPLA_LOG_DEBUG("SPIFFSConfig: initializing storage from file...");
    auto result = initFromMemory(buf, fileSize);
    SUPLA_LOG_DEBUG("SPIFFSConfig: init result %d", result);
    return result;
  }
  else {
    SUPLA_LOG_DEBUG("SPIFFSConfig:: config file missing");
  }
  SPIFFS.end();
  return true;
}

void Supla::SPIFFSConfig::commit() {
  uint8_t buf[SUPLA_SPIFFS_CONFIG_BUF_SIZE] = {};

  size_t dataSize = serializeToMemory(buf, SUPLA_SPIFFS_CONFIG_BUF_SIZE);

  if (!initSPIFFS()) {
    return;
  }

  File cfg = SPIFFS.open(ConfigFileName, "w");
  if (!cfg) {
    SUPLA_LOG_ERROR("SPIFFSConfig: failed to open config file for write");
    SPIFFS.end();
    return;
  }

  cfg.write(buf, dataSize);
  cfg.close();
  SPIFFS.end();
}

bool Supla::SPIFFSConfig::getCustomCA(char* customCA, int maxSize) {
  if (!initSPIFFS()) {
    return false;
  }

  if (SPIFFS.exists(CustomCAFileName)) {
    File file = SPIFFS.open(CustomCAFileName, "r");
    if (!file) {
      SUPLA_LOG_ERROR("SPIFFSConfig: failed to open custom CA file");
      SPIFFS.end();
      return false;
    }

    int fileSize = file.size();

    if (fileSize > maxSize) {
      SUPLA_LOG_ERROR("SPIFFSConfig: custom CA file is too big");
      file.close();
      SPIFFS.end();
      return false;
    }

    int bytesRead = file.read(reinterpret_cast<uint8_t*>(customCA), fileSize);

    file.close();
    SPIFFS.end();
    if (bytesRead != fileSize) {
      SUPLA_LOG_DEBUG("SPIFFSConfig: read bytes %d, while file is %d bytes", bytesRead, fileSize);
      return false;
    }

    return true;
  }
  else {
    SUPLA_LOG_DEBUG("SPIFFSConfig:: custom ca file missing");
  }
  SPIFFS.end();
  return true;
}

int Supla::SPIFFSConfig::getCustomCASize() {
  if (!initSPIFFS()) {
    return 0;
  }

  if (SPIFFS.exists(CustomCAFileName)) {
    File file = SPIFFS.open(CustomCAFileName, "r");
    if (!file) {
      SUPLA_LOG_ERROR("SPIFFSConfig: failed to open custom CA file");
      SPIFFS.end();
      return false;
    }

    int fileSize = file.size();

    file.close();
    SPIFFS.end();
    return fileSize;
  }
  return 0;
}

bool Supla::SPIFFSConfig::setCustomCA(const char* customCA) {
  size_t dataSize = strlen(customCA);

  if (!initSPIFFS()) {
    return false;
  }

  File file = SPIFFS.open(CustomCAFileName, "w");
  if (!file) {
    SUPLA_LOG_ERROR("SPIFFSConfig: failed to open custom CA file for write");
    SPIFFS.end();
    return false;
  }

  file.write(reinterpret_cast<const uint8_t*>(customCA), dataSize);
  file.close();
  SPIFFS.end();
  return true;
}

bool Supla::SPIFFSConfig::initSPIFFS() {
  bool result = SPIFFS.begin();
  if (!result) {
    SUPLA_LOG_WARNING("SPIFFSConfig: formatting partition");
    SPIFFS.format();
    result = SPIFFS.begin();
    if (!result) {
      SUPLA_LOG_ERROR("SPIFFSConfig: failed to mount and to format partition");
    }
  }

  return result;
}

void Supla::SPIFFSConfig::removeAll() {
  SUPLA_LOG_DEBUG("SPIFFSConfig remove all called");

  if (!initSPIFFS()) {
    return;
  }
  SPIFFS.remove(CustomCAFileName);

  File suplaDir = SPIFFS.open("/supla", "r");
  if (suplaDir && suplaDir.isDirectory()) {
    File file = suplaDir.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        SUPLA_LOG_DEBUG("SPIFFSConfig: removing file /supla/%s", file.name());
        char path[200] = {};
        snprintf(path, sizeof(path), "/supla/%s", file.name());
        file.close();
        if (!SPIFFS.remove(path)) {
          SUPLA_LOG_ERROR("SPIFFSConfig: failed to remove file");
        }
      }
      file = suplaDir.openNextFile();
    }
  }
  else {
    SUPLA_LOG_DEBUG("SPIFFSConfig: failed to open supla directory");
  }

  SPIFFS.end();

  Supla::KeyValue::removeAll();
}

bool Supla::SPIFFSConfig::setBlob(const char* key, const char* value, size_t blobSize) {
  if (blobSize < BIG_BLOG_SIZE_TO_BE_STORED_IN_FILE) {
    return Supla::KeyValue::setBlob(key, value, blobSize);
  }

  SUPLA_LOG_DEBUG("SPIFFS: writing file %s", key);
  if (!initSPIFFS()) {
    return false;
  }

  SPIFFS.mkdir("/supla");

  char filename[50] = {};
  snprintf(filename, sizeof(filename), "/supla/%s", key);
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    SUPLA_LOG_ERROR("SPIFFSConfig: failed to open blob file \"%s\" for write", key);
    SPIFFS.end();
    return false;
  }

  file.write(reinterpret_cast<const uint8_t*>(value), blobSize);
  file.close();
  SPIFFS.end();
  return true;
}

bool Supla::SPIFFSConfig::getBlob(const char* key, char* value, size_t blobSize) {
  if (blobSize < BIG_BLOG_SIZE_TO_BE_STORED_IN_FILE) {
    return Supla::KeyValue::getBlob(key, value, blobSize);
  }

  if (!initSPIFFS()) {
    return false;
  }

  char filename[50] = {};
  snprintf(filename, sizeof(filename), "/supla/%s", key);
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    SUPLA_LOG_ERROR("SPIFFSConfig: failed to open blob file \"%s\" for read", key);
    SPIFFS.end();
    return false;
  }

  size_t fileSize = file.size();
  if (fileSize > blobSize) {
    SUPLA_LOG_ERROR("SPIFFSConfig: blob file is too big");
    file.close();
    SPIFFS.end();
    return false;
  }

  size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(value), fileSize);

  file.close();
  SPIFFS.end();
  return bytesRead == fileSize;
}

int Supla::SPIFFSConfig::getBlobSize(const char* key) {
  if (!initSPIFFS()) {
    return false;
  }

  char filename[50] = {};
  snprintf(filename, sizeof(filename), "/supla/%s", key);
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    SUPLA_LOG_ERROR("SPIFFSConfig: failed to open blob file \"%s\"", key);
    SPIFFS.end();
    return false;
  }
  int fileSize = file.size();

  file.close();
  SPIFFS.end();
  return fileSize;
}

#pragma GCC diagnostic pop
#endif
#endif  // SUPLA_EXCLUDE_SPIFFS_CONFIG
