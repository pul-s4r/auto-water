#ifndef _CREDENTIALS_HPP
#define _CREDENTIALS_HPP

const char* wifi_ssid = ""; 
const char* wifi_password = ""; 
const int max_wifi_connection_wait_ms = 15000; 
const int max_mqtt_connection_wait_ms = 15000; 

const char* mqtt_broker_hostname = ""; 
int mqtt_broker_port = 1883; 
const char* mqtt_username = "";
const char* mqtt_password = ""; 
const char* mqtt_client_id = ""; 

const char* location = ""; 

const char* ntp_server = "pool.ntp.org";
const char* tz_info = "EST-10EDT-11,M10.5.0/02:00:00,M3.5.0/03:00:00";

#endif