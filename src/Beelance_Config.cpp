// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <LittleFS.h>

#define TAG "BEELANCE"

void Beelance::BeelanceClass::_initConfig() {
  Mycila::Logger.info(TAG, "Initializing Config System...");

  Mycila::Config.begin(23);

  String hostname = String(Mycila::AppInfo.name + "-" + Mycila::AppInfo.id);
  hostname.toLowerCase();

  Mycila::Config.configure(KEY_ADMIN_PASSWORD);
  Mycila::Config.configure(KEY_AP_MODE_ENABLE, "true");
  Mycila::Config.configure(KEY_BEEHIVE_NAME, hostname);
  Mycila::Config.configure(KEY_CAPTURE_PORTAL_TIMEOUT, String(ESPCONNECT_PORTAL_TIMEOUT));
  Mycila::Config.configure(KEY_DEBUG_ENABLE, "false");
  Mycila::Config.configure(KEY_HOSTNAME, hostname);
  Mycila::Config.configure(KEY_MODEM_APN);
  Mycila::Config.configure(KEY_MODEM_BANDS_LTE_M, "1,3,8,20,28");
  Mycila::Config.configure(KEY_MODEM_BANDS_NB_IOT, "3,8,20");
  Mycila::Config.configure(KEY_MODEM_GPS_SYNC_TIMEOUT, String(MYCILA_MODEM_GPS_SYNC_TIMEOUT));
  Mycila::Config.configure(KEY_MODEM_MODE, "AUTO");
  Mycila::Config.configure(KEY_MODEM_PIN);
  Mycila::Config.configure(KEY_NO_SLEEP_ENABLE, "true");
  Mycila::Config.configure(KEY_SEND_INTERVAL, "3600");
  Mycila::Config.configure(KEY_SEND_URL);
  Mycila::Config.configure(KEY_TEMPERATURE_ENABLE, "true");
  Mycila::Config.configure(KEY_TEMPERATURE_PIN, String(BEELANCE_TEMPERATURE_PIN));
  Mycila::Config.configure(KEY_TIMEZONE_INFO, "CET-1CEST,M3.5.0,M10.5.0/3");
  Mycila::Config.configure(KEY_WIFI_CONNECTION_TIMEOUT, String(ESPCONNECT_CONNECTION_TIMEOUT));
  Mycila::Config.configure(KEY_WIFI_PASSWORD);
  Mycila::Config.configure(KEY_WIFI_SSID);
  Mycila::Config.configure(KEY_BOARD_LED, "true");

  // init logging
  configureDebugTask.forceRun();
  setenv("TZ", Mycila::Config.get(KEY_TIMEZONE_INFO).c_str(), 1);
}
