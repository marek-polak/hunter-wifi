# Telnet Logging

The Hunter WiFi Controller now supports remote logging via Telnet, allowing you to monitor device logs over the network without needing a USB connection.

## How It Works

The device runs a telnet server on port 23. All log messages are sent to:
- The USB serial port (for local debugging)
- All connected telnet clients (for remote monitoring)
- An optional remote syslog server (when enabled)

## Connecting

### Using telnet command (Mac/Linux/Windows)

```bash
telnet <device-ip-address>
```

For example:
```bash
telnet 192.168.1.100
```

### Alternative Tools

You can also use:
- **PuTTY** (Windows) - Set connection type to "Telnet"
- **nc (netcat)**: `nc <device-ip> 23`
- Any telnet client application

## Features

- **Multiple Connections**: Up to 2 simultaneous telnet clients can connect
- **Real-time Logging**: See all system events as they happen
- **No USB Required**: Monitor your device remotely from anywhere on your network
- **Automatic Reconnection**: If you disconnect, simply reconnect - no restart needed

## What You'll See

When you connect, you'll see:
- MQTT connection status and messages
- Zone and program commands
- WiFi status
- Configuration changes
- System errors and warnings
- OTA update progress

## Example Session

```
$ telnet 192.168.1.100
Trying 192.168.1.100...
Connected to 192.168.1.100.

=== Hunter WiFi Controller ===
Telnet logging active
==============================

[MQTT] ✓ Connected successfully!
[MQTT] Subscribed to 52 topics (48 zones + 4 programs)
[MQTT] Message received on topic: hunter/X-CORE/zone/1
[MQTT] Payload (22 bytes): {"action":"start","time":5}
[MQTT] Starting zone 1 for 5 minutes
```

## Technical Details

- **Port**: 23 (standard telnet port)
- **Max Clients**: 2 concurrent connections
- **Protocol**: Plain text telnet
- **Buffer Size**: 512 bytes per message
- **Line Endings**: CR+LF (Windows-style)

## Remote logging (syslog)

You can send device logs to a remote syslog server (e.g. Synology Log Center) so they are stored centrally.

### Configuration

- **config.json**: Set `syslog_server` (IP or hostname), `syslog_port` (default 514), and `syslog_enable` (true/false). Default server when unset is `172.16.1.20`.
- **API**: `POST /api/config/syslog` with parameters `server`, optional `port`, and optional `enable`. See [API reference](api.md) and [ENDPOINTS.md](../../ENDPOINTS.md).

### Synology Log Center

Create a rule with:

- **Log format**: BSD
- **Transfer protocol**: UDP
- **Port**: 514
- **Rule name**: e.g. `hunterwifi` (the device sends messages with tag `hunterwifi`)

No parsing rule is required; the device sends valid BSD (RFC 3164) messages.

### Format and behaviour

- Logs are sent in **BSD syslog** format over UDP, one packet per log line.
- **Tag**: `hunterwifi` (so you can filter by this tag on the server).
- **Timestamps**: The device has no real-time clock by default, so messages use a placeholder timestamp. Accurate timestamps would require adding NTP support later.

## Troubleshooting

### Can't Connect

1. Check that the device is on the network:
   ```bash
   ping <device-ip>
   ```

2. Verify port 23 is not blocked by firewall

3. Ensure maximum clients (2) haven't been reached - wait for another client to disconnect

### Connection Drops

- This is normal if the device restarts
- Simply reconnect when the device is back online
- If you exceed the 2-client limit, the oldest connection may be dropped

## Security Note

Telnet transmits data in plain text. Only use on trusted local networks. The logs may contain configuration information but do not expose passwords (shown as "(set)" in logs).
