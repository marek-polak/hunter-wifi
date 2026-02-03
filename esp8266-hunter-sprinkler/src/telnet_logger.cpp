#include <telnet_logger.h>
#include <global_config.h>
#include <mqtt.h>
#include <time_sync.h>
#include <stdarg.h>

extern MQTT MqttSettings;
extern PubSubClient client;

// RFC 3164 BSD syslog: <PRI>TIMESTAMP HOSTNAME TAG: MESSAGE
#define SYSLOG_PRI "<134>"  // facility 16 (user), severity 6 (info)
#define SYSLOG_TAG "hunterwifi"

TelnetLogger Logger;

TelnetLogger::TelnetLogger() {
    server = nullptr;
    syslog_port = 514;
    syslog_enable = false;
    syslog_line_len = 0;
    enableTimestamps = true;
    atLineStart = true;
}

void TelnetLogger::setSyslogConfig(const String& server, int port, bool enable) {
    syslog_server = server;
    syslog_port = port;
    syslog_enable = enable;
}

void TelnetLogger::flushSyslogLine() {
    if (!syslog_enable || syslog_server.length() == 0 || syslog_line_len == 0) {
        syslog_line_len = 0;
        return;
    }
    syslog_line_buf[syslog_line_len] = '\0';
    
    // Get current timestamp in BSD syslog format
    String timestamp = this->getSyslogTimestamp();
    
    // BSD: <PRI>TIMESTAMP HOSTNAME TAG: MESSAGE
    char packet[512];
    snprintf(packet, sizeof(packet), "%s%s %s %s: %s",
             SYSLOG_PRI, timestamp.c_str(), HOSTNAME, SYSLOG_TAG, syslog_line_buf);
    
    syslog_udp.beginPacket(syslog_server.c_str(), (uint16_t)syslog_port);
    syslog_udp.write((const uint8_t*)packet, strlen(packet));
    syslog_udp.endPacket();
    syslog_line_len = 0;
}

void TelnetLogger::begin() {
    server = new WiFiServer(TELNET_PORT);
    server->begin();
    server->setNoDelay(true);
    
    Serial.println("[Telnet] Server started on port 23");
    Serial.print("[Telnet] Connect with: telnet ");
    Serial.println(WiFi.localIP());
}

void TelnetLogger::setTimestamps(bool enable) {
    enableTimestamps = enable;
}

String TelnetLogger::getTimestamp() {
    if (!enableTimestamps || !timeSync.isSynced()) {
        return "";
    }
    return "[" + timeSync.getFormattedTime() + "] ";
}

// Get BSD syslog formatted timestamp (MMM DD HH:MM:SS)
String TelnetLogger::getSyslogTimestamp() {
    if (!timeSync.isSynced()) {
        return "Jan  1 00:00:00";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[16];
    strftime(buffer, sizeof(buffer), "%b %e %H:%M:%S", &timeinfo);
    return String(buffer);
}

void TelnetLogger::printBanner(WiFiClient& c) {
    c.println("\n=== Hunter WiFi Controller ===");
    
    // Firmware info
    c.print("Firmware ");
    c.print(FIRMWARE_VERSION);
    c.print(" (built ");
    c.print(BUILD_TIME);
    c.println(")");
    
    // Time/NTP status
    c.print("NTP: ");
    if (timeSync.isSynced()) {
        c.print("Synced - ");
        c.println(timeSync.getFormattedTime());
    } else {
        c.println("Not synced");
    }
    
    // MQTT status
    c.print("MQTT: ");
    if (MqttSettings.enable) {
        if (client.connected()) {
            c.print("Connected to ");
        } else {
            c.print("Disconnected (");
        }
        c.print(MqttSettings.server);
        c.print(":");
        c.print(MqttSettings.port);
        if (!client.connected()) {
            c.print(")");
        }
        c.println();
    } else {
        c.println("Disabled");
    }
    
    // Syslog status
    c.print("Syslog: ");
    if (syslog_enable && syslog_server.length() > 0) {
        c.print("Enabled - ");
        c.print(syslog_server);
        c.print(":");
        c.println(syslog_port);
    } else {
        c.println("Disabled");
    }
    
    c.println("Telnet: Active");
    c.println("==============================\n");
}

void TelnetLogger::handle() {
    if (!server) return;
    
    // Check for new clients
    if (server->hasClient()) {
        // Find free slot
        bool connected = false;
        for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
            if (!clients[i] || !clients[i].connected()) {
                if (clients[i]) clients[i].stop();
                clients[i] = server->accept();
                printBanner(clients[i]);
                Serial.printf("[Telnet] New client #%d connected from %s\n", 
                             i, clients[i].remoteIP().toString().c_str());
                connected = true;
                break;
            }
        }
        if (!connected) {
            // No free slot, reject the client
            WiFiClient rejectClient = server->accept();
            rejectClient.println("ERROR: Maximum clients connected");
            rejectClient.stop();
            Serial.println("[Telnet] Client rejected - max connections reached");
        }
    }
    
    // Clean up disconnected clients
    for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (clients[i] && !clients[i].connected()) {
            Serial.printf("[Telnet] Client #%d disconnected\n", i);
            clients[i].stop();
        }
    }
}

