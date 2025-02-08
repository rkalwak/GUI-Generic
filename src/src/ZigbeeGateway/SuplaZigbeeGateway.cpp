
#ifdef SUPLA_ZIGBEE_GATEWAY
#include "SuplaZigbeeGateway.h"

#include <SuplaDevice.h>
#include <ZigbeeGateway.h>

#include <supla/events.h>
#include <supla/control/button.h>
#include <supla/storage/storage.h>

#include <esp_coexist.h>

ZigbeeGateway zbGateway = ZigbeeGateway(GATEWAY_ENDPOINT_NUMBER);

namespace Supla {
SuplaZigbeeGateway::SuplaZigbeeGateway(const char* jsonDevices, int factoryResetButtonPin)
    : factoryResetButtonPin(factoryResetButtonPin), zbInit(true) {
  memset(zbdModelName, 0, sizeof(zbdModelName));
  memset(zbdManufName, 0, sizeof(zbdManufName));

  parseDevicesFromJson(jsonDevices);
  loadDevicesFromProgMem(Z2S_DEVICES_LIST);
}

SuplaZigbeeGateway::SuplaZigbeeGateway(const z2s_device_entity_t* devices, int factoryResetButtonPin)
    : factoryResetButtonPin(factoryResetButtonPin), zbInit(true) {
  memset(zbdModelName, 0, sizeof(zbdModelName));
  memset(zbdManufName, 0, sizeof(zbdManufName));

  loadDevicesFromProgMem(devices);
}

SuplaZigbeeGateway::~SuplaZigbeeGateway() {
}

void SuplaZigbeeGateway::onInit() {
  if (factoryResetButtonPin >= 0) {
    auto buttonCfgRelay = new Supla::Control::Button(factoryResetButtonPin, true, true);
    buttonCfgRelay->setMulticlickTime(400);
    buttonCfgRelay->addAction(factoryReset, this, Supla::ON_CLICK_10);
  }

  Supla::Storage::Init();
  Serial.print("Rozmiar tablicy z2s_devices_table: ");
  Serial.println(sizeof(z2s_devices_table));

  Z2S_loadDevicesTable();
  Z2S_initSuplaChannels();

  new Supla::Clock;

  //  Zigbee Gateway notifications
  zbGateway.onTemperatureReceive(Z2S_onTemperatureReceive);
  zbGateway.onHumidityReceive(Z2S_onHumidityReceive);
  zbGateway.onOnOffReceive(Z2S_onOnOffReceive);
  zbGateway.onRMSVoltageReceive(Z2S_onRMSVoltageReceive);
  zbGateway.onRMSCurrentReceive(Z2S_onRMSCurrentReceive);
  zbGateway.onRMSActivePowerReceive(Z2S_onRMSActivePowerReceive);
  zbGateway.onBatteryPercentageReceive(Z2S_onBatteryPercentageReceive);
  zbGateway.onOnOffCustomCmdReceive(Z2S_onOnOffCustomCmdReceive);

  zbGateway.onCmdCustomClusterReceive(Z2S_onCmdCustomClusterReceive);

  zbGateway.onIASzoneStatusChangeNotification(Z2S_onIASzoneStatusChangeNotification);

  zbGateway.onBoundDevice(Z2S_onBoundDevice);
  zbGateway.onBTCBoundDevice(Z2S_onBTCBoundDevice);

  zbGateway.setManufacturerAndModel("Supla", "Z2SGateway");
  zbGateway.allowMultipleBinding(true);

  Zigbee.addEndpoint(&zbGateway);

  // Open network for 180 seconds after boot
  Zigbee.setRebootOpenNetwork(180);
}

void SuplaZigbeeGateway::iterateAlways() {
  if (zbInit && SuplaDevice.getCurrentStatus() == STATUS_REGISTERED_AND_READY) {
    Serial.println("zbInit");

    esp_coex_wifi_i154_enable();

    if (!Zigbee.begin(ZIGBEE_COORDINATOR)) {
      Serial.println("Zigbee failed to start!");
      Serial.println("Rebooting...");
      ESP.restart();
    }
    zbInit = false;
  }
  delay(100);

  if (zbGateway.isNewDeviceJoined()) {
    zbGateway.clearNewDeviceJoined();
    zbGateway.printJoinedDevices();

    while (!zbGateway.getJoinedDevices().empty()) {
      joined_device = zbGateway.getLastJoinedDevice();
      zbGateway.zbQueryDeviceBasicCluster(joined_device);

      uint16_t devices_list_table_size = sizeof(Z2S_DEVICES_LIST) / sizeof(Z2S_DEVICES_LIST[0]);
      uint16_t devices_desc_table_size = sizeof(Z2S_DEVICES_DESC) / sizeof(Z2S_DEVICES_DESC[0]);
      bool device_recognized = false;

      for (int i = 0; i < devices_list_table_size; i++) {
        if ((strcmp(zbGateway.getQueryBasicClusterData()->zcl_model_name, Z2S_DEVICES_LIST[i].model_name) == 0) &&
            (strcmp(zbGateway.getQueryBasicClusterData()->zcl_manufacturer_name, Z2S_DEVICES_LIST[i].manufacturer_name) == 0)) {
          log_i("LIST matched %s::%s, entry # %d, endpoints # %d, endpoints 0x%x::0x%x,0x%x::0x%x,0x%x::0x%x,0x%x::0x%x",
                Z2S_DEVICES_LIST[i].manufacturer_name, Z2S_DEVICES_LIST[i].model_name, i, Z2S_DEVICES_LIST[i].z2s_device_endpoints_count,
                Z2S_DEVICES_LIST[i].z2s_device_endpoints[0].endpoint_id, Z2S_DEVICES_LIST[i].z2s_device_endpoints[0].z2s_device_desc_id,
                Z2S_DEVICES_LIST[i].z2s_device_endpoints[1].endpoint_id, Z2S_DEVICES_LIST[i].z2s_device_endpoints[1].z2s_device_desc_id,
                Z2S_DEVICES_LIST[i].z2s_device_endpoints[2].endpoint_id, Z2S_DEVICES_LIST[i].z2s_device_endpoints[2].z2s_device_desc_id,
                Z2S_DEVICES_LIST[i].z2s_device_endpoints[3].endpoint_id, Z2S_DEVICES_LIST[i].z2s_device_endpoints[3].z2s_device_desc_id);

          for (int j = 0; j < Z2S_DEVICES_LIST[i].z2s_device_endpoints_count; j++) {
            uint8_t endpoint_id = (Z2S_DEVICES_LIST[i].z2s_device_endpoints_count == 1) ? 1 : Z2S_DEVICES_LIST[i].z2s_device_endpoints[j].endpoint_id;

            uint32_t z2s_device_desc_id = (Z2S_DEVICES_LIST[i].z2s_device_endpoints_count == 1)
                                              ? Z2S_DEVICES_LIST[i].z2s_device_desc_id
                                              : Z2S_DEVICES_LIST[i].z2s_device_endpoints[j].z2s_device_desc_id;

            for (int k = 0; k < devices_desc_table_size; k++) {
              if (z2s_device_desc_id == Z2S_DEVICES_DESC[k].z2s_device_desc_id) {
                log_i("DESC matched 0x%x, %d, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, endpoint 0x%x ", Z2S_DEVICES_DESC[k].z2s_device_desc_id,
                      Z2S_DEVICES_DESC[k].z2s_device_clusters_count, Z2S_DEVICES_DESC[k].z2s_device_clusters[0],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[1], Z2S_DEVICES_DESC[k].z2s_device_clusters[2],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[3], Z2S_DEVICES_DESC[k].z2s_device_clusters[4],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[5], Z2S_DEVICES_DESC[k].z2s_device_clusters[6],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[7], endpoint_id);

                device_recognized = true;

                joined_device->endpoint = endpoint_id;
                joined_device->model_id = Z2S_DEVICES_DESC[k].z2s_device_desc_id;

                if (joined_device->model_id == Z2S_DEVICE_DESC_SWITCH_4X3) {
                  Z2S_addZ2SDevice(joined_device, 0);
                  Z2S_addZ2SDevice(joined_device, 1);
                  Z2S_addZ2SDevice(joined_device, 2);
                }
                else
                  Z2S_addZ2SDevice(joined_device, -1);

                // case Z2S_DEVICE_DESC_ON_OFF: {
                // zbGateway.sendAttributeRead(joined_device, 0x0006,0x8001, true);
                // zbGateway.sendAttributeRead(joined_device, 0x0006,0x8002, true);
                // zbGateway.sendAttributeRead(joined_device, 0x0006,0x5000, true);
                // zbGateway.sendAttributeRead(joined_device, 0x0006,0x8001, true);
                // write_mask = 0x01;
                // zbGateway.sendAttributeWrite(joined_device, 0x0006, 0x8004, ESP_ZB_ZCL_ATTR_TYPE_8BIT_ENUM, 1, &write_mask );
                // zbGateway.sendAttributeRead(joined_device, 0x0006,0x8004, true);
                //} break;

                for (int m = 0; m < Z2S_DEVICES_DESC[k].z2s_device_clusters_count; m++)
                  zbGateway.bindDeviceCluster(joined_device, Z2S_DEVICES_DESC[k].z2s_device_clusters[m]);

                switch (joined_device->model_id) {
                  case 0x0000:
                    break;
                  case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR:
                  case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR_1: {
                  } break;
                  case Z2S_DEVICE_DESC_IAS_ZONE_SENSOR: {
                    // log_i("Trying to configure cluster reporting on device (0x%x), endpoint (0x%x)", joined_device->short_addr,
                    // joined_device->endpoint); zbGateway.sendAttributeRead(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
                    // ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONETYPE_ID, true); zbGateway.readClusterReportCmd(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
                    // ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONETYPE_ID, true); zbGateway.readClusterReportCmd(joined_device, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                    // 0x0021, true); zbGateway.setClusterReporting(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
                    //  ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID, ESP_ZB_ZCL_ATTR_TYPE_16BITMAP, 0, 60, 1, true);
                    // zbGateway.setClusterReporting(joined_device, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                    //  0x0021, //ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
                    //                         ESP_ZB_ZCL_ATTR_TYPE_U8, 0, 4*60*60, 1, true);
                  } break;
                }
              }
              else
                log_i("DESC checking 0x%x, %d, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, endpoint %d ", Z2S_DEVICES_DESC[k].z2s_device_desc_id,
                      Z2S_DEVICES_DESC[k].z2s_device_clusters_count, Z2S_DEVICES_DESC[k].z2s_device_clusters[0],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[1], Z2S_DEVICES_DESC[k].z2s_device_clusters[2],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[3], Z2S_DEVICES_DESC[k].z2s_device_clusters[4],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[5], Z2S_DEVICES_DESC[k].z2s_device_clusters[6],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[7], endpoint_id);
            }
          }
          // here we can configure reporting and restart ESP32
          switch (joined_device->model_id) {
            case 0x0000:
              break;

            case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR:
            case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR_1: {
            } break;
            case Z2S_DEVICE_DESC_IAS_ZONE_SENSOR: {
              /*log_i("Trying to configure cluster reporting on device (0x%x), endpoint (0x%x)", joined_device->short_addr, joined_device->endpoint);
              zbGateway.sendAttributeRead(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE, ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONETYPE_ID, true);
              zbGateway.sendAttributeRead(joined_device, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG, 0x0021, true);
              zbGateway.setClusterReporting(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
                                            ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID, ESP_ZB_ZCL_ATTR_TYPE_16BITMAP, 0, 60, 1, true);
              zbGateway.setClusterReporting(joined_device, ESP_ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
                                            0x0021, //ESP_ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
                                            ESP_ZB_ZCL_ATTR_TYPE_U8, 0, 4*60*60, 1, true);

              */
              // esp_zb_ieee_addr_t addr;
              // esp_zb_get_long_address(addr);
              // zbGateway.sendAttributeWrite(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE, ESP_ZB_ZCL_ATTR_IAS_ZONE_IAS_CIE_ADDRESS_ID,
              //     ESP_ZB_ZCL_ATTR_TYPE_U64,sizeof(esp_zb_ieee_addr_t),&addr);
              // delay(200);
              // zbGateway.sendIASzoneEnrollResponseCmd(joined_device, ESP_ZB_ZCL_IAS_ZONE_ENROLL_RESPONSE_CODE_SUCCESS, 120);
              // delay(200);
              // zbGateway.sendAttributeRead(joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE, ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONETYPE_ID, true);

            } break;
          }
          // zbGateway.setClusterReporting( joined_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
          //                        ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID, ESP_ZB_ZCL_ATTR_TYPE_16BITMAP, 30, 300, 1);

          SuplaDevice.scheduleSoftRestart(5000);
        }
        else
          log_i("LIST checking %s::%s, entry # %d", Z2S_DEVICES_LIST[i].manufacturer_name, Z2S_DEVICES_LIST[i].model_name, i);
      }
      if (!device_recognized)
        log_d("Unknown model %s::%s, no binding is possible", zbGateway.getQueryBasicClusterData()->zcl_manufacturer_name,
              zbGateway.getQueryBasicClusterData()->zcl_model_name);
    }
  }
}

void SuplaZigbeeGateway::handleAction(int event, int action) {
  Serial.printf("handleAction called with event: %d, action: %d\n", event, action);

  if (event == Supla::ON_CLICK_10 && action == factoryReset) {
    Serial.println("Resetting Zigbee to factory settings, reboot.");
    Zigbee.factoryReset();
    Zigbee.openNetwork(180);
  }
}

void SuplaZigbeeGateway::parseDevicesFromJson(const char* json) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& doc = jsonBuffer.parseObject(json);

