#include <time_sync.h>
#include <coredecls.h>
#include <TZ.h>

TimeSync timeSync;

TimeSync::TimeSync() {
    timezone = "<-03>3"; // Default to Argentina/Buenos Aires (UTC-3)
    timeSynced = false;
    lastSyncAttempt = 0;
}

void TimeSync::begin(const String& tz) {
    timezone = tz;
    
    // Configure NTP servers
    configTime(tz.c_str(), "pool.ntp.org", "time.nist.gov", "time.google.com");
    
    // Set callback for when time is synced
    settimeofday_cb([this]() {
        this->timeSynced = true;
        // Note: Can't use Logger here as it may cause issues in callback
    });
    
    lastSyncAttempt = millis();
}

void TimeSync::setTimezone(const String& tz) {
    timezone = tz;
    configTime(tz.c_str(), "pool.ntp.org", "time.nist.gov", "time.google.com");
}

String TimeSync::getTimezone() {
    return timezone;
}

bool TimeSync::isSynced() {
    return timeSynced && (time(nullptr) > 1000000000); // Reasonable time check
}

void TimeSync::loop() {
    // Periodically check if we need to re-sync
    unsigned long now = millis();
    if (now - lastSyncAttempt >= SYNC_INTERVAL) {
        lastSyncAttempt = now;
        // Time library handles periodic sync automatically
    }
}

String TimeSync::getFormattedTime() {
    if (!isSynced()) {
        return "Not synced";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

String TimeSync::getISO8601Time() {
    if (!isSynced()) {
        return "1970-01-01T00:00:00Z";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    return String(buffer);
}
