#ifdef SUPLA_ZIGBEE_GATEWAY
#ifndef CUSTOM_ZIGBEE_GATEWAY_H
#define CUSTOM_ZIGBEE_GATEWAY_H

#include <Supla/element.h>
#include <Supla/action_handler.h>

#include <ZigbeeGateway.h>
#include <Z2S_Devices_Table.h>
#include <Z2S_devices_database.h>

#include <vector>
#include <ArduinoJson.h>

#define GATEWAY_ENDPOINT_NUMBER 1
extern ZigbeeGateway zbGateway;

namespace Supla {

class SuplaZigbeeGateway : public Supla::Element, public Supla::ActionHandler {
 public:
  SuplaZigbeeGateway(const char* jsonDevices = "", int factoryResetButtonPin = -1);
  SuplaZigbeeGateway(const z2s_device_entity_t* devices, int factoryResetButtonPin = -1);
  ~SuplaZigbeeGateway();
  void onInit() override;
  void iterateAlways() override;
  void handleAction(int event, int action) override;

 private:
  std::vector<z2s_device_entity_t> Z2S_DEVICES;
  int factoryResetButtonPin;
  bool zbInit;

  zb_device_params_t* gateway_device;
  zb_device_params_t* joined_device;

  uint8_t counter = 0;
  uint8_t tuya_dp_data[10];

  char zbdModelName[32];
  char zbdManufName[32];

  enum addedActions
  {
    factoryReset,
  };

  void parseDevicesFromJson(const char* json);
  void loadDevicesFromProgMem(const z2s_device_entity_t* deviceList);
};
}  // namespace Supla

#endif  // CUSTOM_ZIGBEE_GATEWAY_H
#endif