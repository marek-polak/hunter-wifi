/**
 * First version: July 2020 - Eloi Codina Torras
 */
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>

#include <global_config.h>
#include <mqtt.h>
#include <telnet_logger.h>
#include <time_sync.h>
#include <web_server.h>
#include <wifi.h>

AsyncWebServer server = AsyncWebServer(80);
bool wifiDisconnected = false;
bool enableMQTT = true;
MQTT MqttSettings;
MQTT from_wifi_manager;

void setup() {
  Serial.begin(SERIAL_SPEED);
  // WiFi.mode(WIFI_STA);
  String syslog_server = "";
  int syslog_port = 514;
  bool syslog_enable = false;
  String timezone = "<-03>3"; // Default: UTC-3

  Logger.println("mounting FS...");
  if (LittleFS.begin()) {
    Logger.println("mounted file system");
    if (LittleFS.exists("/config.json")) {
      // file exists, reading and loading
      Logger.println("reading config file");
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        Logger.println("opened config file");
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, configFile);
        serializeJson(doc, Serial);
        if (error) {
          Logger.print(F("\ndeserializeJson() failed: "));
          Logger.println(error.c_str());
          Logger.println("failed to load json config");
        } else {
          Logger.println("\njson successfully parsed");
          MqttSettings.server = doc["mqtt_server"].as<String>();
          MqttSettings.port = doc["mqtt_port"];
          MqttSettings.username = doc["mqtt_username"].as<String>();
          MqttSettings.password = doc["mqtt_password"].as<String>();
          MqttSettings.enable = doc["mqtt_enable"].as<bool>();

          if (doc.containsKey("syslog_server")) {
            syslog_server = doc["syslog_server"].as<String>();
          }
          if (doc.containsKey("syslog_port")) {
            syslog_port = doc["syslog_port"] | 514;
          }
          if (doc.containsKey("syslog_enable")) {
            syslog_enable = doc["syslog_enable"].as<bool>();
          }
          if (doc.containsKey("timezone")) {
            timezone = doc["timezone"].as<String>();
          }
        }
      }
    }
  } else {
    Logger.println("failed to mount FS");
  }
  // this is here just to make sure that if wifimanager is
  // skipped when config is saved, MQTT still works, otherwise empty data would
  // override what we learned from config file saved on disk
  from_wifi_manager = setup_wifi();
  if (from_wifi_manager.server != "") {
    MqttSettings = from_wifi_manager;
  }
  setup_WebServer();
  if (shouldSaveConfig) {
    Logger.println("saving config");
    DynamicJsonDocument doc(1024);
    doc["mqtt_server"] = MqttSettings.server;
    doc["mqtt_port"] = MqttSettings.port;
    doc["mqtt_username"] = MqttSettings.username;
    doc["mqtt_password"] = MqttSettings.password;
    doc["mqtt_enable"] = MqttSettings.enable;
    doc["syslog_server"] = syslog_server;
    doc["syslog_port"] = syslog_port;
    doc["syslog_enable"] = syslog_enable;
    doc["timezone"] = timezone;
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      Logger.println("failed to open config file for writing");
    }
    serializeJson(doc, Serial);
    serializeJsonPretty(doc, configFile);
    configFile.close();
    // end save
  }
  Logger.print("\nIP address: ");
  Logger.println(WiFi.localIP().toString());

  // Initialize telnet logging
  Logger.begin();
  Logger.setSyslogConfig(syslog_server, syslog_port, syslog_enable);
  Logger.printf("[Boot] Firmware %s (built %s)\n", FIRMWARE_VERSION, BUILD_TIME);

  // Initialize NTP time sync
  Logger.printf("[Time] Initializing NTP with timezone: %s\n", timezone.c_str());
  timeSync.begin(timezone);
  Logger.println("[Time] NTP sync started");

  if (MqttSettings.enable) {
    client.setServer(MqttSettings.server.c_str(), MqttSettings.port);
    client.setCallback(mqtt_callback);
  }
  Logger.printf("Ready! Open http://%s.local in your browser\n", HOSTNAME);
}

void loop() {
  // Handle telnet connections
  Logger.handle();

  // Handle time sync
  timeSync.loop();

  if (MqttSettings.enable) {
    if (!client.connected()) {
      int retries = 0;
      while (!client.connected()) {
        if (retries < 50) {
          mqtt_connect(MqttSettings.username.c_str(),
                       MqttSettings.password.c_str());
        } else {
          retries++;
          // Wait 5 seconds before retrying
          delay(5000);
        }
        if (retries >= 50) {
          ESP.restart();
        }
      }
    }
  }
  // wifiDisconnected = checkWifiConnection(wifiDisconnected);
  client.loop();
  MDNS.update();
  // timer.run();
}