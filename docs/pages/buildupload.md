# Build and upload

You can compile yourself the program and upload it to your board or use the precompiled binary under the releases section.

If using the WeMos D1 Mini, make sure to install the provided [Windows/Mac drivers](https://docs.wemos.cc/en/latest/ch340_driver.html).

## Precompiled binary

Use the *.bin* file under the releases section and `esptool.py` to upload the precompiled file to your board.

Usage:

    esptool.py --port <serial-port-of-ESP8266> write_flash -fm dio 0x00000 <precompiled-binary>.bin

You can find more documentation in the [*esptool.py* repository](https://github.com/espressif/esptool/).

## Build it yourself

If the above method didn't work or you want to make a few changes to the project before uploading to your board you can build the code yourself.

The `esp8266-hunter-sprinkler` folder is a PlatformIO project. Install Visual Studio Code (or any other IDE compatible with PlatformIO) and open the folder as a project.

After you make the changes, connect the board to your computer and upload it.

## Upload over the network (OTA)

You can upload both firmware and the web UI (filesystem) over Wi‑Fi from the **Update** page: open `http://<device>/update` in a browser (e.g. `http://X-CORE.local/update`).

**You need both.** Routes like `/settings` and `/api/version` live in the **firmware**; the HTML/CSS/JS (including the version footer) live in the **filesystem**. The device chooses which partition to flash from the **upload filename**: if the filename contains `spiffs` it flashes the filesystem; otherwise it flashes the app. Use the correct file and keep its filename when uploading.

**OTA checklist (both images):**

1. **Firmware (app):** Upload `firmware.bin` — filename must **not** contain `spiffs`. Wait for reboot.
2. **Filesystem (web UI):** Upload `littlefs.spiffs.bin` — filename must contain `spiffs`. Wait for reboot.
3. **Browser:** Hard refresh the page (Ctrl+Shift+R or Cmd+Shift+R) so you don’t see cached HTML.

If the web footer shows “Firmware ? — upload firmware.bin…” or telnet doesn’t show a version line, the app partition likely has old or wrong firmware; upload `firmware.bin` again with a name that does not contain `spiffs`.

### Firmware

1. From the **repository root**:
   ```bash
   pio run -d esp8266-hunter-sprinkler
   ```
2. Open the device’s **Update** page and upload:
   `esp8266-hunter-sprinkler/.pio/build/<env>/firmware.bin`  
   Use a filename that does **not** contain `spiffs` (e.g. keep `firmware.bin`).

### Filesystem (web UI)

The web UI (Home, Run Program, Run Zone, **Settings**, Update) lives on the LittleFS partition. The project is configured so the filesystem image is always built as `littlefs.spiffs.bin` (the device OTA handler flashes the FS partition only when the filename contains `spiffs`). No manual rename is needed.

1. From the **repository root**, build the filesystem image (packs everything in `esp8266-hunter-sprinkler/data/` into one image):
   ```bash
   pio run -d esp8266-hunter-sprinkler --target buildfs
   ```
   For a clean build (e.g. after adding or changing pages): `pio run -d esp8266-hunter-sprinkler --target clean` then run `buildfs` again.

2. Open the device’s **Update** page and upload:
   `esp8266-hunter-sprinkler/.pio/build/<env>/littlefs.spiffs.bin`  
   Wait for the device to reboot. Replace `<env>` with your environment (e.g. `nodemcuv2`).

