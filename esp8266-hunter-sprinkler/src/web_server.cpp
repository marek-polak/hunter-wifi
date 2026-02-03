/**
 * This file includes sets up the web server to serve teh API and webpage
 * to esily control the Hunter Irrigation System.
 *
 * First version: July 2020 - Eloi Codina Torras
 */
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>

#include <global_config.h>
#include <ota.h>
#include <telnet_logger.h>
#include <web_interface.h>
#include <web_server.h>
#include <web_server_api.h>

/**
 * Configure the paths and start the server.
 */
void setup_WebServer() {
  if (!MDNS.begin(HOSTNAME)) {
    Logger.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Logger.println("mDNS responder started");
  setup_APIRoutes();
  server.begin();
  MDNS.setHostname(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
}

/**
 * Set up all the API routes
 */
void setup_APIRoutes() {
  // API routes - exact matches for routes without parameters
  // Routes with parameters (e.g., /api/start/zone/3) are handled in handleNotFound
  server.on("/api/start/zone", HTTP_GET, startZoneRequest);
  server.on("/api/start/program", HTTP_GET, startProgramRequest);
  server.on("/api/stop/zone", HTTP_GET, stopZoneRequest);
  server.on("/api/config/mqtt", HTTP_POST, updateMqttConfig);
  server.on("/api/config/mqtt", HTTP_GET, mqttConfigStatus);
  server.on("/api/config/syslog", HTTP_POST, updateSyslogConfig);
  server.on("/api/config/syslog", HTTP_GET, syslogConfigStatus);
  server.on("/api/config/timezone", HTTP_POST, updateTimezoneConfig);
  server.on("/api/mqtt/status", HTTP_GET, mqttStatus);
  server.on("/api/time/status", HTTP_GET, timeStatus);
  server.on("/api/version", HTTP_GET, apiVersion);
  server.on("/api/start", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(400, "text/plain", "What should I start?");
  });
  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request) {
    String url = request->url();
    if (url == "/api" || url == "/api/") {
      request->send_P(403, "text/plain", "This is the API root");
      return;
    }
    if (url == "/api/version") {
      apiVersion(request);
      return;
    }
    request->send(404);
  });
  server.on("/", HTTP_GET,
            [](AsyncWebServerRequest *request) { request->redirect("/home"); });
  server.on("/home", HTTP_GET,
            [](AsyncWebServerRequest *request) { handleHomePage(request); });
  server.on("/update", HTTP_GET,
            [](AsyncWebServerRequest *request) { handleUpdate(request); });
  server.on("/zone", HTTP_GET,
            [](AsyncWebServerRequest *request) { handleRunZone(request); });
  server.on("/program", HTTP_GET,
            [](AsyncWebServerRequest *request) { handleRunProgram(request); });
  server.on("/settings", HTTP_GET,
            [](AsyncWebServerRequest *request) { handleSettings(request); });
  server.on(
      "/doUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String &filename, size_t index,
         uint8_t *data, size_t len, bool final) {
        handleDoUpdate(request, filename, index, data, len, final);
      });

  server.onNotFound(handleNotFound);
}

void handleNotFound(AsyncWebServerRequest *request) {
  String url = request->url();
  
  // Handle API routes with path parameters that weren't matched exactly
  if (url.startsWith("/api/start/zone/")) {
    startZoneRequest(request);
    return;
  }
  if (url.startsWith("/api/start/program/")) {
    startProgramRequest(request);
    return;
  }
  if (url.startsWith("/api/stop/zone/")) {
    stopZoneRequest(request);
    return;
  }

  Logger.printf("[Web] 404 Not found: %s\n", url.c_str());
  request->send(404);
}