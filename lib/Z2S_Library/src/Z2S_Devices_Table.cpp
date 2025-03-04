

#include "Z2S_Devices_Table.h"
#include "Z2S_Device_IASzone.h"
#include "Z2S_Device_Tyua_Hvac.h"

#include <SuplaDevice.h>
#include <supla/sensor/virtual_therm_hygro_meter.h>
#include <supla/sensor/one_phase_electricity_meter.h>
#include <supla/control/virtual_relay.h>

#include <Z2S_control/Z2S_virtual_relay.h>
#include <Z2S_sensor/Z2S_OnePhaseElectricityMeter.h>

extern ZigbeeGateway zbGateway;

z2s_device_params_t z2s_devices_table[Z2S_CHANNELMAXCOUNT];

uint32_t Z2S_getDevicesTableSize() {
  uint32_t _z2s_devices_table_size;
  if (Supla::Storage::ConfigInstance()->getUInt32(Z2S_DEVICES_TABLE_SIZE, &_z2s_devices_table_size))
    return _z2s_devices_table_size;
  else
    return 0;
}

uint8_t Z2S_findFirstFreeDevicesTableSlot() {

  for (uint8_t devices_counter = 0; devices_counter < Z2S_CHANNELMAXCOUNT; devices_counter++) 
      if (!z2s_devices_table[devices_counter].valid_record)
        return devices_counter;
  return 0xFF;
  
}

void Z2S_printDevicesTableSlots() {

  for (uint8_t devices_counter = 0; devices_counter < Z2S_CHANNELMAXCOUNT; devices_counter++) 
    if (z2s_devices_table[devices_counter].valid_record)
      log_i("valid %d, ieee addr %s, model_id 0x%x, endpoint 0x%x, cluster 0x%x, channel %d, channel type %d",
        z2s_devices_table[devices_counter].valid_record,
        z2s_devices_table[devices_counter].ieee_addr,
        z2s_devices_table[devices_counter].model_id,
        z2s_devices_table[devices_counter].endpoint,
        z2s_devices_table[devices_counter].cluster_id,
        z2s_devices_table[devices_counter].Supla_channel,
        z2s_devices_table[devices_counter].Supla_channel_type);  
}


int16_t Z2S_findChannelNumberSlot(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, int32_t channel_type, int8_t sub_id) {

  log_i("Z2S_findChannelNumberSlot 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x, channel type 0x%x", 
        ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0], 
        endpoint, channel_type);
  
  for (uint8_t devices_counter = 0; devices_counter < Z2S_CHANNELMAXCOUNT; devices_counter++) {
      if (z2s_devices_table[devices_counter].valid_record)
        if ((memcmp(z2s_devices_table[devices_counter].ieee_addr, ieee_addr,8) == 0) && 
        (z2s_devices_table[devices_counter].endpoint == endpoint) &&
        ((channel_type < 0) || (z2s_devices_table[devices_counter].Supla_channel_type == channel_type)) &&
        ((sub_id < 0) || (z2s_devices_table[devices_counter].sub_id == sub_id))) { 
        //&& (z2s_devices_table[devices_counter].cluster_id = cluster)) {
            return devices_counter;
        }

  }  
  return -1;
}

void Z2S_fillDevicesTableSlot(zb_device_params_t *device, uint8_t slot, uint8_t channel, int32_t channel_type, int8_t sub_id,
                              char *name, uint32_t func) {

  z2s_devices_table[slot].valid_record = true;
  memcpy(z2s_devices_table[slot].ieee_addr,device->ieee_addr,8);
  z2s_devices_table[slot].model_id = device->model_id;
  z2s_devices_table[slot].endpoint = device->endpoint;
  z2s_devices_table[slot].cluster_id = device->cluster_id;
  z2s_devices_table[slot].Supla_channel = channel;
  z2s_devices_table[slot].Supla_channel_type = channel_type;
  z2s_devices_table[slot].sub_id = sub_id; 
  if (name) strcpy(z2s_devices_table[slot].Supla_channel_name, name);
  z2s_devices_table[slot].Supla_channel_func = func;
  
  Z2S_saveDevicesTable();
  Z2S_printDevicesTableSlots();
}

