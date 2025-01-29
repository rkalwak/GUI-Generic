
#ifdef SUPLA_ZIGBEE_GATEWAY
#include "SuplaZigbeeGateway.h"

#include <ZigbeeGateway.h>

#include <supla/events.h>
#include <supla/control/button.h>
#include <supla/storage/storage.h>

#include <esp_coexist.h>

ZigbeeGateway zbGateway = ZigbeeGateway(GATEWAY_ENDPOINT_NUMBER);

namespace Supla {
SuplaZigbeeGateway::SuplaZigbeeGateway(const char* jsonDevices)
    : zbInit(true), startTime(0), printTime(0), zbInitDelay(5000), gatewayDevice(nullptr), joinedDevice(nullptr) {
  memset(zbdModelName, 0, sizeof(zbdModelName));
  memset(zbdManufName, 0, sizeof(zbdManufName));

  parseDevicesFromJson(jsonDevices);
  loadDevicesFromProgMem(Z2S_DEVICES_LIST);
}

SuplaZigbeeGateway::SuplaZigbeeGateway(const z2s_device_entity_t* devices)
    : zbInit(true), startTime(0), printTime(0), zbInitDelay(5000), gatewayDevice(nullptr), joinedDevice(nullptr) {
  memset(zbdModelName, 0, sizeof(zbdModelName));
  memset(zbdManufName, 0, sizeof(zbdManufName));

  loadDevicesFromProgMem(devices);
}
SuplaZigbeeGateway::SuplaZigbeeGateway(const char* jsonDevices, int factoryResetButtonPin)
    : factoryResetButtonPin(factoryResetButtonPin),
      zbInit(true),
      startTime(0),
      printTime(0),
      zbInitDelay(5000),
      gatewayDevice(nullptr),
      joinedDevice(nullptr) {
  memset(zbdModelName, 0, sizeof(zbdModelName));
  memset(zbdManufName, 0, sizeof(zbdManufName));

  parseDevicesFromJson(jsonDevices);
  loadDevicesFromProgMem(Z2S_DEVICES_LIST);
}

SuplaZigbeeGateway::SuplaZigbeeGateway(const z2s_device_entity_t* devices, int factoryResetButtonPin)
    : factoryResetButtonPin(factoryResetButtonPin),
      zbInit(true),
      startTime(0),
      printTime(0),
      zbInitDelay(5000),
      gatewayDevice(nullptr),
      joinedDevice(nullptr) {
  memset(zbdModelName, 0, sizeof(zbdModelName));
  memset(zbdManufName, 0, sizeof(zbdManufName));

  loadDevicesFromProgMem(devices);
}

SuplaZigbeeGateway::~SuplaZigbeeGateway() {
}

void SuplaZigbeeGateway::onInit() {
  auto buttonCfgRelay = new Supla::Control::Button(factoryResetButtonPin, true, true);
  buttonCfgRelay->addAction(factoryReset, this, Supla::ON_CLICK_10);
  // Initialize Supla storage
  Supla::Storage::Init();

  Z2S_loadDevicesTable();
  Z2S_initSuplaChannels();

  //  Zigbee Gateway notifications
  zbGateway.onTemperatureReceive(Z2S_onTemperatureReceive);
  zbGateway.onHumidityReceive(Z2S_onHumidityReceive);
  zbGateway.onOnOffReceive(Z2S_onOnOffReceive);
  zbGateway.onRMSVoltageReceive(Z2S_onRMSVoltageReceive);
  zbGateway.onRMSCurrentReceive(Z2S_onRMSCurrentReceive);
  zbGateway.onRMSActivePowerReceive(Z2S_onRMSActivePowerReceive);
  zbGateway.onBatteryPercentageReceive(Z2S_onBatteryPercentageReceive);

  zbGateway.onCmdCustomClusterReceive(Z2S_onCmdCustomClusterReceive);

  zbGateway.onIASzoneStatusChangeNotification(Z2S_onIASzoneStatusChangeNotification);

  zbGateway.onBoundDevice(Z2S_onBoundDevice);
  zbGateway.onBTCBoundDevice(Z2S_onBTCBoundDevice);

  zbGateway.setManufacturerAndModel("Supla", "Z2SGateway");
  zbGateway.allowMultipleBinding(true);

  Zigbee.addEndpoint(&zbGateway);

  // Open network for 180 seconds after boot
  Zigbee.setRebootOpenNetwork(180);

  startTime = millis();
}

void SuplaZigbeeGateway::iterateAlways() {
  // if (SuplaDevice.getCurrentStatus() == Supla::DEVICE_MODE_CONFIG) {
  //   return;
  // }

  if (millis() - printTime > 10000) {
    if (zbGateway.getGatewayDevices().size() > 0) {
      if (esp_zb_is_started() && esp_zb_lock_acquire(portMAX_DELAY)) {
        zb_device_params_t* gt_device = zbGateway.getGatewayDevices().front();
        // log_i("short address before 0x%x",gt_device->short_addr);
        gt_device->short_addr = esp_zb_address_short_by_ieee(gt_device->ieee_addr);
        // log_i("short address after 0x%x",gt_device->short_addr);
        if (counter == 0) {
          tuya_dp_data[0] = 0x00;
          tuya_dp_data[1] = 0x03;
          tuya_dp_data[2] = 0x65;
          tuya_dp_data[3] = 0x01;
          tuya_dp_data[4] = 0x00;
          tuya_dp_data[5] = 0x01;
          tuya_dp_data[6] = 0x01;

          // zbGateway.sendCustomClusterCmd(gt_device, TUYA_PRIVATE_CLUSTER_EF00, 0x00, 7, tuya_dp_data);
        }
        // if (counter == 1) {
        tuya_dp_data[0] = 0x00;
        tuya_dp_data[1] = 0x03;
        tuya_dp_data[2] = 0x66;
        tuya_dp_data[3] = 0x02;
        tuya_dp_data[4] = 0x00;
        tuya_dp_data[5] = 0x04;
        tuya_dp_data[6] = 0x00;
        tuya_dp_data[7] = 0x00;
        tuya_dp_data[8] = 0x00;
        tuya_dp_data[9] = 0x00;
        // zbGateway.sendCustomClusterCmd(gt_device, TUYA_PRIVATE_CLUSTER_EF00, 0x00, 10, tuya_dp_data);
        //}
        if (counter == 2) {
          tuya_dp_data[0] = 0x00;
          tuya_dp_data[1] = 0x03;
          tuya_dp_data[2] = 0x65;
          tuya_dp_data[3] = 0x01;
          tuya_dp_data[4] = 0x00;
          tuya_dp_data[5] = 0x01;
          tuya_dp_data[6] = 0x00;

          // zbGateway.sendCustomClusterCmd(gt_device, TUYA_PRIVATE_CLUSTER_EF00, 0x00, 7, tuya_dp_data);
        }
        if (counter == 3) {
          tuya_dp_data[0] = 0x00;
          tuya_dp_data[1] = 0x03;
          tuya_dp_data[2] = 0x6C;
          tuya_dp_data[3] = 0x01;
          tuya_dp_data[4] = 0x00;
          tuya_dp_data[5] = 0x01;
          tuya_dp_data[6] = 0x01;

          // zbGateway.sendCustomClusterCmd(gt_device, TUYA_PRIVATE_CLUSTER_EF00, 0x00, 7, tuya_dp_data);
        }
        if (counter == 4) {
          tuya_dp_data[0] = 0x00;
          tuya_dp_data[1] = 0x03;
          tuya_dp_data[2] = 0x6C;
          tuya_dp_data[3] = 0x01;
          tuya_dp_data[4] = 0x00;
          tuya_dp_data[5] = 0x01;
          tuya_dp_data[6] = 0x02;

          // zbGateway.sendCustomClusterCmd(gt_device, TUYA_PRIVATE_CLUSTER_EF00, 0x00, 7, tuya_dp_data);
        }
        counter++;
        if (counter > 4)
          counter = 0;
        // zbGateway.sendAttributeWrite(gt_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE, ESP_ZB_ZCL_ATTR_IAS_ZONE_IAS_CIE_ADDRESS_ID,
        //                          ESP_ZB_ZCL_ATTR_TYPE_U64,8, gt_device->ieee_addr);
        // zbGateway.sendIASzoneEnrollResponseCmd(gt_device, ESP_ZB_ZCL_IAS_ZONE_ENROLL_RESPONSE_CODE_SUCCESS, 120);
        // zbGateway.sendAttributeRead(gt_device, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE, ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID);
        // zbGateway.setClusterReporting(gt_device->ieee_addr, gt_device->endpoint, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
        //                          ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID, ESP_ZB_ZCL_ATTR_TYPE_16BITMAP, 0, 10, 1);
      }
      esp_zb_lock_release();
      printTime = millis();
    }
  }

  if (zbInit && millis() - zbInitDelay > startTime) {
    esp_coex_wifi_i154_enable();
    if (!Zigbee.begin(ZIGBEE_COORDINATOR)) {
      Serial.println("Zigbee initialization failed! Restarting...");
      ESP.restart();
    }
    zbInit = false;
  }

  if (zbGateway.isNewDeviceJoined()) {
    zbGateway.clearNewDeviceJoined();
    zbGateway.printJoinedDevices();

    while (!zbGateway.getJoinedDevices().empty()) {
      joinedDevice = zbGateway.getLastJoinedDevice();

      strcpy(zbdModelName, zbGateway.readManufacturer(joinedDevice->endpoint, joinedDevice->short_addr, joinedDevice->ieee_addr));
      log_i("manufacturer %s ", zbdManufName);
      strcpy(zbdModelName, zbGateway.readModel(joinedDevice->endpoint, joinedDevice->short_addr, joinedDevice->ieee_addr));
      log_i("model %s ", zbdModelName);

      uint16_t devices_list_table_size = sizeof(Z2S_DEVICES) / sizeof(Z2S_DEVICES[0]);
      uint16_t devices_desc_table_size = sizeof(Z2S_DEVICES_DESC) / sizeof(Z2S_DEVICES_DESC[0]);
      bool device_recognized = false;

      for (int i = 0; i < devices_list_table_size; i++) {
        if ((strcmp(zbdModelName, Z2S_DEVICES[i].model_name) == 0) && (strcmp(zbdManufName, Z2S_DEVICES[i].manufacturer_name) == 0)) {
          log_i("LIST matched %s::%s, entry # %d, endpoints # %d, endpoints 0x%x::0x%x,0x%x::0x%x,0x%x::0x%x,0x%x::0x%x",
                Z2S_DEVICES[i].manufacturer_name, Z2S_DEVICES[i].model_name, i, Z2S_DEVICES[i].z2s_device_endpoints_count,
                Z2S_DEVICES[i].z2s_device_endpoints[0].endpoint_id, Z2S_DEVICES[i].z2s_device_endpoints[0].z2s_device_desc_id,
                Z2S_DEVICES[i].z2s_device_endpoints[1].endpoint_id, Z2S_DEVICES[i].z2s_device_endpoints[1].z2s_device_desc_id,
                Z2S_DEVICES[i].z2s_device_endpoints[2].endpoint_id, Z2S_DEVICES[i].z2s_device_endpoints[2].z2s_device_desc_id,
                Z2S_DEVICES[i].z2s_device_endpoints[3].endpoint_id, Z2S_DEVICES[i].z2s_device_endpoints[3].z2s_device_desc_id);

          for (int j = 0; j < Z2S_DEVICES[i].z2s_device_endpoints_count; j++) {
            uint8_t endpoint_id = (Z2S_DEVICES[i].z2s_device_endpoints_count == 1) ? 1 : Z2S_DEVICES[i].z2s_device_endpoints[j].endpoint_id;

            uint32_t z2s_device_desc_id = (Z2S_DEVICES[i].z2s_device_endpoints_count == 1)
                                              ? Z2S_DEVICES[i].z2s_device_desc_id
                                              : Z2S_DEVICES[i].z2s_device_endpoints[j].z2s_device_desc_id;

            for (int k = 0; k < devices_desc_table_size; k++) {
              if (z2s_device_desc_id == Z2S_DEVICES_DESC[k].z2s_device_desc_id) {
                log_i("DESC matched 0x%x, %d, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, endpoint 0x%x ", Z2S_DEVICES_DESC[k].z2s_device_desc_id,
                      Z2S_DEVICES_DESC[k].z2s_device_clusters_count, Z2S_DEVICES_DESC[k].z2s_device_clusters[0],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[1], Z2S_DEVICES_DESC[k].z2s_device_clusters[2],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[3], Z2S_DEVICES_DESC[k].z2s_device_clusters[4],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[5], Z2S_DEVICES_DESC[k].z2s_device_clusters[6],
                      Z2S_DEVICES_DESC[k].z2s_device_clusters[7], endpoint_id);

                device_recognized = true;

                joinedDevice->endpoint = endpoint_id;
                joinedDevice->model_id = Z2S_DEVICES_DESC[k].z2s_device_desc_id;

                Z2S_addZ2SDevice(joinedDevice);

                for (int m = 0; m < Z2S_DEVICES_DESC[k].z2s_device_clusters_count; m++)
                  zbGateway.bindDeviceCluster(joinedDevice, Z2S_DEVICES_DESC[k].z2s_device_clusters[m]);
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
          switch (joinedDevice->model_id) {
            case 0x0000:
              break;

            case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR:
            case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR_1: {
            } break;
            case Z2S_DEVICE_DESC_IAS_ZONE_SENSOR: {
              // log_i("Trying to configure cluster reporting on device (0x%x), endpoint (0x%x)", joined_device->short_addr, joined_device->endpoint);
              // zbGateway.setClusterReporting(joined_device->short_addr, joined_device->endpoint, ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE,
              // ESP_ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID, ESP_ZB_ZCL_ATTR_TYPE_16BITMAP, 30, 300, 1);
            } break;
          }
          SuplaDevice.scheduleSoftRestart(5000);
        }
        else
          log_i("LIST checking %s::%s, entry # %d", Z2S_DEVICES[i].manufacturer_name, Z2S_DEVICES[i].model_name, i);
      }
      if (!device_recognized)
        log_d("Unknown model %s::%s, no binding is possible", zbdManufName, zbdModelName);
    }
  }
}

void SuplaZigbeeGateway::handleAction(int event, int action) {
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

  for (JsonObject& device : devicesArray) {
    z2s_device_entity_t newDevice;

    strncpy(newDevice.manufacturer_name, device["manufacturer_name"], sizeof(newDevice.manufacturer_name) - 1);
    newDevice.manufacturer_name[sizeof(newDevice.manufacturer_name) - 1] = '\0';

    strncpy(newDevice.model_name, device["model_name"], sizeof(newDevice.model_name) - 1);
    newDevice.model_name[sizeof(newDevice.model_name) - 1] = '\0';

    newDevice.z2s_device_desc_id = device["z2s_device_desc_id"];
    newDevice.z2s_device_endpoints_count = device["z2s_device_endpoints_count"];

    Z2S_DEVICES.push_back(newDevice);

    Serial.printf("Added JSON: %s - %s (ID: %d, Endpoints: %d)\n", newDevice.manufacturer_name, newDevice.model_name, newDevice.z2s_device_desc_id,
                  newDevice.z2s_device_endpoints_count);
  }
}

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

};  // namespace Supla
#endif