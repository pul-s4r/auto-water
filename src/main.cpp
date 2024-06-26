#include <Arduino.h>

#include <DHT.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <MQTT.h>
#include <time.h>

#include "Credentials.hpp"
#include "Constants.hpp"
#include "service/MoistureSensor.hpp"

using namespace AutoWater; 

#define FLAG_PIN 2
#define MOISTURE_SENSOR_PIN 35
#define BATTERY_PIN 33
#define PUMP_PIN 32
#define DHT_PIN 18
#define DHT_TYPE DHT22   // DHT 22  (AM2302)

MoistureSensor moistureSensor(MOISTURE_SENSOR_PIN); 
DHT dht(DHT_PIN, DHT_TYPE); // Initialize DHT sensor for normal 16mhz Arduino

MQTTClient mqttClient; 
WiFiClient wifiClient; 

int moisture_limit_1 = 40; 
int moisture_limit_2 = 60; 
uint64_t us_TO_S_FACTOR = 1000000;
RTC_DATA_ATTR int bootCount = 0;

const String sensor_topic = "sensors"; 
const String site_name = location; 

tm timeinfo;
tm last_watered_time; 
time_t now;

int watering_hour = 17; 

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
    if (timeElapsed > max_wifi_connection_wait_ms) { 
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

boolean connect_mqtt() {
  Serial.print("Checking WiFi ");
  unsigned long start = millis(); 
  while (WiFi.status() != WL_CONNECTED) {
    unsigned long timeElapsed = millis() - start; 
    Serial.print(".");
    if (timeElapsed > max_wifi_connection_wait_ms) { 
      Serial.println("\nWifi not connected"); 
      return false; 
    } 
    delay(500);
  }

  Serial.print("\nConnecting to MQTT broker ");
  start = millis(); 
  while (!mqttClient.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
    unsigned long timeElapsed = millis() - start; 
    Serial.print(".");
    if (timeElapsed > max_wifi_connection_wait_ms) { 
      Serial.println("\nTimed out connecting to MQTT broker"); 
      return false; 
    } 
    delay(500);
  }

  Serial.println("\nConnection successful");
  return true; 
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
  Serial.println("Should never happen"); 
}

void increment_and_log_bootcount() {
  bootCount++;
  Serial.println("Boot number: " + String(bootCount));
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  String wakeup_reason_printable; 

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : wakeup_reason_printable = "external signal from RTC_IO"; break;
    case ESP_SLEEP_WAKEUP_EXT1 : wakeup_reason_printable = "external signal from RTC_CNTL"; break;
    case ESP_SLEEP_WAKEUP_TIMER : wakeup_reason_printable = "timer"; break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : wakeup_reason_printable = "touchpad"; break;
    case ESP_SLEEP_WAKEUP_ULP : wakeup_reason_printable = "ULP program"; break;
    default : wakeup_reason_printable = "other - " + wakeup_reason; break;
  }

  Serial.println("Wakeup caused by: " + wakeup_reason_printable); 
}

String create_sensor_data_message(String measurement, String siteName, String value) {
  return "{\"measurement\":\"" + measurement + "\", \"siteName\":\"" + siteName + "\", \"value\":" + value + "}"; 
}

void publish_mqtt_message(String topic, float value) {
  mqttClient.publish(
    (sensor_topic + "/" + topic).c_str(), 
    (create_sensor_data_message(topic, site_name, String(value))).c_str()
  ); 
}

bool getNTPtime(int sec) {
  {
    uint32_t start = millis();
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      Serial.print(".");
      delay(10);
    } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
    if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
    Serial.print("now ");  Serial.println(now);
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    Serial.println(time_output);
    Serial.println();
  }
  return true;
}

void set_last_watered_time() {
  time(&now);
  localtime_r(&now, &last_watered_time);
}

String localtime_to_str(tm lt) {
  char time_output[30];
  strftime(time_output, 30, "%Y-%m-%dT%T", localtime(&now));
  return String(time_output);
}

String localtime_to_day_of_week(tm lt) {
  char time_output[4]; 
  strftime(time_output, 4, "%a", localtime(&now)); 
  return String(time_output); 
}