bool Z2S_loadDevicesTable() {

  log_i("before get devices table");
  uint32_t z2s_devices_table_size =  Z2S_getDevicesTableSize(); //3584

  if (z2s_devices_table_size == 0) {

      log_i(" No devices table found, writing empty one with size %d", sizeof(z2s_devices_table));
      
      memset(z2s_devices_table,0,sizeof(z2s_devices_table));
      
      if (!Supla::Storage::ConfigInstance()->setBlob(Z2S_DEVICES_TABLE, (char *)z2s_devices_table, sizeof(z2s_devices_table))) {
        log_i ("Devices table write failed!");
        return false;
      }
      else { 
        if (Supla::Storage::ConfigInstance()->setUInt32(Z2S_DEVICES_TABLE_SIZE, sizeof(z2s_devices_table))) {
          Supla::Storage::ConfigInstance()->commit();
          return true;
        }
        else { 
          log_i ("Devices table size write failed!");
          return false;
        }
      }
  } else
  {
    if (z2s_devices_table_size != sizeof(z2s_devices_table)) {
      
      log_i("Devices table size mismatch %d <> %d", z2s_devices_table_size, sizeof(z2s_devices_table));
      return false;
    }
    else {
        if (!Supla::Storage::ConfigInstance()->getBlob(Z2S_DEVICES_TABLE, (char *)z2s_devices_table, sizeof(z2s_devices_table))) {
          log_i ("Devices table load failed!");
          return false;
        } else {
          log_i ("Devices table load success!");
          return true;
        }
    }
  }
}

bool Z2S_saveDevicesTable() {
  Serial.println("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  if (!Supla::Storage::ConfigInstance()->setBlob(Z2S_DEVICES_TABLE, (char *)z2s_devices_table, sizeof(z2s_devices_table))) {
    log_i ("Devices table write failed!");
    return false;
  }
  else { 
    if (Supla::Storage::ConfigInstance()->setUInt32(Z2S_DEVICES_TABLE_SIZE, sizeof(z2s_devices_table)))
      return true;
    else { 
      log_i ("Devices table size write failed!");
      return false;
    }
  }
  Serial.println("tttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt");
  Serial.println(sizeof(z2s_devices_table));
  Supla::Storage::ConfigInstance()->commit();
}

void Z2S_initSuplaChannels(){

  log_i ("initSuplaChannels starting");
  for (uint8_t devices_counter = 0; devices_counter < Z2S_CHANNELMAXCOUNT; devices_counter++) {
      if (z2s_devices_table[devices_counter].valid_record) {

        zb_device_params_t device;
        device.endpoint = z2s_devices_table[devices_counter].endpoint;
        device.cluster_id = z2s_devices_table[devices_counter].cluster_id;
        memcpy(device.ieee_addr, z2s_devices_table[devices_counter].ieee_addr,8);
        device.short_addr = z2s_devices_table[devices_counter].short_addr;

        switch (z2s_devices_table[devices_counter].Supla_channel_type) {
          case SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR: {
            
            auto Supla_VirtualThermHygroMeter = new Supla::Sensor::VirtualThermHygroMeter();
            Supla_VirtualThermHygroMeter->getChannel()->setChannelNumber(z2s_devices_table[devices_counter].Supla_channel);
          } break;
          case SUPLA_CHANNELTYPE_BINARYSENSOR: {
            
            initZ2SDeviceIASzone(z2s_devices_table[devices_counter].Supla_channel); 
            
          } break;
          case SUPLA_CHANNELTYPE_RELAY: {
            auto Supla_Z2S_VirtualRelay = new Supla::Control::Z2S_VirtualRelay(&zbGateway,z2s_devices_table[devices_counter].ieee_addr );
            Supla_Z2S_VirtualRelay->getChannel()->setChannelNumber(z2s_devices_table[devices_counter].Supla_channel);
          } break;
          case SUPLA_CHANNELTYPE_ACTIONTRIGGER: {
            auto Supla_VirtualRelay = new Supla::Control::VirtualRelay();
            Supla_VirtualRelay->setInitialCaption(z2s_devices_table[devices_counter].Supla_channel_name);
            Supla_VirtualRelay->setDefaultFunction(z2s_devices_table[devices_counter].Supla_channel_func);
            Supla_VirtualRelay->getChannel()->setChannelNumber(z2s_devices_table[devices_counter].Supla_channel);
          } break;
          case SUPLA_CHANNELTYPE_ELECTRICITY_METER: {
            auto Supla_Z2S_OnePhaseElectricityMeter = new Supla::Sensor::OnePhaseElectricityMeter();
            Supla_Z2S_OnePhaseElectricityMeter->getChannel()->setChannelNumber(z2s_devices_table[devices_counter].Supla_channel);
          } break;
          case SUPLA_CHANNELTYPE_HVAC: {

            initZ2SDeviceTyuaHvac(&zbGateway, &device, z2s_devices_table[devices_counter].Supla_channel);
          } break;
          default: {
            log_i("Can't create channel for %d channel type", z2s_devices_table[devices_counter].Supla_channel_type);
          } break;
        }
      }
  }  
}

void Z2S_onTemperatureReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, float temperature) {

  log_i("onTemperatureReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR) {

        auto Supla_VirtualThermHygroMeter = reinterpret_cast<Supla::Sensor::VirtualThermHygroMeter *>(element);
        Supla_VirtualThermHygroMeter->setTemp(temperature);
    }
  }
}

