#ifndef _WmbusMeter_h
#define _WmbusMeter_h
#include <Arduino.h>

#include "../lib/wmbus/rf_mbus.hpp"
#include "../lib/wmbus/Drivers/driver.h"
#include "../lib/wmbus/wmbus_utils.hpp"
#include "../lib/wmbus/utils.hpp"
#include "../lib/wmbus/SensorBase.h"

#include <stdint.h>
#include <string>
#include <vector>

#include <supla/element.h>

namespace Supla
{
	namespace Sensor
	{
		class WmbusMeter : public Element
		{
		public:
			WmbusMeter(uint8_t mosi = 23, uint8_t miso = 19, uint8_t clk = 18, uint8_t cs = 5, uint8_t gdo0 = 4, uint8_t gdo2 = 2);
			WmbusMeter(int a);

			void iterateAlways();
			std::map<std::string, Driver *> drivers_{};
			std::map<std::string, SensorBase *> sensors_{};
			void add_driver(Driver *driver);
			void add_sensor(SensorBase *sensor);
			bool decrypt_telegram(std::vector<unsigned char> &telegram, std::vector<unsigned char> key);
			float parse_frame(std::vector<unsigned char> &frame);

		private:
			float readValue = 0.0;
			int packetLength = 192;
			rf_mbus receiver;
		};
	};
};
#endif