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
    this->init(this->sensorPin, moistureOffset, moistureMax, moistureOutMin, moistureOutMax); 
}

void MoistureSensor::init(uint8_t sensorPin, long moistureOffset, long moistureMax, long moistureOutMin, long moistureOutMax) {
    this->sensorPin = sensorPin; 
    pinMode(this->sensorPin, INPUT); 
    this->moistureOffset = moistureOffset; 
    this->moistureMax = moistureMax; 
    this->moistureRmin = moistureOutMin; 
    this->moistureRmax = moistureOutMax;
}

int MoistureSensor::readRawValue() {
    Serial.println("Sensor pin: " + String(this->sensorPin)); 
    this->rawValue = analogRead(this->sensorPin); 
    return this->rawValue; 
}

void MoistureSensor::setRawValue(int rawValue) {
    this->rawValue = rawValue; 
}

float MoistureSensor::readValue() {
    return map(this->rawValue, this->moistureOffset, this->moistureMax, this->moistureRmin, this->moistureRmax); 
}