void Z2S_onHumidityReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, float humidity) {

  log_i("onHumidityReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR) {

        auto Supla_VirtualThermHygroMeter = reinterpret_cast<Supla::Sensor::VirtualThermHygroMeter *>(element);
        Supla_VirtualThermHygroMeter->setHumi(humidity);
    }
  }
}

void Z2S_onOnOffReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, bool state) {

  log_i("onOnOffReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_RELAY, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_RELAY) {

        auto Supla_Z2S_VirtualRelay = reinterpret_cast<Supla::Control::Z2S_VirtualRelay *>(element);
        Supla_Z2S_VirtualRelay->Z2S_setOnOff(state); 
    }
  }
}

void Z2S_onRMSVoltageReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, uint16_t voltage) {

  log_i("onRMSVoltageReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_ELECTRICITY_METER, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_ELECTRICITY_METER) {

        auto Supla_OnePhaseElectricityMeter = reinterpret_cast<Supla::Sensor::OnePhaseElectricityMeter *>(element);
        Supla_OnePhaseElectricityMeter->setVoltage(0, voltage * 100);
    }
  }
}

void Z2S_onRMSCurrentReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, uint16_t current) {

  log_i("onRMSCurrentReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_ELECTRICITY_METER, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_ELECTRICITY_METER) {

        auto Supla_OnePhaseElectricityMeter = reinterpret_cast<Supla::Sensor::OnePhaseElectricityMeter *>(element);
        Supla_OnePhaseElectricityMeter->setCurrent(0, current * 1000);
    }
  }
}

void Z2S_onRMSActivePowerReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, uint16_t active_power) {

  log_i("onRMSVoltageReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_ELECTRICITY_METER, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_ELECTRICITY_METER) {

        auto Supla_OnePhaseElectricityMeter = reinterpret_cast<Supla::Sensor::OnePhaseElectricityMeter *>(element);
        Supla_OnePhaseElectricityMeter->setPowerActive(0, active_power * 100000);
    }
  }
}

void Z2S_onBatteryPercentageReceive(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, uint8_t battery_remaining) {

  log_i("onBatteryPercentageReceive 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, endpoint 0x%x", ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4], ieee_addr[3],
   ieee_addr[2], ieee_addr[1], ieee_addr[0], endpoint);
  int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, -1, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
  {
    auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
    if (true) { //element != nullptr && element->getChannel()->isBatteryPowered()) {

        element->getChannel()->setBatteryLevel(battery_remaining);
    }
  }
}

void Z2S_onIASzoneStatusChangeNotification(esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, int iaszone_status) {
  
int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_BINARYSENSOR, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
    msgZ2SDeviceIASzone(z2s_devices_table[channel_number_slot].Supla_channel, iaszone_status);
}

void Z2S_onOnOffCustomCmdReceive( esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint8_t command_id, uint8_t command_data) {
  
  log_i("Z2S_onOnOffCustomCmdReceive command id 0x%x, command data 0x%x", command_id, command_data);
  if ((command_id == 0xFD) || (command_id == 0xFC)) {

    int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, 
                                                            SUPLA_CHANNELTYPE_ACTIONTRIGGER, command_data);
    if (channel_number_slot < 0)
      log_i("No channel found for address %s", ieee_addr);
    else {
      log_i("z2s_devices_table[channel_number_slot].Supla_channel 0x%x", z2s_devices_table[channel_number_slot].Supla_channel);
      auto element = Supla::Element::getElementByChannelNumber(z2s_devices_table[channel_number_slot].Supla_channel);
      if (element) log_i("element->getChannel()->getChannelType() 0x%x", element->getChannel()->getChannelType());
      else log_i("element not found");
      if (element) { //(element != nullptr && element->getChannel()->getChannelType() == SUPLA_CHANNELTYPE_ACTIONTRIGGER) {
        log_i("trying to toggle");
      auto Supla_VirtualRelay = reinterpret_cast<Supla::Control::VirtualRelay *>(element);
      Supla_VirtualRelay->toggle();
      }
    }
  }
}


void Z2S_onCmdCustomClusterReceive( esp_zb_ieee_addr_t ieee_addr, uint16_t endpoint, uint16_t cluster, uint8_t command_id,
                                    uint16_t payload_size, uint8_t *payload) {
  
int16_t channel_number_slot = Z2S_findChannelNumberSlot(ieee_addr, endpoint, cluster, SUPLA_CHANNELTYPE_HVAC, -1);
  if (channel_number_slot < 0)
    log_i("No channel found for address %s", ieee_addr);
  else
    msgZ2SDeviceTyuaHvac(z2s_devices_table[channel_number_slot].Supla_channel, cluster, command_id, payload_size, payload);
}

