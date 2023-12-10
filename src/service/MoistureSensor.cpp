#include "service/MoistureSensor.hpp"
#include "Constants.hpp"
#include <Arduino.h>
#include <iostream>

using namespace AutoWater; 
using namespace std; 

MoistureSensor::MoistureSensor(uint8_t sensorPin=16) {
    MoistureSensor(
        sensorPin, 
        Constants::DEFAULT_MOISTURE_MIN, 
        Constants::DEFAULT_MOISTURE_MAX, 
        Constants::DEFAULT_MOISTURE_OUTPUT_MIN, 
        Constants::DEFAULT_MOISTURE_OUTPUT_MAX
    ); 
}

MoistureSensor::MoistureSensor(uint8_t sensorPin, long moistureOffset, long moistureMax, long moistureRmin, long moistureRmax) {
    this->sensorPin = sensorPin; 
    this->moistureOffset = moistureOffset; 
    this->moistureMax = moistureMax; 
    this->moistureRmin = moistureRmin; 
    this->moistureRmax = moistureRmax;
}

void MoistureSensor::init(long moistureOffset, long moistureMax, long moistureOutMin, long moistureOutMax) {
    pinMode(this->sensorPin, INPUT); 
    this->moistureOffset = moistureOffset; 
    this->moistureMax = moistureMax; 
    this->moistureRmin = moistureOutMin; 
    this->moistureRmax = moistureOutMax;
}

int MoistureSensor::readRawValue() {
    return analogRead(this->sensorPin); 
}

void MoistureSensor::setRawValue(int rawValue) {
    this->rawValue = rawValue; 
}

float MoistureSensor::readValue() {
    return map(this->rawValue, this->moistureOffset, this->moistureMax, this->moistureRmin, this->moistureRmax); 
}