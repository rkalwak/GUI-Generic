#ifndef _LANGUAGE_EN_S_H_
#define _LANGUAGE_EN_S_H_

#define S_LANG "en"

#define S_SETTING_FOR             "Setting for"
#define S_SETTING_WIFI_SSID       "Setting WIFI"
#define S_WIFI_SSID               "Name WIFI"
#define S_WIFI_PASS               "Password"
#define S_HOST_NAME               "Module name"
#define S_SETTING_SUPLA           "Setting SUPLA"
#define S_SUPLA_SERVER            "Server address"
#define S_SUPLA_EMAIL             "Email"
#define S_SETTING_ADMIN           "Administrator settings"
#define S_LOGIN                   "Login"
#define S_LOGIN_PASS              "Password"
#define S_ROLLERSHUTTERS          "Roller shuter"
#define S_SAVE                    "Save"
#define S_DEVICE_SETTINGS         "Device settings"
#define S_TOOLS                   "Tools"
#define S_SAVE_CONFIGURATION      "Save configuration"
#define S_LOAD_CONFIGURATION      "Load configuration"
#define S_RESET_CONFIGURATION     "Reset device settings"
#define S_RESTORE_FACTORY_SETTING "Restore factory settings"
#define S_UPDATE                  "Update"
#define S_RESTART                 "Restart"
#define S_RETURN                  "Return"

#define S_TEMPLATE_BOARD                                       "Template board"
#define S_TYPE                                                 "Type"
#define S_RELAYS                                               "RELAYS"
#define S_BUTTONS                                              "BUTTONS"
#define S_SENSORS_1WIRE                                        "SENSORS 1Wire"
#define S_SENSORS_I2C                                          "SENSORS i2c"
#define S_SENSORS_SPI                                          "SENSORS SPI"
#define S_SENSORS_OTHER                                        "SENSORS OTHER"
#define S_LED_BUTTON_CFG                                       "LED, BUTTON CONFIG"
#define S_CFG_MODE                                             "CFG mode"
#define S_QUANTITY                                             "QUANTITY"
#define S_GPIO_SETTINGS_FOR_RELAYS                             "GPIO settings for relays"
#define S_RELAY                                                "RELAYS"
#define S_RELAY_NR_SETTINGS                                    "Settings relay nr. "
#define S_STATE_CONTROL                                        "State control"
#define S_REACTION_AFTER_RESET                                 "Reaction after reset"
#define S_GPIO_SETTINGS_FOR_BUTTONS                            "GPIO settings for buttons"
#define S_BUTTON                                               "BUTTON"
#define S_BUTTON_NR_SETTINGS                                   "Setting button nr. "
#define S_REACTION_TO                                          "Reaction to"
#define S_RELAY_CONTROL                                        "Relay control"
#define S_ACTION                                               "Action"
#define S_GPIO_SETTINGS_FOR_LIMIT_SWITCH                       "GPIO settings for limit switch"
#define S_LIMIT_SWITCH                                         "LIMIT SWITCH"
#define S_GPIO_SETTINGS_FOR                                    "GPIO settings for"
#define S_FOUND                                                "Found"
#define S_NO_SENSORS_CONNECTED                                 "No sensor conected"
#define S_SAVE_FOUND                                           "Save found"
#define S_TEMPERATURE                                          "Temperature"
#define S_NAME                                                 "Name"
#define S_ALTITUDE_ABOVE_SEA_LEVEL                             "Altitude"
#define S_GPIO_SETTINGS_FOR_CONFIG                             "GPIO settings for config"
#define S_SOFTWARE_UPDATE                                      "Software update"
#define S_DATA_SAVED                                           "Data saved"
#define S_RESTART_MODULE                                       "Restart module"
#define S_DATA_ERASED_RESTART_DEVICE                           "Data erased - restart module"
#define S_WRITE_ERROR_UNABLE_TO_READ_FILE_FS_PARTITION_MISSING "Write error - unable to read file FS. Partition missing"
#define S_DATA_SAVED_RESTART_MODULE                            "Data saved - restart module"
#define S_WRITE_ERROR_BAD_DATA                                 "Write error - bad data"
#define S_SETTINGS_FOR                                         "Setting for"
#define S_ACTION_TRIGGER                                       "Action trigger"
#define S_ADDITIONAL                                           "Additionnel"

