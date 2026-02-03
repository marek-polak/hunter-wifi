# Name the LittleFS image so it contains "spiffs" in the filename.
# The device OTA handler flashes the filesystem partition only when
# the uploaded filename contains "spiffs" (see ota.cpp).
Import("env")
env.Replace(ESP8266_FS_IMAGE_NAME="littlefs.spiffs")
