// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <Beelance.h>

#include <LittleFS.h>

#include <string>

#define TAG "BEELANCE"

void Beelance::BeelanceClass::_initConfig() {
  logger.info(TAG, "Initializing Config System...");

  config.begin("BEELANCE");

  config.configure(KEY_ADMIN_PASSWORD);
  config.configure(KEY_AP_MODE_ENABLE, "true");
  config.configure(KEY_BEEHIVE_NAME, Mycila::AppInfo.defaultHostname);
  config.configure(KEY_DEBUG_ENABLE, "false");
  config.configure(KEY_HX711_CLOCK_PIN, std::to_string(BEELANCE_HX711_CLOCK_PIN));
  config.configure(KEY_HX711_DATA_PIN, std::to_string(BEELANCE_HX711_DATA_PIN));
  config.configure(KEY_HX711_OFFSET, "0");
  config.configure(KEY_HX711_SCALE, "1");
  config.configure(KEY_MODEM_APN);
  config.configure(KEY_MODEM_BANDS_LTE_M, "1,3,8,20,28");
  config.configure(KEY_MODEM_BANDS_NB_IOT, "3,8,20");
  config.configure(KEY_MODEM_GPS_SYNC_TIMEOUT, std::to_string(MYCILA_MODEM_GPS_SYNC_TIMEOUT));
  config.configure(KEY_MODEM_MODE, "AUTO");
  config.configure(KEY_MODEM_PIN);
  config.configure(KEY_NIGHT_START_TIME, "23:00");
  config.configure(KEY_NIGHT_STOP_TIME, "05:00");
  config.configure(KEY_PREVENT_SLEEP_ENABLE, "true");
  config.configure(KEY_PMU_CHARGING_CURRENT, "500");
  config.configure(KEY_SEND_INTERVAL, "3600");
  config.configure(KEY_SEND_URL);
  config.configure(KEY_TEMPERATURE_PIN, std::to_string(BEELANCE_TEMPERATURE_PIN));
  config.configure(KEY_TIMEZONE_INFO, "CET-1CEST,M3.5.0,M10.5.0/3");
  config.configure(KEY_WIFI_PASSWORD);
  config.configure(KEY_WIFI_SSID);

  // init logging
  configureDebugTask.forceRun();
}
