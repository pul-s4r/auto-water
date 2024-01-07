#include <Arduino.h>

#include <DHT.h>
#include <WiFi.h>
#include "Credentials.hpp"
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

uint8_t connect_wifi() {
  WiFi.disconnect(); 
  WiFi.mode(WIFI_STA); 
  Serial.print("Connecting to: ");
  Serial.println(String(wifi_ssid));

  WiFi.begin(wifi_ssid, wifi_password); 

  unsigned long start = millis(); 
  uint8_t connectionStatus; 
  bool attemptConnection = true; 
  while (attemptConnection) {
    connectionStatus = WiFi.status(); 
    unsigned long timeElapsed = millis() - start; 
    if (connectionStatus == WL_CONNECTED || connectionStatus == WL_CONNECT_FAILED) { 
      attemptConnection = false; 
    }
    if (timeElapsed > max_connection_wait_ms) { 
      Serial.print("Timed out"); 
      attemptConnection = false;
    } 
    delay(1000); 
  }

  if (connectionStatus == WL_CONNECTED) {
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi connection *** FAILED ***");
  }

  return connectionStatus; 
}

void disconnect_wifi() {
  WiFi.disconnect(); 
  delay(500); 
  if (WiFi.status() == WL_DISCONNECTED) {
    Serial.println("WiFi disconnected");
  } else {
    Serial.println("Error disconnecting wifi");
  }
  WiFi.mode(WIFI_OFF); 
}

void setup() {
  Serial.begin(9600); 
  dht.begin(); 
  moistureSensor.init(
      Constants::DEFAULT_MOISTURE_MIN, 
      Constants::DEFAULT_MOISTURE_MAX, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MIN, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MAX
  ); 

  connect_wifi(); 
  delay(5000); 
  disconnect_wifi(); 
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

void stop() {
  disconnect_wifi(); 
  noInterrupts(); 
  while(1); 
}