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
#include <web_server_scheduled.h>
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
          MqttSettings.port = doc["mqtt_port"] | 1883; // Default to 1883 if missing
          MqttSettings.username = doc["mqtt_username"].as<String>();
          MqttSettings.password = doc["mqtt_password"].as<String>();

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

          // Handle enable flag: if server is set, default to enabled
          if (doc.containsKey("mqtt_enable")) {
            MqttSettings.enable = doc["mqtt_enable"].as<bool>();
          } else {
            // If key is missing, default to true if server is set
            MqttSettings.enable = (MqttSettings.server.length() > 0);
            if (MqttSettings.server.length() > 0) {
              Logger.println("[Config] mqtt_enable not found, defaulting to enabled");
            }
          }

          // If server is empty, ensure enable is false
          if (MqttSettings.server.length() == 0 && MqttSettings.enable) {
            Logger.println("[Config] Warning: server empty but enable=true, disabling");
            MqttSettings.enable = false;
          }

          Logger.printf("[Config] Loaded: server='%s', port=%d, enable=%d\n",
                        MqttSettings.server.c_str(), MqttSettings.port, MqttSettings.enable);
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
  Logger.printf("[Config] WiFi manager returned: server='%s', enable=%d\n",
                from_wifi_manager.server.c_str(), from_wifi_manager.enable);

  if (from_wifi_manager.server != "") {
    MqttSettings = from_wifi_manager;
    Logger.println("[Config] Using WiFi manager values");
  } else {
    Logger.println("[Config] WiFi manager empty, keeping config file values");
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
  Logger.println("[Time] NTP sync started, waiting for time sync...");

  // Only enable MQTT if server is configured and not null/empty
  bool mqttCanStart = MqttSettings.enable &&
                      MqttSettings.server.length() > 0 &&
                      MqttSettings.server != "null";

  if (mqttCanStart) {
    Logger.println("\n[MQTT] MQTT is enabled");
    Logger.printf("[MQTT] Server: %s:%d\n", MqttSettings.server.c_str(), MqttSettings.port);
    if (MqttSettings.username.length() > 0) {
      Logger.printf("[MQTT] Username: %s\n", MqttSettings.username.c_str());
      Logger.println("[MQTT] Password: (set)");
    } else {
      Logger.println("[MQTT] Authentication: Anonymous");
    }
    client.setServer(MqttSettings.server.c_str(), MqttSettings.port);
    client.setCallback(mqtt_callback);
    Logger.println("[MQTT] Client configured, will connect in loop()");
  } else if (MqttSettings.enable && (MqttSettings.server.length() == 0 || MqttSettings.server == "null")) {
    Logger.println("\n[MQTT] MQTT enabled but server not configured - skipping");
    MqttSettings.enable = false;
  } else {
    Logger.println("\n[MQTT] MQTT is disabled");
  }

  Logger.printf("Ready! Open http://%s.local in your browser\n", HOSTNAME);
}

void loop() {
  // Handle telnet connections
  Logger.handle();

  // Handle time sync
  static bool timeSyncLogged = false;
  if (!timeSyncLogged && timeSync.isSynced()) {
    Logger.println("[Time] Time synchronized successfully!");
    Logger.printf("[Time] Current time: %s\n", timeSync.getFormattedTime().c_str());
    timeSyncLogged = true;
  }
  timeSync.loop();

  // Check if MQTT should be enabled - require a valid server
  bool mqttCanConnect = MqttSettings.enable &&
                        MqttSettings.server.length() > 0 &&
                        MqttSettings.server != "null";

  if (mqttCanConnect) {
    static unsigned long lastConnectAttempt = 0;
    static int retryCount = 0;
    static bool wasDisconnected = false;
    const unsigned long retryInterval = 5000; // 5 seconds

    if (!client.connected()) {
      unsigned long now = millis();
      if (now - lastConnectAttempt >= retryInterval || lastConnectAttempt == 0) {
        lastConnectAttempt = now;
        retryCount++;
        if (retryCount == 1) {
          Logger.println("[MQTT] Connection lost. Starting reconnection attempts...");
          wasDisconnected = true;
        }
        Logger.printf("[MQTT] Retry attempt #%d\n", retryCount);
        mqtt_connect(MqttSettings.username.c_str(), MqttSettings.password.c_str());

        if (retryCount >= 50) {
          Logger.println("[MQTT] Too many connection failures, restarting ESP...");
          ESP.restart();
        }
      }
    } else {
      // Reset retry count on successful connection
      if (retryCount > 0 || wasDisconnected) {
        if (wasDisconnected) {
          Logger.println("[MQTT] Connection restored!");
          wasDisconnected = false;
        }
        retryCount = 0;
        lastConnectAttempt = 0;
      }
    }
    client.loop();
    checkScheduledZoneStop();
  }
  // wifiDisconnected = checkWifiConnection(wifiDisconnected);
  MDNS.update();
  // timer.run();
}