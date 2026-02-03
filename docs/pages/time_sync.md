# NTP Time Synchronization

The Hunter WiFi Controller now includes automatic time synchronization via NTP (Network Time Protocol), providing accurate timestamps for all logs and system events.

## Features

- **Automatic NTP sync** when WiFi connects
- **Configurable timezone** via API
- **Timestamps in all logs** - telnet, serial, and syslog
- **Periodic re-sync** every hour to maintain accuracy
- **Multiple NTP servers** for reliability (pool.ntp.org, time.nist.gov, time.google.com)

## Default Configuration

- **Default Timezone**: Argentina/Buenos Aires (UTC-3)
- **NTP Servers**: pool.ntp.org, time.nist.gov, time.google.com
- **Sync Interval**: 1 hour

## Configuration via API

### Get Current Time Status

**Endpoint**: `GET /api/time/status`

**Response**:
```json
{
  "synced": true,
  "timezone": "<-03>3",
  "current_time": "2026-01-28 15:30:45",
  "iso8601": "2026-01-28T15:30:45"
}
```

### Update Timezone

**Endpoint**: `POST /api/config/timezone`

**Parameters**:
- `timezone` (required): POSIX timezone string

**Example**:
```bash
curl -X POST http://192.168.1.100/api/config/timezone \
  -d "timezone=<-03>3"
```

**Response**:
```json
{
  "status": "Timezone configuration updated successfully",
  "timezone": "<-03>3",
  "current_time": "2026-01-28 15:30:45",
  "synced": true
}
```

## Common Timezone Strings

The device uses POSIX timezone format. Here are some common examples:

### Americas

| Location | Timezone String |
|----------|----------------|
| Argentina/Buenos Aires | `<-03>3` |
| US Eastern | `EST5EDT,M3.2.0,M11.1.0` |
| US Central | `CST6CDT,M3.2.0,M11.1.0` |
| US Mountain | `MST7MDT,M3.2.0,M11.1.0` |
| US Pacific | `PST8PDT,M3.2.0,M11.1.0` |
| Brazil/Sao Paulo | `<-03>3` |
| Mexico City | `CST6CDT,M4.1.0,M10.5.0` |
| Chile/Santiago | `<-04>4<-03>,M9.1.6/24,M4.1.6/24` |

### Europe

| Location | Timezone String |
|----------|----------------|
| UTC | `UTC0` |
| UK (GMT/BST) | `GMT0BST,M3.5.0/1,M10.5.0` |
| Central European (CET) | `CET-1CEST,M3.5.0,M10.5.0/3` |
| Eastern European (EET) | `EET-2EEST,M3.5.0/3,M10.5.0/4` |

### Asia & Pacific

| Location | Timezone String |
|----------|----------------|
| Japan | `JST-9` |
| China | `CST-8` |
| India | `IST-5:30` |
| Australia/Sydney | `AEST-10AEDT,M10.1.0,M4.1.0/3` |
| New Zealand | `NZST-12NZDT,M9.5.0,M4.1.0/3` |

## POSIX Timezone Format

The timezone string follows POSIX format:

```
std offset [dst [offset],start[/time],end[/time]]
```

- `std`: Standard time abbreviation
- `offset`: Hours offset from UTC (positive = west, negative = east)
- `dst`: Daylight saving time abbreviation (optional)
- `start/end`: When DST starts/ends (optional)

### Examples Explained

- `<-03>3`: UTC-3, no DST (Argentina)
- `EST5EDT,M3.2.0,M11.1.0`: EST (UTC-5), EDT (UTC-4), DST starts 2nd Sunday of March, ends 1st Sunday of November
- `CET-1CEST,M3.5.0,M10.5.0/3`: CET (UTC+1), CEST (UTC+2), DST starts last Sunday of March, ends last Sunday of October

## How It Works

1. **On Boot**: Device loads timezone from config.json (defaults to Argentina/Buenos Aires)
2. **After WiFi**: NTP sync starts automatically
3. **Sync Check**: Device waits for time sync (usually takes 1-5 seconds)
4. **Timestamps**: Once synced, all log messages include timestamps
5. **Re-sync**: Automatic re-sync every hour to maintain accuracy

## Viewing Timestamps

### Telnet Logs

When you connect via telnet, you'll see timestamps in all messages:

```
$ telnet 192.168.1.100
Connected to 192.168.1.100.

=== Hunter WiFi Controller ===
Telnet logging active
Current time: 2026-01-28 15:30:45
==============================

[2026-01-28 15:30:47] [MQTT] ✓ Connected successfully!
[2026-01-28 15:30:48] [MQTT] Subscribed to 52 topics
[2026-01-28 15:31:02] [MQTT] Message received on topic: hunter/X-CORE/zone/1
[2026-01-28 15:31:02] [MQTT] Starting zone 1 for 10 minutes
```

### Serial Monitor

USB serial also shows timestamps (if time is synced):

```
[2026-01-28 15:30:45] [Time] NTP sync started
[2026-01-28 15:30:47] [Time] Time synced successfully
[2026-01-28 15:30:47] [MQTT] ✓ Connected successfully!
```

## Troubleshooting

### Time Not Syncing

1. **Check WiFi**: Time sync requires internet connection
2. **Check NTP servers**: Make sure your network allows NTP (UDP port 123)
3. **Check logs**: Look for "[Time]" messages in telnet or serial

### Wrong Timezone

1. **Verify timezone string**: Use the correct POSIX format
2. **Update via API**: `POST /api/config/timezone` with correct timezone
3. **Check current setting**: `GET /api/time/status`

### Timestamps Missing

- Timestamps only appear **after** NTP sync completes
- Usually takes 1-5 seconds after WiFi connection
- Check sync status with: `GET /api/time/status`

## Configuration File

The timezone is stored in `/config.json` on the device:

```json
{
  "mqtt_server": "mqtt.example.com",
  "mqtt_port": 1883,
  "timezone": "<-03>3"
}
```

## Benefits

- 📅 **Accurate debugging** - Know exactly when events occurred
- 🔍 **Better troubleshooting** - Correlate logs with other systems
- 📊 **Log analysis** - Parse and analyze logs by timestamp
- 🌍 **Local time** - All timestamps in your local timezone
- ⏰ **No manual setup** - Automatic after first boot

## Technical Details

- Uses ESP8266's built-in NTP client
- Leverages hardware RTC for time keeping between syncs
- Configurable via `configTime()` Arduino function
- Timezone changes take effect immediately
- No external RTC module required
