/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

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

#include <supla/input_noise_guard.h>

#include <supla/time.h>
#include <supla/log_wrapper.h>

namespace {
uint32_t ignoreUntilMs = 0;
uint32_t wifiTransitionGuardMs = SUPLA_INPUT_NOISE_GUARD_WIFI_TRANSITION_MS;
bool wifiStaDisconnectSeriesActive = false;
uint32_t wifiStaDisconnectSeriesStartMs = 0;

constexpr uint32_t kWifiStaDisconnectMaxGuardMultiplier = 3;

bool timeBefore(uint32_t left, uint32_t right) {
  return static_cast<int32_t>(left - right) < 0;
}
}  // namespace

void Supla::InputNoiseGuard::NotifyWifiTransition() {
  wifiStaDisconnectSeriesActive = false;
  wifiStaDisconnectSeriesStartMs = 0;
  IgnoreForMs(wifiTransitionGuardMs);
}

void Supla::InputNoiseGuard::NotifyWifiStaDisconnected() {
  uint32_t now = millis();
  if (!wifiStaDisconnectSeriesActive) {
    wifiStaDisconnectSeriesActive = true;
    wifiStaDisconnectSeriesStartMs = now;
  }

  uint32_t maxIgnoreUntilMs =
      wifiStaDisconnectSeriesStartMs +
      wifiTransitionGuardMs * kWifiStaDisconnectMaxGuardMultiplier;
  if (!timeBefore(now, maxIgnoreUntilMs)) {
    return;
  }

  uint32_t newIgnoreUntilMs = now + wifiTransitionGuardMs;
  if (timeBefore(maxIgnoreUntilMs, newIgnoreUntilMs)) {
    newIgnoreUntilMs = maxIgnoreUntilMs;
  }
  IgnoreForMs(newIgnoreUntilMs - now);
}

void Supla::InputNoiseGuard::NotifyWifiStaConnected() {
  wifiStaDisconnectSeriesActive = false;
  wifiStaDisconnectSeriesStartMs = 0;
}

void Supla::InputNoiseGuard::IgnoreForMs(uint32_t timeoutMs) {
  if (timeoutMs == 0) {
    return;
  }

  uint32_t newIgnoreUntilMs = millis() + timeoutMs;
  if (!IsActive() || timeBefore(ignoreUntilMs, newIgnoreUntilMs)) {
    ignoreUntilMs = newIgnoreUntilMs;
  }
}

void Supla::InputNoiseGuard::SetWifiTransitionGuardMs(uint32_t timeoutMs) {
  wifiTransitionGuardMs = timeoutMs;
}

uint32_t Supla::InputNoiseGuard::GetWifiTransitionGuardMs() {
  return wifiTransitionGuardMs;
}

bool Supla::InputNoiseGuard::IsActive() {
  return ignoreUntilMs != 0 && timeBefore(millis(), ignoreUntilMs);
}

uint32_t Supla::InputNoiseGuard::RemainingMs() {
  if (!IsActive()) {
    return 0;
  }
  return ignoreUntilMs - millis();
}

void Supla::InputNoiseGuard::Clear() {
  ignoreUntilMs = 0;
  wifiStaDisconnectSeriesActive = false;
  wifiStaDisconnectSeriesStartMs = 0;
}
