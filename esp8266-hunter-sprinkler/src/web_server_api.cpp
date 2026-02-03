/**
 * This file includes all the functions that are called by the webserver to
 * serve API paths. It makes it easier to control the actions to be performed to
 * the Hunter Irrigation System.
 *
 * First version: July 2020 - Eloi Codina Torras
 */
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <Schedule.h>

#include <global_config.h>
#include <mqtt.h>
#include <telnet_logger.h>
#include <time_sync.h>
#include <web_server_api.h>
#include <web_server_scheduled.h>

extern MQTT MqttSettings;
extern PubSubClient client;

/**
 * Extract the latest parameter on the URL given the request and the base path.
 * It has to be done with this "hack" as REGEX doesnt't work properly on the
 * ESP8266 currently.
 *
 * Example:
 * AsyncWebServerRequest request </api/start/zone/3>
 * int id = getIdFromURL(request, "/api/start/zone/");
 * Serial.println(id);
 *  --> 3
 *
 * @param request request made by the client
 * @param root base path of the URL.
 */
int getIdFromURL(AsyncWebServerRequest *request, String root) {
  String string_id = request->url();
  string_id.replace(root, "");
  int id = string_id.toInt();
  return id;
}

/**
 * Function to be called when the client makes a request to
 * /api/start/zone
 * It checks that all the parameters have been passed and schedules a function
 * to be run at the next loop to start the zone with the HunterRoam library.
 *
 * @param request request made by the client
 */
void startZoneRequest(AsyncWebServerRequest *request) {
  if (!(request->hasParam("time"))) {
    request->send_P(400, "text/plain", "Please include parameter 'time'.");
  } else {
    byte num = getIdFromURL(request, "/api/start/zone/");
    byte time = request->getParam("time")->value().toInt();

    String result;
    String toReturn;
    if (!(request->hasParam("result"))) {
      result = "";
      toReturn = "\"As you have not specified a 'result' argument, the result "
                 "of the operation can't be send to you.\"";
    } else {
      result = (String)request->getParam("result")->value();
      toReturn = "\"After the action, the result will be sent to the webhook "
                 "url you have specified.\"";
    }

    schedule_function([num, time, result]() { startZone(num, time, result); });

    request->send_P(200, "application/json",
                    ((String) "{\"status\": \"Request received and processed "
                              "correctly.\"," +
                     (String) "\"result\": " + toReturn + "}")
                        .c_str());
  }
}

/**
 * Function to be called when the client makes a request to
 * /api/start/program
 * It checks that all the parameters have been passed and schedules a function
 * to be run at the next loop to start the program with the HunterRoam library.
 *
 * @param request request made by the client
 */
void startProgramRequest(AsyncWebServerRequest *request) {

  String result;
  String toReturn;
  if (!(request->hasParam("result"))) {
    result = "";
    toReturn = "\"As you have not specified a 'result' argument, the result of "
               "the operation can't be send to you.\"";
  } else {
    result = request->getParam("result")->value();
    toReturn = "\"After the action, the result will be sent to the webhook url "
               "you have specified.\"";
  }

  byte num = getIdFromURL(request, "/api/start/program/");
  schedule_function([num, result]() { startProgram(num, result); });

  request->send_P(
      200, "application/json",
      ((String) "{\"status\": \"Request received and processed correctly.\"," +
       (String) "\"result\": " + toReturn + "}")
          .c_str());
}

/**
 * Function to be called when the client makes a request to
 * /api/stop/zone
 * It checks that all the parameters have been passed and schedules a function
 * to be run at the next loop to stop the zone with the HunterRoam library.
 *
 * @param request request made by the client
 */
void stopZoneRequest(AsyncWebServerRequest *request) {

  String result;
  String toReturn;
  if (!(request->hasParam("result"))) {
    result = "";
    toReturn = "\"As you have not specified a 'result' argument, the result of "
               "the operation can't be send to you.\"";
  } else {
    result = request->getParam("result")->value();
    toReturn = "\"After the action, the result will be sent to the webhook url "
               "you have specified.\"";
  }

  byte num = getIdFromURL(request, "/api/stop/zone/");
  schedule_function([num, result]() { stopZone(num, result); });

  request->send_P(
      200, "application/json",
      ((String) "{\"status\": \"Request received and processed correctly.\"," +
       (String) "\"result\": " + toReturn + "}")
          .c_str());
}

