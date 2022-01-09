# Wemos D1 Mini SHT30 [SundayProjects]
Test for technologies listed bellow.

* SHT30 Wemos D1 Mini Shield
* REST API
    * Query params processing
* OTA
* WIFI
* MQTT

# include/credentials.h
```cpp
const char* ssid = "<ssid>";
const char* password = "<password>";

// MQTT Broker
const char *mqtt_broker = "<mqtt address>";
const char *topic_temperature = "<temperature topic>";
const char *topic_humidity = "<humidity topic>";
const char *mqtt_username = "<mqtt user>";
const char *mqtt_password = "<mqtt password>";
const int mqtt_port = 1883;
```
# Sources
## MQTT
https://www.emqx.com/en/blog/esp8266-connects-to-the-public-mqtt-broker