//#### SuplaConfigESP.cpp ####
#define S_STATUS_ALREADY_INITIALIZED     "Already initiated"
#define S_STATUS_INVALID_GUID            "Invalid GUID"
#define S_STATUS_UNKNOWN_SERVER_ADDRESS  "Unknown server address"
#define S_STATUS_UNKNOWN_LOCATION_ID     "Unknown ID"
#define S_STATUS_INITIALIZED             "Initiated"
#define S_STATUS_CHANNEL_LIMIT_EXCEEDED  "Channel limit exceeded"
#define S_STATUS_SERVER_DISCONNECTED     "Disconnected"
#define S_STATUS_REGISTER_IN_PROGRESS    "Registration is pending"
#define S_STATUS_PROTOCOL_VERSION_ERROR  "Protocol version error"
#define S_STATUS_BAD_CREDENTIALS         "Bad credentials"
#define S_STATUS_TEMPORARILY_UNAVAILABLE "Temporarily unavailable"
#define S_STATUS_LOCATION_CONFLICT       "Location conflict"
#define S_STATUS_CHANNEL_CONFLICT        "Channel conflict"
#define S_STATUS_REGISTERED_AND_READY    "Registered and ready"
#define S_STATUS_DEVICE_IS_DISABLED      "The device is disconnected"
#define S_STATUS_LOCATION_IS_DISABLED    "The location is disabled"
#define S_STATUS_DEVICE_LIMIT_EXCEEDED   "Device limit exceeded"
#define S_STATUS_REGISTRATION_DISABLED   "Device registration INACTIVE"
#define S_STATUS_MISSING_CREDENTIALS     "Missing email address"
#define S_STATUS_INVALID_AUTHKEY         "Missing AuthKey"
#define S_STATUS_NO_LOCATION_AVAILABLE   "No location available!"
#define S_STATUS_UNKNOWN_ERROR           "Unknown registration error"
#define S_STATUS_NETWORK_DISCONNECTED    "No connection to network"

//#### SuplaCommonPROGMEM.h ####
#define S_OFF                           "OFF"
#define S_ON                            "ON"
#define S_TOGGLE                        "TOGGLE"
#define S_LOW                           "LOW"
#define S_HIGH                          "HIGH"
#define S_POSITION_MEMORY               "POSITION MEMORY"
#define S_REACTION_ON_PRESS             "ON PRESS"
#define S_REACTION_ON_RELEASE           "ON RELEASE"
#define S_REACTION_ON_CHANGE            "ON CHANGE"
#define S_REACTION_ON_HOLD              "ON HOLD"
#define S_CFG_10_PRESSES                "10 ON PRESSES"
#define S_5SEK_HOLD                     "5 SEC HOLD"
#define S_NORMAL                        "NORMAL"
#define S_SLOW                          "SLOW"
#define S_MANUALLY                      "MANUALLY"
#define S_ON_CH_VAL_OFF_HEATING         "ON > channel value > OFF - heating"
#define S_ON_CH_VAL_OFF_COOLING         "ON < channel value < OFF - cooling"
#define S_ON_2CH_VAL_OFF_HUMIDIFICATION "ON > 2nd channel value > OFF - humidification"
#define S_ON_2CH_VAL_OFF_DRYING         "ON < 2nd channel value < OFF - drying"

//#### SuplaWebServer.cpp ####
#define S_LIMIT_SWITCHES "LIMIT SWITCHES"
#define S_CORRECTION     "CORRECTION FOR SENSORS"

//#### SuplaTemplateBoard.h ####
#define S_ABSENT "ABSENT"