/**
 * Function to update MQTT configuration via API
 * POST /api/config/mqtt
 * Parameters: server, port, username, password, enable (optional)
 * 
 * @param request request made by the client
 */
void updateMqttConfig(AsyncWebServerRequest *request) {
  // Check if required parameters are present
  if (!request->hasParam("server", true)) {
    request->send(400, "application/json", "{\"error\": \"Missing required parameter: server\"}");
    return;
  }

  // Update MQTT settings from request parameters
  if (request->hasParam("server", true)) {
    MqttSettings.server = request->getParam("server", true)->value();
  }
  if (request->hasParam("port", true)) {
    String portStr = request->getParam("port", true)->value();
    MqttSettings.port = portStr.toInt();
  } else {
    MqttSettings.port = 1883; // Default port
  }
  if (request->hasParam("username", true)) {
    MqttSettings.username = request->getParam("username", true)->value();
  } else {
    MqttSettings.username = "";
  }
  if (request->hasParam("password", true)) {
    String newPassword = request->getParam("password", true)->value();
    // Only update password if a new one was provided (allows keeping existing password)
    if (newPassword.length() > 0) {
      MqttSettings.password = newPassword;
    }
    // If empty password is explicitly sent, we keep the existing password
    // To clear password, user must provide an empty username too
  }
  // If password param not present at all, keep existing password
  // Handle enable flag: if server is provided, default to enabled unless explicitly disabled
  if (request->hasParam("enable", true)) {
    String enableStr = request->getParam("enable", true)->value();
    MqttSettings.enable = (enableStr == "1" || enableStr == "true");
  } else {
    // If enable not specified but server is provided, default to enabled
    if (MqttSettings.server.length() > 0 && MqttSettings.server != "null") {
      MqttSettings.enable = true;
      Logger.println("[MQTT] Enable flag not specified, defaulting to enabled (server provided)");
    }
    // Otherwise keep current value
  }
  
  // Prevent enabling MQTT with empty/null server
  if (MqttSettings.enable && (MqttSettings.server.length() == 0 || MqttSettings.server == "null")) {
    Logger.println("[MQTT] Cannot enable MQTT with empty/null server, setting enable=false");
    MqttSettings.enable = false;
  }
  
  // If username is cleared, also clear password
  if (MqttSettings.username.length() == 0) {
    MqttSettings.password = "";
  }

  // Save configuration to filesystem (read existing first to preserve other keys e.g. syslog)
  if (LittleFS.begin()) {
    DynamicJsonDocument doc(1024);
    if (LittleFS.exists("/config.json")) {
      File readFile = LittleFS.open("/config.json", "r");
      if (readFile) {
        deserializeJson(doc, readFile);
        readFile.close();
      }
    }
    doc["mqtt_server"] = MqttSettings.server;
    doc["mqtt_port"] = MqttSettings.port;
    doc["mqtt_username"] = MqttSettings.username;
    doc["mqtt_password"] = MqttSettings.password;
    doc["mqtt_enable"] = MqttSettings.enable;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      request->send(500, "application/json", "{\"error\": \"Failed to open config file for writing\"}");
      return;
    }
    
    serializeJsonPretty(doc, configFile);
    configFile.close();
    
    // Update MQTT client settings
    Logger.println("[MQTT] Configuration updated via API");
    Logger.printf("[MQTT] Server: %s:%d\n", MqttSettings.server.c_str(), MqttSettings.port);
    if (MqttSettings.username.length() > 0) {
      Logger.printf("[MQTT] Username: %s\n", MqttSettings.username.c_str());
      Logger.println("[MQTT] Password: (set)");
    } else {
      Logger.println("[MQTT] Authentication: Anonymous (no username/password)");
    }
    Logger.printf("[MQTT] Enabled: %s\n", MqttSettings.enable ? "yes" : "no");
    
    if (MqttSettings.enable) {
      client.setServer(MqttSettings.server.c_str(), MqttSettings.port);
      // Disconnect current connection if any
      if (client.connected()) {
        Logger.println("[MQTT] Disconnecting existing connection to apply new settings...");
        client.disconnect();
      }
      Logger.println("[MQTT] New server settings applied, will reconnect in loop()");
    } else {
      Logger.println("[MQTT] MQTT disabled, disconnecting if connected...");
      if (client.connected()) {
        client.disconnect();
      }
    }
    
    request->send(200, "application/json", 
      "{\"status\": \"MQTT configuration updated successfully\", "
      "\"server\": \"" + MqttSettings.server + "\", "
      "\"port\": " + String(MqttSettings.port) + ", "
      "\"enable\": " + String(MqttSettings.enable ? "true" : "false") + "}");
  } else {
    request->send(500, "application/json", "{\"error\": \"Failed to mount filesystem\"}");
  }
}