void operate_pump(int duration_sec) {
  int duration_ms = duration_sec * 1000;
  digitalWrite(PUMP_PIN, HIGH); 
  delay(duration_ms); 
  digitalWrite(PUMP_PIN, LOW); 
}

int calculate_operation_time(float current, float desired, int max = 10) {
  double K = 1; 
  int u = (int) std::round(K * (desired - current)); 
  if (u <= 1) {
    u = 0; 
  }
  u = std::min(u, max); 
  return u; 
}

void setup() {
  Serial.begin(9600); 
  delay(1000); 
  dht.begin(); 
  moistureSensor.init(
      MOISTURE_SENSOR_PIN, 
      Constants::DEFAULT_MOISTURE_MIN, 
      Constants::DEFAULT_MOISTURE_MAX, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MIN, 
      Constants::DEFAULT_MOISTURE_OUTPUT_MAX
  ); 

  increment_and_log_bootcount(); 
  print_wakeup_reason(); 

  connect_wifi(); 

  // setup time
  configTime(0, 0, ntp_server);
  setenv("TZ", tz_info, 1); 

  if (getNTPtime(10)) {
  } else {
    Serial.println("Time not set");
    ESP.restart();
  }

  Serial.println("Time now: " + localtime_to_str(timeinfo) + " (" + localtime_to_day_of_week(timeinfo) + ")"); 

  mqttClient.begin(mqtt_broker_hostname, mqtt_broker_port, wifiClient); 

  boolean isMqttConnected = connect_mqtt(); 
  if (!isMqttConnected) {
    Serial.println("Connection to broker not established, entering deep sleep"); 
    enter_deep_sleep(60); 
  }
  mqttClient.loop(); 

  mqttClient.subscribe(sensor_topic);

  int raw_moisture = moistureSensor.readRawValue(); 
  float soil_pct_moisture = moistureSensor.readValue(); 

  float hum = dht.readHumidity();
  float temp = dht.readTemperature();
  Serial.print("[RAW=" + String(raw_moisture) + "] "); 
  boolean is_moisture_below_limit = soil_pct_moisture < moisture_limit_1; 
  boolean is_moisture_below_daily_limit = soil_pct_moisture < moisture_limit_2; 
  String isWet = is_moisture_below_limit ? "DRY" : "WET"; 
  Serial.println("Soil is: " + isWet + " (" + String(soil_pct_moisture) + 
    "), Humidity: " + String(hum) + "%, Temp: " + String(temp) + " deg C"
  ); 

  if (is_moisture_below_limit) {
    int pump_duration_s = calculate_operation_time(soil_pct_moisture, moisture_limit_1); 
    Serial.println("Operating pump for: " + String(pump_duration_s) + "s to reach instantaneous threshold"); 
    operate_pump(pump_duration_s); 
    set_last_watered_time(); 
  } else if (is_moisture_below_daily_limit 
      && last_watered_time.tm_mday != timeinfo.tm_mday
      && last_watered_time.tm_hour >= watering_hour) {
    int pump_duration_s = calculate_operation_time(soil_pct_moisture, moisture_limit_2); 
    Serial.println("Operating pump for: " + String(pump_duration_s) + "s to reach daily threshold"); 
    operate_pump(pump_duration_s); 
    set_last_watered_time(); 
  } else {
    Serial.println("Moisture is normal"); 
  }

  float batteryLevel = map(analogRead(BATTERY_PIN), 0.0f, 4095.0f, 0, 100);

  publish_mqtt_message("soilMoisture", soil_pct_moisture); 
  publish_mqtt_message("humidity", hum); 
  publish_mqtt_message("temperature", temp); 
  publish_mqtt_message("batteryLevel", batteryLevel); 

  mqttClient.unsubscribe(sensor_topic.c_str());
  bool disconnected = mqttClient.disconnect(); 
  Serial.print("Disconnected from MQTT broker: ");
  Serial.println(disconnected ? "yes" : "no");  

  enter_deep_sleep(60); 
}

void loop() {

}

void stop() {
  noInterrupts(); 
  while(1); 
}