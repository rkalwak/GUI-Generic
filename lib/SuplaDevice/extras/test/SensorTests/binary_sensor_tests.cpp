/*
   Copyright (C) AC SOFTWARE SP. Z O.O

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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <supla-common/proto.h>
#include <supla/events.h>
#include <supla/sensor/virtual_binary.h>
#include <supla/sensor/binary.h>
#include <simple_time.h>
#include <arduino_mock.h>

#include "../doubles/supla_io_mock.h"

class BinaryConfigStub : public Supla::Sensor::BinaryBase {
 public:
  bool getValue() override {
    return false;
  }
};

class BinaryActionHandlerMock : public Supla::ActionHandler {
 public:
  MOCK_METHOD(void, handleAction, (int, int), (override));
};

TEST(BinarySensorTests, VirtualBinaryValuesTest) {
  Supla::Channel::resetToDefaults();
  SimpleTime time;
  Supla::Sensor::VirtualBinary sensor;

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);

  sensor.setServerInvertLogic(true);

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);

  sensor.set();

  for (int i = 0; i < 50; ++i) {
    sensor.iterateAlways();
    time.advance(100);
  }

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);

  sensor.setServerInvertLogic(false);

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);
}

TEST(BinarySensorTests, VirtualBinaryTimeoutClearsLogicalState) {
  Supla::Channel::resetToDefaults();
  SimpleTime time;
  Supla::Sensor::VirtualBinary sensor;

  sensor.setTimeoutDs(25);
  sensor.onInit();
  sensor.set();

  for (int i = 0; i < 25; ++i) {
    time.advance(100);
    sensor.iterateAlways();
  }

  EXPECT_TRUE(sensor.getChannel()->getValueBool());

  time.advance(100);
  sensor.iterateAlways();

  EXPECT_FALSE(sensor.getChannel()->getValueBool());
  EXPECT_FALSE(sensor.getValue());
}

TEST(BinarySensorTests, VirtualBinaryTimeoutRespectsServerInvertLogic) {
  Supla::Channel::resetToDefaults();
  SimpleTime time;
  Supla::Sensor::VirtualBinary sensor;

  sensor.setTimeoutDs(25);
  sensor.setServerInvertLogic(true);
  sensor.onInit();
  sensor.clear();

  for (int i = 0; i < 25; ++i) {
    time.advance(100);
    sensor.iterateAlways();
  }

  EXPECT_TRUE(sensor.getChannel()->getValueBool());

  time.advance(100);
  sensor.iterateAlways();

  EXPECT_FALSE(sensor.getChannel()->getValueBool());
  EXPECT_TRUE(sensor.getValue());
}

TEST(BinarySensorTests, VirtualBinaryCanDisableConfiguredTimeout) {
  Supla::Channel::resetToDefaults();
  SimpleTime time;
  Supla::Sensor::VirtualBinary sensor;

  sensor.setTimeoutDs(25);
  sensor.setUseConfiguredTimeout(false);
  sensor.onInit();
  sensor.set();

  for (int i = 0; i < 26; ++i) {
    time.advance(100);
    sensor.iterateAlways();
  }

  EXPECT_TRUE(sensor.getChannel()->getValueBool());
  EXPECT_TRUE(sensor.getValue());
}

TEST(BinarySensorTests, VirtualBinaryInitDoesNotRunActions) {
  Supla::Channel::resetToDefaults();
  SimpleTime time;
  testing::StrictMock<BinaryActionHandlerMock> actionHandler;
  Supla::Sensor::VirtualBinary sensor;

  sensor.addAction(1, actionHandler, Supla::ON_TURN_ON);
  sensor.addAction(2, actionHandler, Supla::ON_TURN_OFF);
  sensor.addAction(3, actionHandler, Supla::ON_CHANGE);
  sensor.addAction(4, actionHandler, Supla::ON_SECONDARY_CHANNEL_CHANGE);
  sensor.set();

  sensor.onInit();

  EXPECT_TRUE(sensor.getChannel()->getValueBool());

  testing::InSequence sequence;
  EXPECT_CALL(actionHandler, handleAction(Supla::ON_TURN_OFF, 2));
  EXPECT_CALL(actionHandler, handleAction(Supla::ON_CHANGE, 3));
  EXPECT_CALL(actionHandler,
              handleAction(Supla::ON_SECONDARY_CHANNEL_CHANGE, 4));

  sensor.clear();
  time.advance(101);
  sensor.iterateAlways();

  EXPECT_FALSE(sensor.getChannel()->getValueBool());
}

TEST(BinarySensorTests, BinaryInitDoesNotRunActions) {
  Supla::Channel::resetToDefaults();
  DigitalInterfaceMock ioMock;
  SimpleTime time;
  testing::StrictMock<BinaryActionHandlerMock> actionHandler;

  int gpioValue = 1;
  EXPECT_CALL(ioMock, pinMode(1, INPUT)).WillOnce(::testing::Return());
  EXPECT_CALL(ioMock, digitalRead(1))
      .WillRepeatedly(::testing::ReturnPointee(&gpioValue));

  Supla::Sensor::Binary sensor(1);
  sensor.addAction(1, actionHandler, Supla::ON_TURN_ON);
  sensor.addAction(2, actionHandler, Supla::ON_TURN_OFF);
  sensor.addAction(3, actionHandler, Supla::ON_CHANGE);
  sensor.addAction(4, actionHandler, Supla::ON_SECONDARY_CHANNEL_CHANGE);

  sensor.onInit();

  EXPECT_TRUE(sensor.getChannel()->getValueBool());

  testing::InSequence sequence;
  EXPECT_CALL(actionHandler, handleAction(Supla::ON_TURN_OFF, 2));
  EXPECT_CALL(actionHandler, handleAction(Supla::ON_CHANGE, 3));
  EXPECT_CALL(actionHandler,
              handleAction(Supla::ON_SECONDARY_CHANNEL_CHANGE, 4));

  gpioValue = 0;
  time.advance(101);
  sensor.iterateAlways();

  EXPECT_FALSE(sensor.getChannel()->getValueBool());
}

TEST(BinarySensorTests, BinaryConfigStoresAlarmMutedInChannelConfig) {
  Supla::Channel::resetToDefaults();
  BinaryConfigStub sensor;

  ASSERT_TRUE(sensor.setAlarmMuted(2, false));

  TChannelConfig_BinarySensor channelConfig = {};
  int configSize = 0;

  sensor.fillChannelConfig(&channelConfig,
                           &configSize,
                           SUPLA_CONFIG_TYPE_DEFAULT);

  EXPECT_EQ(configSize, sizeof(TChannelConfig_BinarySensor));
  EXPECT_EQ(channelConfig.AlarmMuted, 2);
}

TEST(BinarySensorTests, BinaryConfigAppliesAlarmMutedFromServer) {
  Supla::Channel::resetToDefaults();
  BinaryConfigStub sensor;

  ASSERT_TRUE(sensor.setAlarmMuted(2, false));

  TSD_ChannelConfig newConfig = {};
  newConfig.ConfigSize = sizeof(TChannelConfig_BinarySensor);
  newConfig.ConfigType = SUPLA_CONFIG_TYPE_DEFAULT;

  auto serverConfig =
      reinterpret_cast<TChannelConfig_BinarySensor *>(newConfig.Config);

  serverConfig->AlarmMuted = 1;
  EXPECT_EQ(sensor.applyChannelConfig(&newConfig, false),
            Supla::ApplyConfigResult::Success);
  EXPECT_EQ(sensor.getAlarmMuted(), 1);

  serverConfig->AlarmMuted = 0;
  EXPECT_EQ(sensor.applyChannelConfig(&newConfig, false),
            Supla::ApplyConfigResult::SetChannelConfigNeeded);
  EXPECT_EQ(sensor.getAlarmMuted(), 1);
}

TEST(BinarySensorTests, BinaryValuesTest) {
  Supla::Channel::resetToDefaults();
  DigitalInterfaceMock ioMock;
  SimpleTime time;

  int gpio1Value = 0;
  EXPECT_CALL(ioMock, pinMode(1, INPUT)).WillOnce(::testing::Return());
  EXPECT_CALL(ioMock, digitalRead(1))
      .WillRepeatedly(::testing::ReturnPointee(&gpio1Value));

  Supla::Sensor::Binary sensor(1);

  sensor.onInit();

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);

  sensor.setServerInvertLogic(true);

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);

  gpio1Value = 1;

  for (int i = 0; i < 50; ++i) {
    sensor.iterateAlways();
    time.advance(100);
  }

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);

  sensor.setServerInvertLogic(false);

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);
}

TEST(BinarySensorTests, BinaryValuesWithLocalInvertTest) {
  Supla::Channel::resetToDefaults();
  DigitalInterfaceMock ioMock;
  SimpleTime time;

  int gpio1Value = 0;
  EXPECT_CALL(ioMock, pinMode(1, INPUT)).WillOnce(::testing::Return());
  EXPECT_CALL(ioMock, digitalRead(1))
      .WillRepeatedly(::testing::ReturnPointee(&gpio1Value));

  Supla::Sensor::Binary sensor(1, false, true);

  sensor.onInit();

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);

  sensor.setServerInvertLogic(true);

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);

  gpio1Value = 1;

  for (int i = 0; i < 50; ++i) {
    sensor.iterateAlways();
    time.advance(100);
  }

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);

  sensor.setServerInvertLogic(false);

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);
}

TEST(BinarySensorTests, BinaryIoPinConstructorUsesSeparateIoAndPolarity) {
  Supla::Channel::resetToDefaults();
  SuplaIoMock ioMock;
  SimpleTime time;

  int gpio1Value = 0;
  EXPECT_CALL(ioMock, customPinMode(0, 1, INPUT_PULLUP))
      .WillOnce(::testing::Return());
  EXPECT_CALL(ioMock, customDigitalRead(0, 1))
      .WillRepeatedly(::testing::ReturnPointee(&gpio1Value));

  Supla::Io::IoPin inputPin(1, &ioMock);
  inputPin.setPullUp(true);
  inputPin.setActiveHigh(false);

  Supla::Sensor::Binary sensor(inputPin);

  sensor.onInit();

  EXPECT_EQ(sensor.getValue(), true);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), true);

  gpio1Value = 1;

  for (int i = 0; i < 50; ++i) {
    sensor.iterateAlways();
    time.advance(100);
  }

  EXPECT_EQ(sensor.getValue(), false);
  EXPECT_EQ(sensor.getChannel()->getValueBool(), false);
}