/**
 * Function to update syslog configuration via API
 * POST /api/config/syslog
 * Parameters: server (required), port (optional, default 514), enable (optional)
 *
 * @param request request made by the client
 */
void updateSyslogConfig(AsyncWebServerRequest *request) {
  if (!request->hasParam("server", true)) {
    request->send(400, "application/json", "{\"error\": \"Missing required parameter: server\"}");
    return;
  }

  String syslog_server = request->getParam("server", true)->value();
  int syslog_port = 514;
  if (request->hasParam("port", true)) {
    syslog_port = request->getParam("port", true)->value().toInt();
  }
  bool syslog_enable = false;
  if (request->hasParam("enable", true)) {
    String enableStr = request->getParam("enable", true)->value();
    syslog_enable = (enableStr == "1" || enableStr == "true");
  }

  if (LittleFS.begin()) {
    DynamicJsonDocument doc(1024);
    if (LittleFS.exists("/config.json")) {
      File readFile = LittleFS.open("/config.json", "r");
      if (readFile) {
        deserializeJson(doc, readFile);
        readFile.close();
      }
    }
    doc["syslog_server"] = syslog_server;
    doc["syslog_port"] = syslog_port;
    doc["syslog_enable"] = syslog_enable;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      request->send(500, "application/json", "{\"error\": \"Failed to open config file for writing\"}");
      return;
    }
    serializeJsonPretty(doc, configFile);
    configFile.close();

    Logger.setSyslogConfig(syslog_server, syslog_port, syslog_enable);
    Logger.println("[Syslog] Configuration updated via API");
    Logger.printf("[Syslog] Server: %s:%d, enable: %s\n",
                  syslog_server.c_str(), syslog_port, syslog_enable ? "yes" : "no");

    request->send(200, "application/json",
      "{\"status\": \"Syslog configuration updated successfully\", "
      "\"server\": \"" + syslog_server + "\", "
      "\"port\": " + String(syslog_port) + ", "
      "\"enable\": " + String(syslog_enable ? "true" : "false") + "}");
  } else {
    request->send(500, "application/json", "{\"error\": \"Failed to mount filesystem\"}");
  }
}

/**
 * Get current syslog configuration (for web UI pre-fill).
 * GET /api/config/syslog
 * Reads from config.json and returns server, port, enable.
 *
 * @param request request made by the client
 */
void syslogConfigStatus(AsyncWebServerRequest *request) {
  if (!LittleFS.begin()) {
    request->send(500, "application/json", "{\"error\": \"Failed to mount filesystem\"}");
    return;
  }
  String server = "";
  int port = 514;
  bool enable = false;
  if (LittleFS.exists("/config.json")) {
    File readFile = LittleFS.open("/config.json", "r");
    if (readFile) {
      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, readFile) == DeserializationError::Ok) {
        if (doc.containsKey("syslog_server")) server = doc["syslog_server"].as<String>();
        if (doc.containsKey("syslog_port")) port = doc["syslog_port"] | 514;
        if (doc.containsKey("syslog_enable")) enable = doc["syslog_enable"].as<bool>();
      }
      readFile.close();
    }
  }
  // Build JSON response (escape server for JSON string)
  String escaped = server;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  String response = "{\"server\": \"" + escaped + "\", \"port\": " + String(port) + ", \"enable\": " + String(enable ? "true" : "false") + "}";
  request->send(200, "application/json", response);
}

