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
#include <supla/storage/key_value.h>
#include <supla/log_wrapper.h>
#include <string.h>

namespace Supla {
const char ConfigFileName[] = "/supla-dev.cfg";
const char BackupConfigFileName[] = "/supla-dev.cfg.bak";
const char CustomCAFileName[] = "/custom_ca.pem";
};  // namespace Supla

#define BIG_BLOG_SIZE_TO_BE_STORED_IN_FILE 32

Supla::SPIFFSConfig::SPIFFSConfig(int configMaxSize) : configMaxSize(configMaxSize) {
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

  auto files = {ConfigFileName, BackupConfigFileName};
  bool result = false;

  for (auto file : files) {
    if (SPIFFS.exists(file)) {
      File cfg = SPIFFS.open(file, "r");
      if (!cfg) {
        SUPLA_LOG_ERROR("SPIFFSConfig: failed to open \"%s\"", file);
        continue;
      }

      int fileSize = cfg.size();

      SUPLA_LOG_DEBUG("SPIFFSConfig: file \"%s\" size %d", file, fileSize);
      if (fileSize > configMaxSize) {
        SUPLA_LOG_ERROR("SPIFFSConfig: config file is too big");
        cfg.close();
        continue;
      }

      uint8_t* buf = new uint8_t[configMaxSize];
      if (buf == nullptr) {
        SUPLA_LOG_ERROR("SPIFFSConfig: failed to allocate memory");
        cfg.close();
        continue;
      }

      memset(buf, 0, configMaxSize);
      int bytesRead = cfg.read(buf, fileSize);
      cfg.close();
      if (bytesRead != fileSize) {
        SUPLA_LOG_DEBUG("SPIFFSConfig: read bytes %d, while file is %d bytes", bytesRead, fileSize);
        delete[] buf;
        continue;
      }

      SUPLA_LOG_DEBUG("SPIFFSConfig: initializing storage from file...");
      result = initFromMemory(buf, fileSize);
      delete[] buf;
      SUPLA_LOG_DEBUG("SPIFFSConfig: init result %s", result ? "success" : "failure");

      if (!result) {
        continue;
      }
      else {
        break;
      }
    }
    else {
      SUPLA_LOG_DEBUG("SPIFFSConfig:: config file \"%s\" missing", file);
    }
  }
  SPIFFS.end();
  return result;
}

void Supla::SPIFFSConfig::commit() {
  uint8_t* buf = new uint8_t[configMaxSize];
  if (buf == nullptr) {
    SUPLA_LOG_ERROR("SPIFFSConfig: failed to allocate memory");
    return;
  }

  memset(buf, 0, configMaxSize);

  size_t dataSize = serializeToMemory(buf, configMaxSize);

  if (!initSPIFFS()) {
    return;
  }

  auto files = {ConfigFileName, BackupConfigFileName};
  bool result = false;

  for (auto file : files) {
    SUPLA_LOG_DEBUG("SPIFFSConfig: writing to file \"%s\"", file);
    File cfg = SPIFFS.open(file, "w");
    if (!cfg) {
      SUPLA_LOG_ERROR("SPIFFSConfig: failed to open config file \"%s\"for write", file);
      SPIFFS.end();
      return;
    }

    cfg.write(buf, dataSize);
    cfg.close();
  }
  delete[] buf;
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

  int bytesRead = file.read(reinterpret_cast<uint8_t*>(value), fileSize);

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
