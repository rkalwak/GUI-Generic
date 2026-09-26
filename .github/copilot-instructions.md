# Copilot instructions for adding a new sensor

This project follows a repeatable integration pattern. When adding a sensor, do not patch only one file. The change should be wired through the same layers as the existing sensors.

## Sensor addition checklist

### 1) Add the sensor definition and capability flags
- Update the feature flags in [platformio.ini](../platformio.ini) if the sensor is a new optional library or build block.
- Add the compile-time macro for the sensor and any related option, for example:
  - `SUPLA_<SENSOR>_KPOP`
  - `P_SUPLA_<SENSOR>_KPOP_Address`
  - `P_SUPLA_<SENSOR>_KPOP_Gain` if the hardware supports user-selectable gain/range.
- If the sensor is a board feature that must appear in generated config metadata, check [builder.json](../builder.json).

### 2) Add the wrapper class in the sensor layer
- Create the main class in [src/src/sensor](../src/src/sensor), following the existing pattern used by SPS30 / ADS1115 / INA* wrappers.
- Keep one controller object for the hardware and one child object per measurement channel if the chip exposes multiple channels.
- Use the project’s existing sensor base types such as `Element`, `GeneralPurposeMeasurement`, or an equivalent pattern already used in the repository.
- Ensure the constructor receives all runtime-configurable values, especially address or gain, and never relies only on compile-time defaults.

### 3) Hook it into runtime creation
- Update [src/main.cpp](../src/main.cpp).
- Add the sensor instantiation block under the matching feature guard.
- Read values from `ConfigManager` instead of hardcoded constants when possible.
- Always pass the runtime-selected address/gain/other parameters into the sensor object.

### 4) Add config keys and defaults
- Update [src/SuplaConfigManager.h](../src/SuplaConfigManager.h) and [src/SuplaConfigManager.cpp](../src/SuplaConfigManager.cpp).
- If the sensor needs a persistent setting, add a dedicated key in the enum and register it in `SuplaConfigManager`.
- Define a default value for the config key and keep it aligned with the actual allowed ranges.
- For new sensor selection, use the correct `KEY_ACTIVE_SENSOR` or `KEY_ACTIVE_SENSOR_2` slot and preserve the project’s indexing pattern.

### 5) Add startup defaults for generated boards
- Update [src/SuplaDeviceGUI.cpp](../src/SuplaDeviceGUI.cpp).
- If a new sensor should be enabled in preconfigured templates or static build defaults, set the corresponding `KEY_ACTIVE_SENSOR` / `KEY_ACTIVE_SENSOR_2` element here.
- If a compile-time build parameter exists, map it into the config manager so the generated runtime config matches the compile-time configuration.

### 6) Add UI controls for config and save logic
- Update [src/SuplaWebPageSensorI2c.h](../src/SuplaWebPageSensorI2c.h) and [src/SuplaWebPageSensorI2c.cpp](../src/SuplaWebPageSensorI2c.cpp) for I2C sensors.
- Add:
  - the sensor selection field,
  - any address selector,
  - any gain/range selector,
  - any additional per-sensor settings.
- Follow the existing pattern: render the control in `handleSensorI2c()`, then save it in `handleSensorI2cSave()`.
- Persist values using `ConfigManager->set(...)` or `ConfigManager->setElement(...)` depending on whether the value is scalar or a per-element array value.

### 7) Add UI option labels and selectable values
- Update [src/SuplaCommonPROGMEM.h](../src/SuplaCommonPROGMEM.h).
- Add the PROGMEM arrays used by the web UI for the sensor address, gain, or other enum values.
- Keep option names short and consistent with existing sensor naming.

### 8) Validate runtime behavior
- Ensure the chosen option is not only saved, but also read back and applied at runtime.
- Verify the same values that are displayed in the UI are the values that are actually used when constructing the sensor.
- Rebuild the target after each sensor implementation.

## Minimal implementation template

Use this sequence for the next sensor:

1. Add build flag and dependency in [platformio.ini](../platformio.ini).
2. Add sensor wrapper in [src/src/sensor](../src/src/sensor).
3. Add runtime instantiation in [src/main.cpp](../src/main.cpp).
4. Add config key in [src/SuplaConfigManager.h](../src/SuplaConfigManager.h).
5. Register default value in [src/SuplaConfigManager.cpp](../src/SuplaConfigManager.cpp).
6. Set default/generated config in [src/SuplaDeviceGUI.cpp](../src/SuplaDeviceGUI.cpp).
7. Add UI selector and save handler in [src/SuplaWebPageSensorI2c.h](../src/SuplaWebPageSensorI2c.h) and [src/SuplaWebPageSensorI2c.cpp](../src/SuplaWebPageSensorI2c.cpp).
8. Add PROGMEM options in [src/SuplaCommonPROGMEM.h](../src/SuplaCommonPROGMEM.h).
9. Build and verify with PlatformIO.

## Important project-specific rule

For this codebase, a sensor is not complete if it is only created in code. It must also be:
- configurable in the UI,
- stored in config,
- restored on startup,
- and passed to the sensor constructor with the chosen runtime parameters.

This is the main pattern the project expects.

## Example of the required pattern

The ADS1115 integration is the reference for a multi-channel I2C ADC with configurable gain:
- wrapper: [src/src/sensor/ADS1115.h](../src/src/sensor/ADS1115.h)
- runtime creation: [src/main.cpp](../src/main.cpp)
- config: [src/SuplaConfigManager.h](../src/SuplaConfigManager.h), [src/SuplaConfigManager.cpp](../src/SuplaConfigManager.cpp)
- UI: [src/SuplaWebPageSensorI2c.h](../src/SuplaWebPageSensorI2c.h), [src/SuplaWebPageSensorI2c.cpp](../src/SuplaWebPageSensorI2c.cpp)
- options: [src/SuplaCommonPROGMEM.h](../src/SuplaCommonPROGMEM.h)

Follow that structure when adding the next sensor.
