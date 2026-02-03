#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

#include <mqtt.h>
#include <telnet_logger.h>
#include <web_server_scheduled.h>
#include <wifi.h>

extern MQTT MqttSettings;

String device_hostname = HOSTNAME;
String TopicCheckIn = "hunter/" + device_hostname + "/checkIn";
String TopicResult = "hunter/" + device_hostname + "/result";
String TopicZone = "hunter/" + device_hostname + "/zone/";
String TopicProgram = "hunter/" + device_hostname + "/program/";

WiFiClient espClient;
PubSubClient client(espClient);

void mqtt_subscribe_topics() {
  Logger.println("[MQTT] Subscribing to topics...");
  int subscribed = 0;
  for (int i = 1; i <= 48; i++) {
    String zone_number = String(i);
    String TopicZoneFull = TopicZone + zone_number;
    if (client.subscribe(TopicZoneFull.c_str())) {
      subscribed++;
    }
  };
  for (int i = 1; i <= 4; i++) {
    String program_number = String(i);
    String TopicProgramFull = TopicProgram + program_number;
    if (client.subscribe(TopicProgramFull.c_str())) {
      subscribed++;
    }
  };
  Logger.printf("[MQTT] Subscribed to %d topics (48 zones + 4 programs)\n", subscribed);
}

void mqtt_connect(const char *MQTT_USER, const char *MQTT_PASSWORD) {
  static bool boot = true;
  
  Logger.println("[MQTT] Attempting to connect...");
  Logger.printf("[MQTT] Server: %s:%d\n", MqttSettings.server.c_str(), MqttSettings.port);
  
  // Handle authentication - support anonymous connections (empty username/password)
  bool hasAuth = (MQTT_USER != NULL && strlen(MQTT_USER) > 0);
  if (hasAuth) {
    Logger.printf("[MQTT] Username: %s\n", MQTT_USER);
    Logger.println("[MQTT] Password: (set)");
  } else {
    Logger.println("[MQTT] Authentication: Anonymous (no username/password)");
  }
  Logger.printf("[MQTT] Client ID: %s\n", HOSTNAME);
  
  // Attempt to connect - PubSubClient accepts NULL or empty strings for anonymous connection
  const char *user = (hasAuth && MQTT_USER != NULL) ? MQTT_USER : NULL;
  const char *pass = (hasAuth && MQTT_PASSWORD != NULL && strlen(MQTT_PASSWORD) > 0) ? MQTT_PASSWORD : NULL;
  
  if (client.connect(HOSTNAME, user, pass, TopicCheckIn.c_str(),
                     0, 0, "Dead Somewhere")) {
    Logger.println("[MQTT] ✓ Connected successfully!");
    
    // Once connected, publish an announcement...
    if (boot == false) {
      Logger.println("[MQTT] Publishing reconnection message...");
      if (client.publish(TopicCheckIn.c_str(), "Reconnected")) {
        Logger.println("[MQTT] ✓ Reconnection message published");
      } else {
        Logger.println("[MQTT] ✗ Failed to publish reconnection message");
      }
    }
    if (boot == true) {
      Logger.println("[MQTT] Publishing boot message...");
      if (client.publish(TopicCheckIn.c_str(), "Rebooted")) {
        Logger.println("[MQTT] ✓ Boot message published");
      } else {
        Logger.println("[MQTT] ✗ Failed to publish boot message");
      }
      boot = false;
    }
    // ... and resubscribe
    mqtt_subscribe_topics();
  } else {
    Logger.print("[MQTT] ✗ Connection failed! State: ");
    Logger.print(client.state());
    Logger.print(" (");
    switch (client.state()) {
      case -4: Logger.print("MQTT_CONNECTION_TIMEOUT"); break;
      case -3: Logger.print("MQTT_CONNECTION_LOST"); break;
      case -2: Logger.print("MQTT_CONNECT_FAILED"); break;
      case -1: Logger.print("MQTT_DISCONNECTED"); break;
      case 1: Logger.print("MQTT_CONNECT_BAD_PROTOCOL"); break;
      case 2: Logger.print("MQTT_CONNECT_BAD_CLIENT_ID"); break;
      case 3: Logger.print("MQTT_CONNECT_UNAVAILABLE"); break;
      case 4: Logger.print("MQTT_CONNECT_BAD_CREDENTIALS"); break;
      case 5: Logger.print("MQTT_CONNECT_UNAUTHORIZED"); break;
      default: Logger.print("UNKNOWN"); break;
    }
    Logger.println(")");
  }
}

void mqttPublishResult(const char *toSend) {
  Logger.printf("[MQTT] Publishing result to %s: %s\n", TopicResult.c_str(), toSend);
  if (client.publish(TopicResult.c_str(), toSend)) {
    Logger.println("[MQTT] ✓ Result published successfully");
  } else {
    Logger.println("[MQTT] ✗ Failed to publish result");
  }
}

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  String newTopic = topic;
  payload[length] = '\0';
  String payloadStr = String((char *)payload);
  
  Logger.printf("[MQTT] Message received on topic: %s\n", topic);
  Logger.printf("[MQTT] Payload (%d bytes): %s\n", length, payloadStr.c_str());
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payloadStr);
  if (error) {
    Logger.print(F("[MQTT] ✗ deserializeJson() failed: "));
    Logger.println(error.f_str());
    return;
  }
  
  String action = doc["action"];
  int time = 0;
  if (action == "start") {
    time = doc["time"] | 0;
    if (time < 1 || time > 254) {
      Logger.printf("[MQTT] ✗ Invalid time for start: %d (use 1-254 minutes)\n", time);
      return;
    }
  }
  Logger.printf("[MQTT] Parsed action: %s, time: %d\n", action.c_str(), time);

  for (byte i = 1; i <= 48; i++) {
    String zone_number = String(i);
    String TopicZoneFull = TopicZone + zone_number;
    if (newTopic == TopicZoneFull.c_str()) {
      if (action == "start") {
        startZone(i, (byte)time, "");
      }
      if (action == "stop") {
        stopZone(i, "");
      }
      return;
    }
  };
  for (byte i = 1; i <= 4; i++) {
    String program_number = String(i);
    String TopicProgramFull = TopicProgram + program_number;
    if (newTopic == TopicProgramFull.c_str()) {
      if (action == "start") {
        startProgram(i, "");
      }
      return;
    }
  };
  Logger.println("[MQTT] ⚠ No matching topic handler found");
}