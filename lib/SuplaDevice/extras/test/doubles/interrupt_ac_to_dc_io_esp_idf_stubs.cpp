/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
*/

#include <interrupt_ac_to_dc_io_esp_idf_stubs.h>

#include <array>

namespace {
constexpr int kMaxGpios = 64;

std::array<int, kMaxGpios> gpioLevels = {};
std::array<void (*)(void *), kMaxGpios> gpioHandlers = {};
std::array<void *, kMaxGpios> gpioHandlerArgs = {};
}  // namespace

esp_err_t gpio_config(const gpio_config_t *config) {
  (void)config;
  return ESP_OK;
}

esp_err_t gpio_install_isr_service(int flags) {
  (void)flags;
  return ESP_OK;
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio,
                               void (*handler)(void *),
                               void *arg) {
  gpioHandlers[gpio] = handler;
  gpioHandlerArgs[gpio] = arg;
  return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio) {
  return gpioLevels[gpio];
}

void InterruptAcToDcIoTestSupport::reset() {
  gpioLevels.fill(0);
  gpioHandlers.fill(nullptr);
  gpioHandlerArgs.fill(nullptr);
}

void InterruptAcToDcIoTestSupport::setGpioLevel(int gpio, int level) {
  gpioLevels[gpio] = level;
}

void InterruptAcToDcIoTestSupport::triggerInterrupt(int gpio, int count) {
  for (int i = 0; i < count; i++) {
    gpioHandlers[gpio](gpioHandlerArgs[gpio]);
  }
}
