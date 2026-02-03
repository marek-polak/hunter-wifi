/**
 * This file includes all the functions that are scheduled to run after
 * returning a response to the client. The client asks to do some action with
 * the Hunter Irrigation System and these functions contain the necessary logic
 * to perform it and send the result to a webhook.
 *
 * First version: July 2020 - Eloi Codina Torras
 */
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <HunterRoam.h>

#include <global_config.h>
#include <mqtt.h>
#include <telnet_logger.h>
#include <web_server_scheduled.h>

extern MQTT MqttSettings;

HunterRoam smartPort(REM_PIN);
HTTPClient http;
WiFiClient wifi;

/** Scheduled zone-stop: publish stop result to MQTT when zone timer expires */
static byte scheduled_stop_zone = 0;
static unsigned long scheduled_stop_millis = 0;

void setScheduledZoneStop(byte zone, unsigned int durationMinutes) {
  scheduled_stop_zone = zone;
  scheduled_stop_millis = millis() + (unsigned long)durationMinutes * 60000UL;
}

void clearScheduledZoneStop() {
  scheduled_stop_zone = 0;
}

void checkScheduledZoneStop() {
  if (scheduled_stop_zone == 0 || !MqttSettings.enable) return;
  if (millis() < scheduled_stop_millis) return;
  Logger.printf("[Zone] Zone %d timer expired\n", scheduled_stop_zone);
  String payload = "{\"action_num\":2,\"zone\":" + String(scheduled_stop_zone) + "}";
  mqttPublishResult(payload.c_str());
  scheduled_stop_zone = 0;
}

/**
 * Send the result of the action performed on the Hunter Irrigation System to a
 * webhook given by the client when it made the petition.
 * MQTT result topic is published separately with minimal JSON in startZone/stopZone.
 *
 * @param toSend information that will be sent. Probably JSON formatted
 * @param webhook url where the information will be sent
 */
void sendResult(String toSend, String webhook) {
  http.begin(wifi, webhook);
  http.addHeader("Content-Type", "application/json");
  http.POST(toSend);
  http.end();
}

/**
 * Start a zone and send the result of the action to the webhook given by the
 * client.
 *
 * @param num zone number
 * @param time irrigation minutes
 * @param webhook url where the result will be sent
 */
void startZone(byte num, byte time, String webhook) {
  Logger.printf("[Zone] Starting zone %d for %d minutes...\n", num, time);
  
  byte result = smartPort.startZone(num, time);
  Logger.printf("[Zone] HunterRoam returned: %d (%s)\n", result, smartPort.errorHint(result).c_str());

  String toSend = "{";
  toSend = toSend + "\"action\": \"Start zone " + (String)num + " for " +
           (String)time + " minutes.\",";
  toSend = toSend + "\"action_num\": 1,";
  toSend = toSend + "\"zone\": " + (String)num + ",";
  toSend = toSend + "\"result\": \"" + smartPort.errorHint(result) + "\",";
  toSend = toSend + "\"result_num\": " + (String)result;
  toSend = toSend + "}";

  sendResult(toSend, webhook);

  if (MqttSettings.enable) {
    String mqttPayload = "{\"action_num\":1,\"zone\":" + String(num) + ",\"time\":" + String(time) + "}";
    mqttPublishResult(mqttPayload.c_str());
    if (time > 0) {
      setScheduledZoneStop(num, time);
    }
  }
}

/**
 * Stop a zone and send the result of the action to the webhook given by the
 * client.
 *
 * @param num zone number
 * @param webhook url where the result will be sent
 */
void stopZone(byte num, String webhook) {
  Logger.printf("[Zone] Stopping zone %d...\n", num);
  clearScheduledZoneStop();
  byte result = smartPort.stopZone(num);
  Logger.printf("[Zone] HunterRoam returned: %d (%s)\n", result, smartPort.errorHint(result).c_str());

  String toSend = "{";
  toSend = toSend + "\"action\": \"Stop zone " + (String)num + ".\",";
  toSend = toSend + "\"action_num\": 2,";
  toSend = toSend + "\"zone\": " + (String)num + ",";
  toSend = toSend + "\"result\": \"" + smartPort.errorHint(result) + "\",";
  toSend = toSend + "\"result_num\": " + (String)result;
  toSend = toSend + "}";

  sendResult(toSend, webhook);

  if (MqttSettings.enable) {
    String mqttPayload = "{\"action_num\":2,\"zone\":" + String(num) + "}";
    mqttPublishResult(mqttPayload.c_str());
  }
}

/**
 * Start a program and send the result of the action to the webhook given by the
 * client.
 *
 * @param num program number
 * @param webhook url where the result will be sent
 */
void startProgram(byte num, String webhook) {
  Logger.printf("[Program] Starting program %d...\n", num);
  byte result = smartPort.startProgram(num);
  Logger.printf("[Program] HunterRoam returned: %d (%s)\n", result, smartPort.errorHint(result).c_str());

  String toSend = "{";
  toSend = toSend + "\"action\": \"Start program " + (String)num + ".\",";
  toSend = toSend + "\"action_num\": 3,";
  toSend = toSend + "\"program\": " + (String)num + ",";
  toSend = toSend + "\"result\": \"" + smartPort.errorHint(result) + "\",";
  toSend = toSend + "\"result_num\": " + (String)result;
  toSend = toSend + "}";

  sendResult(toSend, webhook);

  if (MqttSettings.enable) {
    String mqttPayload = "{\"action_num\":3,\"program\":" + String(num) + "}";
    mqttPublishResult(mqttPayload.c_str());
  }
}