  if (!doc.success()) {
    Serial.println("Deserialization failed");
    return;
  }

  JsonArray& devicesArray = doc["devices"];
  if (devicesArray.size() == 0) {
    Serial.println("Error: 'devices' is null or not found");
    return;
  }

  for (JsonArray& device : devicesArray) {
    z2s_device_entity_t newDevice = {};

    const char* manufacturer_name = device[0];
    const char* model_name = device[1];

    if (manufacturer_name && model_name) {
      strncpy(newDevice.manufacturer_name, manufacturer_name, sizeof(newDevice.manufacturer_name) - 1);
      newDevice.manufacturer_name[sizeof(newDevice.manufacturer_name) - 1] = '\0';

      strncpy(newDevice.model_name, model_name, sizeof(newDevice.model_name) - 1);
      newDevice.model_name[sizeof(newDevice.model_name) - 1] = '\0';
    } else {
      Serial.println("Error: Missing manufacturer_name or model_name");
      continue;
    }

    const char* device_desc_id = device[2];
    if (device_desc_id) {
      newDevice.z2s_device_desc_id = strtoul(device_desc_id, NULL, 16);
    }

    newDevice.z2s_device_endpoints_count = device[3].as<int>();

    Serial.printf("Added JSON: %s - %s (ID: 0x%X, Endpoints: %d)\n", 
                  newDevice.manufacturer_name, newDevice.model_name,
                  newDevice.z2s_device_desc_id, newDevice.z2s_device_endpoints_count);
  }
}