// Raw output to Serial and telnet clients (no timestamp handling)
void TelnetLogger::output(const char* str) {
    Serial.print(str);
    for (int i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            clients[i].print(str);
        }
    }
}

// Main print function - handles timestamps and syslog
void TelnetLogger::print(const char* str) {
    const char* p = str;
    
    while (*p) {
        // If at line start and timestamps enabled, prepend timestamp
        if (atLineStart && enableTimestamps && timeSync.isSynced()) {
            String ts = "[" + timeSync.getFormattedTime() + "] ";
            output(ts.c_str());
            atLineStart = false;
        }
        
        // Find end of current segment (up to newline or end)
        const char* lineEnd = p;
        while (*lineEnd && *lineEnd != '\n') {
            lineEnd++;
        }
        
        // Output the segment
        size_t len = lineEnd - p;
        if (len > 0) {
            char segment[256];
            if (len >= sizeof(segment)) len = sizeof(segment) - 1;
            strncpy(segment, p, len);
            segment[len] = '\0';
            output(segment);
            
            // Also buffer for syslog
            if (syslog_enable && syslog_server.length() > 0) {
                for (size_t i = 0; i < len; i++) {
                    if (syslog_line_len < SYSLOG_LINE_MAX - 1) {
                        syslog_line_buf[syslog_line_len++] = segment[i];
                    }
                }
            }
        }
        
        // Handle newline
        if (*lineEnd == '\n') {
            output("\r\n");
            if (syslog_enable && syslog_server.length() > 0) {
                flushSyslogLine();
            }
            atLineStart = true;
            p = lineEnd + 1;
        } else {
            p = lineEnd;
        }
    }
}

void TelnetLogger::print(const String& str) {
    print(str.c_str());
}

void TelnetLogger::print(int val) {
    snprintf(buffer, sizeof(buffer), "%d", val);
    print(buffer);
}

void TelnetLogger::print(unsigned int val) {
    snprintf(buffer, sizeof(buffer), "%u", val);
    print(buffer);
}

void TelnetLogger::print(long val) {
    snprintf(buffer, sizeof(buffer), "%ld", val);
    print(buffer);
}

void TelnetLogger::print(unsigned long val) {
    snprintf(buffer, sizeof(buffer), "%lu", val);
    print(buffer);
}

void TelnetLogger::print(double val, int decimals) {
    snprintf(buffer, sizeof(buffer), "%.*f", decimals, val);
    print(buffer);
}

void TelnetLogger::println(const char* str) {
    print(str);
    print("\n");
}

void TelnetLogger::println(const String& str) {
    println(str.c_str());
}

void TelnetLogger::println(int val) {
    snprintf(buffer, sizeof(buffer), "%d", val);
    print(buffer);
    print("\n");
}

void TelnetLogger::println(unsigned int val) {
    snprintf(buffer, sizeof(buffer), "%u", val);
    print(buffer);
    print("\n");
}

void TelnetLogger::println() {
    print("\n");
}

void TelnetLogger::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    print(buffer);
}
