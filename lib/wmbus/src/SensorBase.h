#ifndef _SensorBase_h
#define _SensorBase_h

#include <vector>
#include <string>
#include <stdint.h>
namespace Supla
{
    namespace Sensor
    {
        class SensorBase
        {
        public:
    SensorBase(std::string meter_id, std::string type, std::string property_to_send, std::vector<unsigned char> key)
        : _key(key), _type(type), _meter_id(meter_id), _property_to_send(property_to_send) {};
        
            std::string get_type() { return this->_type; };
            std::string get_meter_id() { return this->_meter_id; };
            std::string get_property_to_send() { return this->_property_to_send; };
            std::vector<unsigned char> get_key() { return this->_key; };
            void virtual setNewValue(int value){};
            void virtual iterateAlways(){};

        protected:
            SensorBase();
            std::vector<unsigned char> _key;
            std::string _type;
            std::string _meter_id;
            std::string _property_to_send;
        };

    };
};
#endif