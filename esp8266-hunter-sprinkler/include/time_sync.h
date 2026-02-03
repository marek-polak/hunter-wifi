#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>
#include <time.h>

class TimeSync {
private:
    String timezone;
    bool timeSynced;
    unsigned long lastSyncAttempt;
    static const unsigned long SYNC_INTERVAL = 3600000; // 1 hour in milliseconds
    
public:
    TimeSync();
    void begin(const String& tz);
    void setTimezone(const String& tz);
    String getTimezone();
    bool isSynced();
    void loop();
    String getFormattedTime();
    String getISO8601Time();
};

extern TimeSync timeSync;

#endif
