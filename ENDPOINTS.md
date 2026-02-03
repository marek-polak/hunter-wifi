# Available HTTP Endpoints

## Web Interface Pages (requires LittleFS filesystem upload)

These endpoints serve HTML pages from the LittleFS filesystem. **You need to upload the filesystem first** using:
```bash
python3.13 -m platformio run --target uploadfs
```

- `GET /` - Redirects to `/home`
- `GET /home` - Serves `index.html` (main dashboard)
- `GET /zone` - Serves `runzone.html` (zone control page)
- `GET /program` - Serves `runprogram.html` (program control page)
- `GET /settings` - Serves `settings.html` (timezone/NTP and remote syslog settings)
- `GET /update` - Serves `update.html` (OTA update page)
- `POST /doUpdate` - Handles OTA firmware updates

## API Endpoints

### API Root
- `GET /api` - Returns: `"This is the API root"` (403 status)

### Zone Control
- `GET /api/start/zone/[ZONE-NUMBER]?time=[MINUTES]&result=[WEBHOOK_URL]`
  - Starts a zone for specified minutes
  - **Required parameter**: `time` (watering duration in minutes)
  - **Optional parameter**: `result` (webhook URL to receive operation result)
  - **Example**: `/api/start/zone/3?time=10`
  - Returns JSON with status

- `GET /api/stop/zone/[ZONE-NUMBER]?result=[WEBHOOK_URL]`
  - Stops a running zone
  - **Optional parameter**: `result` (webhook URL)
  - **Example**: `/api/stop/zone/3`
  - Returns JSON with status

### Program Control
- `GET /api/start/program/[PROGRAM-NUMBER]?result=[WEBHOOK_URL]`
  - Starts a program (1-4, where 1=A, 2=B, 3=C, 4=D)
  - **Optional parameter**: `result` (webhook URL)
  - **Example**: `/api/start/program/2` (starts program B)
  - Returns JSON with status

### Configuration
- `POST /api/config/mqtt` - Update MQTT configuration
  - **Required parameter**: `server` (MQTT server address)
  - **Optional parameters**: 
    - `port` (MQTT port, default: 1883)
    - `username` (MQTT username)
    - `password` (MQTT password)
    - `enable` (Enable MQTT: "1" or "true" to enable, "0" or "false" to disable)
  - **Example**: 
    ```bash
    curl -X POST "http://X-CORE.local/api/config/mqtt" \
      -d "server=raspi3b.local&port=1883&username=mqtt_user&password=secret&enable=1"
    ```
  - Saves configuration to `/config.json` and reconnects MQTT if enabled
  - Returns JSON with updated configuration

- `GET /api/config/syslog` - Get current syslog configuration (for web UI)
  - Returns JSON: `server`, `port`, `enable`

- `POST /api/config/syslog` - Update remote syslog configuration
  - **Required parameter**: `server` (syslog server IP or hostname)
  - **Optional parameters**:
    - `port` (syslog port, default: 514)
    - `enable` (Enable remote syslog: "1" or "true" to enable, "0" or "false" to disable)
  - **Example**:
    ```bash
    curl -X POST "http://X-CORE.local/api/config/syslog" \
      -d "server=172.16.1.20&port=514&enable=1"
    ```
  - Saves configuration to `/config.json`; takes effect immediately (no reboot)
  - Logs are sent in BSD (RFC 3164) format over UDP with tag `hunterwifi`

- `GET /api/time/status` - Get current time and timezone status
  - Returns JSON: `synced`, `timezone`, `current_time`, `iso8601` (when synced)

- `GET /api/version` - Get firmware version and build info
  - Returns JSON: `version`, `build` (compile date/time)

- `POST /api/config/timezone` - Update timezone (NTP)
  - **Required parameter**: `timezone` (POSIX timezone string, e.g. `CET-1CEST,M3.5.0,M10.5.0/3` or `UTC0`)
  - Saves to `/config.json` and applies immediately

### Other API Endpoints
- `GET /api/start` - Returns: `"What should I start?"` (400 status)

## Notes

### 404 on /home
If you get a 404 when accessing `/home`, the filesystem hasn't been uploaded. The HTML files exist in the `data/` directory but need to be uploaded to the ESP8266's LittleFS filesystem.

## Testing Endpoints

You can test the API endpoints using curl:

```bash
# Start zone 3 for 10 minutes
curl "http://X-CORE.local/api/start/zone/3?time=10"

# Stop zone 3
curl "http://X-CORE.local/api/stop/zone/3"

# Start program 2 (Hunter's program B)
curl "http://X-CORE.local/api/start/program/2"

# Update MQTT configuration
curl -X POST "http://X-CORE.local/api/config/mqtt" \
  -d "server=raspi3b.local&port=1883&username=mqtt_user&password=secret&enable=1"

# Update syslog configuration (e.g. Synology Log Center)
curl -X POST "http://X-CORE.local/api/config/syslog" \
  -d "server=172.16.1.20&port=514&enable=1"
```

Replace `X-CORE.local` with the device's IP address or mDNS hostname.

