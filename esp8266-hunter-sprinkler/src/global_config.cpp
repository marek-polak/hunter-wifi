#include <Arduino.h>

#include <global_config.h>
#include <telnet_logger.h>

bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback() {
  Logger.println("Should save config");
  shouldSaveConfig = true;
}