//#### SuplaWebPageSensor.cpp ####
#define S_IMPULSE_COUNTER                  "Impulse counter"
#define S_IMPULSE_COUNTER_DEBOUNCE_TIMEOUT "Debounce timeout"
#define S_IMPULSE_COUNTER_RAISING_EDGE     "Raising edge"
#define S_IMPULSE_COUNTER_PULL_UP          "Pull up"
#define S_IMPULSE_COUNTER_CHANGE_VALUE     "Change value"
#define S_SCREEN_TIME                      "Screen [s]"
#define S_OLED_BUTTON                      "OLED button"
#define S_SCREEN                           "Screen"
#define S_BACKLIGHT_S                      "Backlight [s]"
#define S_BACKLIGHT_PERCENT                "Brightness [%]"
#define S_ADDRESS                          "Address"

//#### SuplaWebPageUpload.cpp ####
#define S_GENERATE_GUID_AND_KEY "Generate GUID & AUTHKEY"
#define S_UPLOAD                "Upload"

//#### SuplaWebPageControl.cpp ####
#define S_SETTINGS_FOR_BUTTONS "Settings for buttons"
#define S_REVERSE_LOGIC        "Reverse logic"
#define S_INTERNAL_PULL_UP     "Internal pull-up"

//#### SuplaWebPageOther.cpp ####
#define S_CALIBRATION             "Calibration"
#define S_CALIBRATION_SETTINGS    "Calibration settings"
#define S_BULB_POWER_W            "Bulb power [W]"
#define S_VOLTAGE_V               "Voltage [V]"
#define S_DEPTH_CM                "Depth [cm]"
#define S_SENSOR_READING_DISTANCE "sensor reading distance"
#define S_ELECTRIC_PHASE          "1/3phases"
#define S_OPTIONAL                "(Optional)"
#define S_STATUS_LED              "Status LED"

//#### SuplaWebPageRelay.cpp ####
#define S_RELAY_ACTIVATION_STATUS "Relay activation status"
#define S_STATE                   "State"
#define S_MESSAGE                 "Message"
#define S_DIRECT_LINKS            "Direct links"
#define S_CONDITIONING            "Conditioning"
#define S_SENSOR                  "Sensor"
#define S_CONDITION               "Condition"
#define S_SWITCH_ON_VALUE         "switch-on value"
#define S_SWITCH_OFF_VALUE        "switch-off value"
#define S_SETTINGS_FOR_RELAYS     "Settings for relays"

//#### SuplaHTTPUpdateServer.cpp ####
#define S_FLASH_MEMORY_SIZE        "Flash Memory Size"
#define S_SKETCH_MEMORY_SIZE       "Sketch memory size"
#define S_SKETCH_LOADED_SIZE       "Sketch Loaded Size"
#define S_SKETCH_UPLOAD_MAX_SIZE   "Sketch Upload Max Size"
#define S_UPDATE_FIRMWARE          "Update Firmware"
#define S_UPDATE_SUCCESS_REBOOTING "Update Success! Rebooting..."
#define S_WARNING                  "WARNING"
#define S_ONLY_2_STEP_OTA          "only use 2-step OTA update. Use"

//#### SuplaOled.cpp ####
#define S_CONFIGURATION_MODE "Configuration mode"
#define S_AP_NAME            "AP name"
#define S_ERROR              "error"

//#### SuplaWebCorrection.cpp ####
#define S_CORRECTION_FOR_CH "Correction for channels"
#define S_CH_CORRECTION     "Channel correction:"

#ifdef SUPLA_RF_BRIDGE
#define S_CODES       "codes"
#define S_NO          "no"
#define S_READ        "Read"
#define S_TRANSMITTER "Transmitter"
#define S_RECEIVER    "Receiver"
#endif

#define S_TEMP_HYGR         "Temperature + Humidity"
#define S_PRESS             "Pressure"
#define S_ELECTRICITY_METER "Electric energy meter"

#endif  // _LANGUAGE_EN_S_H_
