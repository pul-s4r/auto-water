#include <Arduino.h>

#include <DHT.h>

#define FLAG_PIN 2
#define MOISTURE_SENSOR_PIN 4
#define DHT_PIN 18
#define DHT_TYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHT_PIN, DHT_TYPE); //// Initialize DHT sensor for normal 16mhz Arduino

int moisture_sensor_threshold = 65; 

long moisture_offset = 1071; 
long moisture_max = 4095; 
long moisture_rmin = 100; 
long moisture_rmax = 0; 

void setup() {
  Serial.begin(9600); 
  dht.begin(); 
}

void loop() {
  int raw_moisture = analogRead(MOISTURE_SENSOR_PIN);
  float soil_pct_moisture = map(raw_moisture, moisture_offset, moisture_max, moisture_rmin, moisture_rmax);

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