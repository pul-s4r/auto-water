#ifndef _MOISTURE_SENSOR_HPP
#define _MOISTURE_SENSOR_HPP

#include <Arduino.h>

namespace AutoWater {
    class MoistureSensor {
    public:
        MoistureSensor(uint8_t sensorPin); 
        MoistureSensor(uint8_t sensorPin, long moistureOffset, long moistureMax, long moistureRmin, long moistureRmax); 
        void init(long moistureOffset, long moistureMax, long moistureOutMin, long moistureOutMax);
        int readRawValue(); 
        void setRawValue(int rawValue); 
        float readValue();
    private: 
        uint8_t sensorPin; 
        int rawValue; 
        long moistureOffset; 
        long moistureMax; 
        long moistureRmin; 
        long moistureRmax; 
    };
}

#endif