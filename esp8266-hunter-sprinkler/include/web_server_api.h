#ifndef WEB_SERVER_API_H
#define WEB_SERVER_API_H

#include <ESPAsyncWebServer.h>

void startZoneRequest(AsyncWebServerRequest *request);
void startProgramRequest(AsyncWebServerRequest *request);
void stopZoneRequest(AsyncWebServerRequest *request);
void updateMqttConfig(AsyncWebServerRequest *request);
void updateSyslogConfig(AsyncWebServerRequest *request);
void updateTimezoneConfig(AsyncWebServerRequest *request);
void mqttStatus(AsyncWebServerRequest *request);
void mqttConfigStatus(AsyncWebServerRequest *request);
void timeStatus(AsyncWebServerRequest *request);
void syslogConfigStatus(AsyncWebServerRequest *request);
void apiVersion(AsyncWebServerRequest *request);
int getIdFromURL(AsyncWebServerRequest *request, String root);

#endif