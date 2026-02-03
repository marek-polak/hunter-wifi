#include "LittleFS.h"
#include <web_interface.h>
#include <ESPAsyncWebServer.h>
#include <telnet_logger.h>

void handleHomePage(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", String());
}

void handleRunZone(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/runzone.html", String());
}

void handleRunProgram(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/runprogram.html", String());
}

void handleSettings(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/settings.html", String());
}