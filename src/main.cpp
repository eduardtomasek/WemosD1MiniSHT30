#include <Arduino.h>

/**
 * WIFI
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "credentials.h" // WiFi SSID and password
ESP8266WebServer server(80);

/**
 * SHT30
 */
#include <Wire.h>
#include <WEMOS_SHT3X.h>

SHT3X sht30(0x45);
float sht30Data[2]; // 0 => temperature, 1 => humidity

/**
 * MQTT
 */
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

void mqttSetup () {
    client.setServer(mqtt_broker, mqtt_port);

    while (!client.connected()) {
      String client_id = "esp8266-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
}

/**
 * HTTP CONTROLLERS
 */
void getHelloWorldController() {
    server.send(200, "text/json", "{\"name\": \"Hello world\"}");
}

void getTemperatureController () {
    server.send(200, "text/json", String("{ \"temp\": \""+ String(sht30Data[0], 2) +"\", \"unit\": \"°C\" }"));
}

void getHumidityController () {
    server.send(200, "text/json", String("{ \"humidity\": \""+ String(sht30Data[1], 3) +"\", \"unit\": \"rh\" }"));
}

void getArgumentsController () {
    String value = server.arg("value");
    Serial.println(value);
    server.send(200, "text/json", String("{ \"value\": \""+ value +"\" }"));
}

void getAllController () {
  server.send(200, "text/json", String("[\
      { \"temp\": \""+ String(sht30Data[0], 2) +"\", \"unit\": \"°C\" }, \
      { \"humidity\": \""+ String(sht30Data[1], 3) +"\", \"unit\": \"rh\" }, \
     ]"));
}

void handleNotFound() { // Manage not found URL
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}
 
/**
 * ROUTING
 */
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"), F("Sensor Array Web Server"));
    });
    
    server.on(F("/helloWorld"), HTTP_GET, getHelloWorldController);
    server.on(F("/temperature"), HTTP_GET, getTemperatureController);
    server.on(F("/humidity"), HTTP_GET, getHumidityController);
    server.on(F("/all"), HTTP_GET, getAllController);

    server.on(F("/argument"), HTTP_GET, getArgumentsController);
}
 
void restAPISetup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
 
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void otaSetup () {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);

  restAPISetup();
  otaSetup();
  mqttSetup();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  if (sht30.get() == 0) {
    sht30Data[0] = sht30.cTemp;
    char temperature[8];
    dtostrf(sht30Data[0], 6, 2, temperature);
    client.publish(topic_temperature, temperature);

    sht30Data[1] = sht30.humidity;
    char humidity[8];
    dtostrf(sht30Data[1], 6, 2, humidity);
    client.publish(topic_humidity, humidity);

    Serial.print("T: ");
    Serial.println(sht30Data[0]);

    Serial.print("H: ");
    Serial.println(sht30Data[1]);
  } else {
    Serial.println("Error!");
  }

  delay(1000);
}