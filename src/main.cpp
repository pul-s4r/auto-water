#include <Arduino.h>

#include <DHT.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <MQTT.h>

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

MQTTClient mqttClient; 
WiFiClient wifiClient; 

int moisture_sensor_threshold = 65; 
uint64_t us_TO_S_FACTOR = 1000000;
RTC_DATA_ATTR int bootCount = 0;

const String sensor_topic = "sensors"; 
const String site_name = location; 

uint8_t connect_wifi() {
  WiFi.disconnect(); 
  WiFi.mode(WIFI_STA); 
  Serial.println("Connecting to: " + String(wifi_ssid));

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

void connect_mqtt() {
  Serial.print("Checking WiFi ...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\nConnecting to MQTT broker ...");
  while (!mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nConnection successful");
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

void enter_deep_sleep(uint64_t sleepTimeInSecs) {
  disconnect_wifi(); 
  btStop(); 
  delay(300); 
  esp_wifi_stop(); 
  esp_bt_controller_disable(); 

  esp_sleep_enable_timer_wakeup(sleepTimeInSecs * us_TO_S_FACTOR); 

  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start(); 
  Serial.println("SHould never happen"); 
}

void increment_and_log_bootcount() {
  bootCount++;
  Serial.println("Boot number: " + String(bootCount));
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  Serial.begin(9600); 
  delay(1000); 
  dht.begin(); 
  moistureSensor.init(
      Constants::DEFAULT_MOISTURE_MIN, 
      Constants::DEFAULT_MOISTURE_MAX, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MIN, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MAX
  ); 

  increment_and_log_bootcount(); 
  print_wakeup_reason(); 

  connect_wifi(); 

  mqttClient.begin(mqtt_broker_hostname, mqtt_broker_port, wifiClient); 
  // mqttClient.onMessage(messageReceived); 

  connect_mqtt(); 
  mqttClient.loop(); 

  mqttClient.subscribe(sensor_topic);

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

  // Serial.println(("soilMoisture,site=" + site_name + " value=" + String(soil_pct_moisture)).c_str()); 
  Serial.println((String(soil_pct_moisture)).c_str()); 

  mqttClient.publish(
    (sensor_topic + "/soilMoisture").c_str(), 
    ("soilMoisture,site=" + site_name + " value=" + String(soil_pct_moisture)).c_str()
  ); 
  mqttClient.publish(
    (sensor_topic + "/humidity").c_str(), 
    ("humidity,site=" + site_name + " value=" + String(hum)).c_str()
  ); 
  mqttClient.publish(
    (sensor_topic + "/temperature").c_str(), 
    ("temperature,site=" + site_name + " value=" + String(temp)).c_str()
  ); 


  mqttClient.unsubscribe(sensor_topic.c_str());
  bool disconnected = mqttClient.disconnect(); 
  Serial.print("Disconnected from MQTT broker: ");
  Serial.println(disconnected ? "yes" : "no");  

  enter_deep_sleep(60); 
}

void loop() {

}

void stop() {
  disconnect_wifi(); 
  noInterrupts(); 
  while(1); 
}