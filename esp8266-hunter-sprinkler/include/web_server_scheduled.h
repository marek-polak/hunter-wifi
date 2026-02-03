#ifndef WEB_SERVER_SCHEDULED_H
#define WEB_SERVER_SCHEDULED_H

#include <ESPAsyncWebServer.h>

void startZone(byte num, byte time, String webhook);
void startProgram(byte num, String webhook);
void stopZone(byte num, String webhook);

/** Publish stop result to MQTT when zone timer expires (call from loop). */
void checkScheduledZoneStop();

#endif