void Z2S_onBTCBoundDevice(zb_device_params_t *device) {

  
  log_i("BTC bound device 0x%x on endpoint 0x%x cluster id 0x%x", device->short_addr, device->endpoint, device->cluster_id );
  //zbGateway.zbQueryDeviceBasicCluster(device);
}


void Z2S_onBoundDevice(zb_device_params_t *device, bool last_cluster) {
  
}

void Z2S_addZ2SDevice(zb_device_params_t *device, int8_t sub_id) {
  
  
  Z2S_printDevicesTableSlots();

  int16_t channel_number_slot = Z2S_findChannelNumberSlot(device->ieee_addr, device->endpoint, device->cluster_id, -1, sub_id);
  
  if (channel_number_slot < 0) {
    log_i("No channel found for address %s, adding new one", device->ieee_addr);
    
    uint8_t first_free_slot = Z2S_findFirstFreeDevicesTableSlot();
    
    if (first_free_slot == 0xFF) {
        log_i("Devices table full");
        return;
    }
    log_i("model id %d, first free slot %d", device->model_id, first_free_slot);
    
    switch (device->model_id) {
      case 0x0000: break;
      
      case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR:
      case Z2S_DEVICE_DESC_TEMPHUMIDITY_SENSOR_1: {
        auto Supla_VirtualThermHygroMeter = new Supla::Sensor::VirtualThermHygroMeter();
        Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_VirtualThermHygroMeter->getChannelNumber(), SUPLA_CHANNELTYPE_HUMIDITYANDTEMPSENSOR, -1);
      } break;
      case Z2S_DEVICE_DESC_IAS_ZONE_SENSOR: {
        addZ2SDeviceIASzone(device, first_free_slot);
        //auto Supla_VirtualBinary = new Supla::Sensor::VirtualBinary();
        //Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_VirtualBinary->getChannelNumber(), SUPLA_CHANNELTYPE_BINARYSENSOR); 
      } break;
      case Z2S_DEVICE_DESC_RELAY:
      case Z2S_DEVICE_DESC_RELAY_1: {
        auto Supla_Z2S_VirtualRelay = new Supla::Control::Z2S_VirtualRelay(&zbGateway,device->ieee_addr);
        Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_Z2S_VirtualRelay->getChannelNumber(), SUPLA_CHANNELTYPE_RELAY,-1); 
      } break;
      case Z2S_DEVICE_DESC_ON_OFF:
      case Z2S_DEVICE_DESC_ON_OFF_1: {
        auto Supla_Z2S_VirtualRelay = new Supla::Control::VirtualRelay();
        Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_Z2S_VirtualRelay->getChannelNumber(), SUPLA_CHANNELTYPE_ACTIONTRIGGER, sub_id); 
      } break;
      case Z2S_DEVICE_DESC_SWITCH_4X3: {
        auto Supla_Z2S_VirtualRelay = new Supla::Control::VirtualRelay();
        char button_name_function[30];
        char button_function[][15] = {"pressed", "double pressed","held"};
        sprintf(button_name_function, "button #%d %s", device->endpoint, button_function[sub_id]);
        
        Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_Z2S_VirtualRelay->getChannelNumber(), SUPLA_CHANNELTYPE_ACTIONTRIGGER, sub_id,
        button_name_function, SUPLA_CHANNELFNC_POWERSWITCH); 
      } break;
      case Z2S_DEVICE_DESC_RELAY_ELECTRICITY_METER: {
        auto Supla_Z2S_VirtualRelay = new Supla::Control::Z2S_VirtualRelay(&zbGateway,device->ieee_addr);
        Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_Z2S_VirtualRelay->getChannelNumber(), SUPLA_CHANNELTYPE_RELAY,-1); 
        first_free_slot = Z2S_findFirstFreeDevicesTableSlot();
        if (first_free_slot == 0xFF) {
          log_i("Devices table full");
          return;
        }
        auto Supla_Z2S_OnePhaseElectricityMeter = new Supla::Sensor::OnePhaseElectricityMeter();
        Z2S_fillDevicesTableSlot(device, first_free_slot, Supla_Z2S_OnePhaseElectricityMeter->getChannelNumber(), 
                                SUPLA_CHANNELTYPE_ELECTRICITY_METER, -1);

      } break;
      case Z2S_DEVICE_TUYA_HVAC: {
        addZ2SDeviceTyuaHvac(&zbGateway, device, first_free_slot);
      } break;
      default : {
        log_i("Device (0x%x), endpoint (0x%x), model (0x%x) unknown", device->short_addr, device->endpoint, device->model_id);
      } break;
    }
  } else
    log_i("Device (0x%x), endpoint (0x%x) already in z2s_devices_table (index 0x%x)", device->short_addr, device->endpoint, channel_number_slot);   
}