/**
 * Get firmware version and build info
 * GET /api/version
 */
void apiVersion(AsyncWebServerRequest *request) {
  String response = "{\"version\": \"" + String(FIRMWARE_VERSION) + "\", \"build\": \"" + String(BUILD_TIME) + "\"}";
  request->send(200, "application/json", response);
}

/**
 * Function to update timezone configuration via API
 * POST /api/config/timezone
 * Parameters: timezone (required, POSIX format)
 * 
 * Example timezones:
 * - Argentina/Buenos Aires: "<-03>3"
 * - US Eastern: "EST5EDT,M3.2.0,M11.1.0"
 * - Central European: "CET-1CEST,M3.5.0,M10.5.0/3"
 * - UTC: "UTC0"
 * 
 * @param request request made by the client
 */
void updateTimezoneConfig(AsyncWebServerRequest *request) {
  if (!request->hasParam("timezone", true)) {
    request->send(400, "application/json", "{\"error\": \"Missing required parameter: timezone\"}");
    return;
  }

  String timezone = request->getParam("timezone", true)->value();
  
  // Basic validation - timezone should not be empty
  if (timezone.length() == 0) {
    request->send(400, "application/json", "{\"error\": \"Timezone cannot be empty\"}");
    return;
  }

  // Save configuration to filesystem
  if (LittleFS.begin()) {
    DynamicJsonDocument doc(1024);
    if (LittleFS.exists("/config.json")) {
      File readFile = LittleFS.open("/config.json", "r");
      if (readFile) {
        deserializeJson(doc, readFile);
        readFile.close();
      }
    }
    doc["timezone"] = timezone;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      request->send(500, "application/json", "{\"error\": \"Failed to open config file for writing\"}");
      return;
    }
    
    serializeJsonPretty(doc, configFile);
    configFile.close();
    
    // Update time sync with new timezone
    Logger.printf("[Time] Timezone configuration updated via API: %s\n", timezone.c_str());
    timeSync.setTimezone(timezone);
    
    String currentTime = timeSync.isSynced() ? timeSync.getFormattedTime() : "Not synced yet";
    
    request->send(200, "application/json", 
      "{\"status\": \"Timezone configuration updated successfully\", "
      "\"timezone\": \"" + timezone + "\", "
      "\"current_time\": \"" + currentTime + "\", "
      "\"synced\": " + String(timeSync.isSynced() ? "true" : "false") + "}");
  } else {
    request->send(500, "application/json", "{\"error\": \"Failed to mount filesystem\"}");
  }
}

/**
 * Get current time and timezone information
 * GET /api/time/status
 * 
 * @param request request made by the client
 */
void timeStatus(AsyncWebServerRequest *request) {
  String response = "{";
  response += "\"synced\": " + String(timeSync.isSynced() ? "true" : "false") + ",";
  response += "\"timezone\": \"" + timeSync.getTimezone() + "\",";
  if (timeSync.isSynced()) {
    response += "\"current_time\": \"" + timeSync.getFormattedTime() + "\",";
    response += "\"iso8601\": \"" + timeSync.getISO8601Time() + "\"";
  } else {
    response += "\"current_time\": \"Not synced yet\",";
    response += "\"iso8601\": \"1970-01-01T00:00:00Z\"";
  }
  response += "}";
  
  request->send(200, "application/json", response);
}

/**
 * Get current MQTT configuration (for web UI pre-fill).
 * GET /api/config/mqtt
 * Reads from config.json and returns server, port, username, enable.
 *
 * @param request request made by the client
 */
