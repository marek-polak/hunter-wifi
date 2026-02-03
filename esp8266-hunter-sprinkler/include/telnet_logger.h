#ifndef TELNET_LOGGER_H
#define TELNET_LOGGER_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define MAX_TELNET_CLIENTS 2
#define TELNET_PORT 23
#define SYSLOG_LINE_MAX 256

class TelnetLogger {
private:
    WiFiServer* server;
    WiFiClient clients[MAX_TELNET_CLIENTS];
    char buffer[512];
    String syslog_server;
    int syslog_port;
    bool syslog_enable;
    WiFiUDP syslog_udp;
    char syslog_line_buf[SYSLOG_LINE_MAX];
    size_t syslog_line_len;
    bool enableTimestamps;
    bool atLineStart;  // Track if we're at the start of a new line
    void flushSyslogLine();
    String getTimestamp();
    String getSyslogTimestamp();
    void output(const char* str);  // Raw output without timestamp handling
    void printBanner(WiFiClient& client);

public:
    TelnetLogger();
    void begin();
    void setSyslogConfig(const String& server, int port, bool enable);
    void setTimestamps(bool enable);
    void handle();
    
    // Getters for status display
    bool isSyslogEnabled() const { return syslog_enable; }
    String getSyslogServer() const { return syslog_server; }
    int getSyslogPort() const { return syslog_port; }
    void print(const char* str);
    void print(const String& str);
    void print(int val);
    void print(unsigned int val);
    void print(long val);
    void print(unsigned long val);
    void print(double val, int decimals = 2);
    void println(const char* str);
    void println(const String& str);
    void println(int val);
    void println(unsigned int val);
    void println();
    void printf(const char* format, ...);
};

// Global logger instance
extern TelnetLogger Logger;

#endif
