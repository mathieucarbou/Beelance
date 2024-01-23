// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <LittleFS.h>

#define TAG "BEELANCE"

void Beelance::BeelanceClass::_initConfig() {
  Mycila::Logger.info(TAG, "Initializing Config System...");

  Mycila::Config.begin(16);

  String hostname = String(Mycila::AppInfo.name + "-" + Mycila::AppInfo.id);
  hostname.toLowerCase();

  /// others
  Mycila::Config.configure(KEY_HOSTNAME, hostname);
  Mycila::Config.configure(KEY_ADMIN_PASSWORD);
  // logger
  Mycila::Config.configure(KEY_DEBUG_ENABLE, BEELANCE_BOOL(BEELANCE_DEBUG_ENABLE));
  // ap
  Mycila::Config.configure(KEY_AP_MODE_ENABLE, BEELANCE_BOOL(BEELANCE_AP_MODE_ENABLE));
  /// wifi
  Mycila::Config.configure(KEY_WIFI_SSID);
  Mycila::Config.configure(KEY_WIFI_PASSWORD);
  Mycila::Config.configure(KEY_WIFI_CONNECTION_TIMEOUT, String(ESPCONNECT_CONNECTION_TIMEOUT));
  // portal
  Mycila::Config.configure(KEY_CAPTURE_PORTAL_TIMEOUT, String(ESPCONNECT_PORTAL_TIMEOUT));
  // system temperature
  Mycila::Config.configure(KEY_TEMPERATURE_ENABLE, BEELANCE_BOOL(BEELANCE_TEMPERATURE_ENABLE));
  Mycila::Config.configure(KEY_TEMPERATURE_PIN, String(BEELANCE_TEMPERATURE_PIN));
  // beelance
  Mycila::Config.configure(KEY_SLEEP_ENABLE, BEELANCE_BOOL(BEELANCE_SLEEP_ENABLE));
  Mycila::Config.configure(KEY_SEND_INTERVAL, String(BEELANCE_SEND_INTERVAL));
}
