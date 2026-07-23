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

#ifndef SRC_SUPLA_INPUT_NOISE_GUARD_H_
#define SRC_SUPLA_INPUT_NOISE_GUARD_H_

#include <stdint.h>

#ifndef SUPLA_INPUT_NOISE_GUARD_WIFI_TRANSITION_MS
#define SUPLA_INPUT_NOISE_GUARD_WIFI_TRANSITION_MS 1000
#endif

namespace Supla {
namespace InputNoiseGuard {

void NotifyWifiTransition();
void NotifyWifiStaDisconnected();
void NotifyWifiStaConnected();
void IgnoreForMs(uint32_t timeoutMs);

void SetWifiTransitionGuardMs(uint32_t timeoutMs);
uint32_t GetWifiTransitionGuardMs();

bool IsActive();
uint32_t RemainingMs();
void Clear();

}  // namespace InputNoiseGuard
}  // namespace Supla

#endif  // SRC_SUPLA_INPUT_NOISE_GUARD_H_