void mqttConfigStatus(AsyncWebServerRequest *request) {
  if (!LittleFS.begin()) {
    request->send(500, "application/json", "{\"error\": \"Failed to mount filesystem\"}");
    return;
  }
  String server = "";
  int port = 1883;
  String username = "";
  bool enable = false;
  if (LittleFS.exists("/config.json")) {
    File readFile = LittleFS.open("/config.json", "r");
    if (readFile) {
      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, readFile) == DeserializationError::Ok) {
        if (doc.containsKey("mqtt_server")) server = doc["mqtt_server"].as<String>();
        if (doc.containsKey("mqtt_port")) port = doc["mqtt_port"] | 1883;
        if (doc.containsKey("mqtt_username")) username = doc["mqtt_username"].as<String>();
        if (doc.containsKey("mqtt_enable")) enable = doc["mqtt_enable"].as<bool>();
      }
      readFile.close();
    }
  }
  // Don't send password for security, just indicate if it's set
  bool hasPassword = false;
  if (LittleFS.exists("/config.json")) {
    File readFile = LittleFS.open("/config.json", "r");
    if (readFile) {
      DynamicJsonDocument doc(1024);
      if (deserializeJson(doc, readFile) == DeserializationError::Ok) {
        if (doc.containsKey("mqtt_password")) {
          String pwd = doc["mqtt_password"].as<String>();
          hasPassword = (pwd.length() > 0);
        }
      }
      readFile.close();
    }
  }
  
  // Escape strings for JSON
  String escapedServer = server;
  escapedServer.replace("\\", "\\\\");
  escapedServer.replace("\"", "\\\"");
  String escapedUsername = username;
  escapedUsername.replace("\\", "\\\\");
  escapedUsername.replace("\"", "\\\"");
  
  String response = "{\"server\": \"" + escapedServer + "\", ";
  response += "\"port\": " + String(port) + ", ";
  response += "\"username\": \"" + escapedUsername + "\", ";
  response += "\"has_password\": " + String(hasPassword ? "true" : "false") + ", ";
  response += "\"enable\": " + String(enable ? "true" : "false") + ", ";
  response += "\"connected\": " + String(client.connected() ? "true" : "false") + "}";
  request->send(200, "application/json", response);
}

/**
 * Get MQTT connection status and topic information
 * GET /api/mqtt/status
 * 
 * @param request request made by the client
 */
void mqttStatus(AsyncWebServerRequest *request) {
  String response = "{";
  response += "\"enabled\": " + String(MqttSettings.enable ? "true" : "false") + ",";
  response += "\"connected\": " + String(client.connected() ? "true" : "false") + ",";
  response += "\"server\": \"" + MqttSettings.server + "\",";
  response += "\"port\": " + String(MqttSettings.port) + ",";
  response += "\"client_id\": \"" + String(HOSTNAME) + "\",";
  response += "\"topics\": {";
  response += "\"checkIn\": \"hunter/" + String(HOSTNAME) + "/checkIn\",";
  response += "\"result\": \"hunter/" + String(HOSTNAME) + "/result\",";
  response += "\"zone\": \"hunter/" + String(HOSTNAME) + "/zone/[1-48]\",";
  response += "\"program\": \"hunter/" + String(HOSTNAME) + "/program/[1-4]\"";
  response += "}";
  
  if (!client.connected() && MqttSettings.enable) {
    response += ",\"connection_state\": " + String(client.state());
    response += ",\"connection_state_name\": \"";
    switch (client.state()) {
      case -4: response += "MQTT_CONNECTION_TIMEOUT"; break;
      case -3: response += "MQTT_CONNECTION_LOST"; break;
      case -2: response += "MQTT_CONNECT_FAILED"; break;
      case -1: response += "MQTT_DISCONNECTED"; break;
      case 1: response += "MQTT_CONNECT_BAD_PROTOCOL"; break;
      case 2: response += "MQTT_CONNECT_BAD_CLIENT_ID"; break;
      case 3: response += "MQTT_CONNECT_UNAVAILABLE"; break;
      case 4: response += "MQTT_CONNECT_BAD_CREDENTIALS"; break;
      case 5: response += "MQTT_CONNECT_UNAUTHORIZED"; break;
      default: response += "UNKNOWN"; break;
    }
    response += "\"";
  }
  
  response += "}";
  
  request->send(200, "application/json", response);
}