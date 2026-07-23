/*
 * Copyright (C) AC SOFTWARE SP. Z O.O
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config_mock.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <simple_time.h>
#include <supla/clock/clock.h>
#include <supla/device/enter_cfg_mode_after_power_cycle.h>
#include <supla/storage/storage.h>

namespace {
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;

class EnterCfgModeAfterPowerCycleTests : public ::testing::Test {
 protected:
  void SetUp() override {
    Supla::Storage::SetConfigInstance(&config);
  }

  void TearDown() override {
    Supla::Storage::SetConfigInstance(nullptr);
  }

  Supla::Clock clock;
  SimpleTime time;
  NiceMock<ConfigMock> config;
};
}  // namespace

TEST_F(EnterCfgModeAfterPowerCycleTests,
       ResetCounterClearsStoredCounterAndStopsCurrentSessionIncrement) {
  Supla::Device::EnterCfgModeAfterPowerCycle powerCycle(5000, 3, true);

  EXPECT_CALL(config, getUInt32(StrEq("power_cycle"), _))
      .WillOnce(DoAll(SetArgPointee<1>(2), Return(true)));
  powerCycle.onLoadConfig(nullptr);

  EXPECT_CALL(config, setUInt32(StrEq("power_cycle"), 0))
      .WillOnce(Return(true));
  EXPECT_CALL(config, commit()).Times(1);
  EXPECT_CALL(config, saveWithDelay(_)).Times(0);
  powerCycle.resetCounter();
  powerCycle.iterateAlways();
}
