#include <Arduino.h>

#include <DHT.h>
#include "Constants.hpp"
#include "service/MoistureSensor.hpp"

using namespace AutoWater; 

#define FLAG_PIN 2
#define MOISTURE_SENSOR_PIN 35
#define DHT_PIN 18
#define DHT_TYPE DHT22   // DHT 22  (AM2302)

MoistureSensor moistureSensor(MOISTURE_SENSOR_PIN); 
DHT dht(DHT_PIN, DHT_TYPE); //// Initialize DHT sensor for normal 16mhz Arduino

int moisture_sensor_threshold = 65; 

void setup() {
  Serial.begin(9600); 
  dht.begin(); 
  moistureSensor.init(
      Constants::DEFAULT_MOISTURE_MIN, 
      Constants::DEFAULT_MOISTURE_MAX, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MIN, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MAX
  ); 
}

void loop() {
  int raw_moisture = analogRead(MOISTURE_SENSOR_PIN);
  moistureSensor.setRawValue(raw_moisture); 
  float soil_pct_moisture = moistureSensor.readValue(); 

  float hum = dht.readHumidity();
  float temp = dht.readTemperature();
  Serial.print("[RAW=");
  Serial.print(raw_moisture);
  Serial.print("] "); 
  if (soil_pct_moisture > moisture_sensor_threshold) {
    Serial.print("Soil is: WET (");
    digitalWrite(FLAG_PIN, HIGH); 
  } else { 
    Serial.print("Soil is: DRY (");
    digitalWrite(FLAG_PIN, LOW); 
  }
  Serial.print(soil_pct_moisture); 
  Serial.print("), "); 
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" deg C");

  delay(1000); 
}