// void SuplaZigbeeGateway::parseDevicesFromJson(const char* json) {
//   DynamicJsonBuffer jsonBuffer;
//   JsonObject& doc = jsonBuffer.parseObject(json);

//   if (!doc.success()) {
//     Serial.println("Deserialization failed");
//     return;
//   }

//   JsonArray& devicesArray = doc["devices"];
//   if (devicesArray.size() == 0) {
//     Serial.println("Error: 'devices' is null or not found");
//     return;
//   }

//   for (JsonObject& device : devicesArray) {
//     z2s_device_entity_t newDevice;

//     strncpy(newDevice.manufacturer_name, device["manuf"], sizeof(newDevice.manufacturer_name) - 1);
//     newDevice.manufacturer_name[sizeof(newDevice.manufacturer_name) - 1] = '\0';

//     strncpy(newDevice.model_name, device["model"], sizeof(newDevice.model_name) - 1);
//     newDevice.model_name[sizeof(newDevice.model_name) - 1] = '\0';

//     newDevice.z2s_device_desc_id = (uint32_t)strtol(device["desc_id"], NULL, 16);
//     newDevice.z2s_device_endpoints_count = device["endpoints"];

//     Z2S_DEVICES.push_back(newDevice);

//     Serial.printf("Added JSON: %s - %s (ID: %d, Endpoints: %d)\n", newDevice.manufacturer_name, newDevice.model_name, newDevice.z2s_device_desc_id,
//                   newDevice.z2s_device_endpoints_count);
//   }
// }

void SuplaZigbeeGateway::loadDevicesFromProgMem(const z2s_device_entity_t* deviceList) {
  size_t deviceCount = sizeof(Z2S_DEVICES_LIST) / sizeof(Z2S_DEVICES_LIST[0]);
  for (size_t i = 0; i < deviceCount; ++i) {
    z2s_device_entity_t device;
    memcpy_P(&device, &deviceList[i], sizeof(z2s_device_entity_t));
    Z2S_DEVICES.push_back(device);

    Serial.printf("Added PROG: %s - %s (ID: %d, Endpoints: %d)\n", device.manufacturer_name, device.model_name, device.z2s_device_desc_id,
                  device.z2s_device_endpoints_count);
  }
}
}  // namespace Supla
#endif