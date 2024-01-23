// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */

#include <Beelance.h>

#define TAG "EVENTS"

void refresh() {
  websiteTask.requestEarlyRun();
}

void Beelance::BeelanceClass::_initEventHandlers() {
  ESPConnect.listen([this](ESPConnectState previous, ESPConnectState state) {
    Mycila::Logger.debug(TAG, "NetworkState: %s => %s", ESPConnect.getStateName(previous), ESPConnect.getStateName(state));
    switch (state) {
      case ESPConnectState::NETWORK_CONNECTED:
        Mycila::Logger.info(TAG, "Connected with IP address %s", ESPConnect.getIPAddress().toString().c_str());
        startNetworkServicesTask.resume();
        break;
      case ESPConnectState::AP_STARTED:
        Mycila::Logger.info(TAG, "Access Point %s started with IP address %s", ESPConnect.getWiFiSSID().c_str(), ESPConnect.getIPAddress().toString().c_str());
        startNetworkServicesTask.resume();
        break;
      case ESPConnectState::NETWORK_DISCONNECTED:
        Mycila::Logger.warn(TAG, "Disconnected!");
        stopNetworkServicesTask.resume();
        break;
      case ESPConnectState::NETWORK_DISABLED:
        Mycila::Logger.info(TAG, "Disable Network...");
        break;
      case ESPConnectState::NETWORK_CONNECTING:
        Mycila::Logger.info(TAG, "Connecting to network...");
        break;
      case ESPConnectState::AP_STARTING:
        Mycila::Logger.info(TAG, "Starting Access Point %s...", ESPConnect.getAccessPointSSID().c_str());
        break;
      case ESPConnectState::PORTAL_STARTING:
        Mycila::Logger.info(TAG, "Starting Captive Portal %s for %u seconds...", ESPConnect.getAccessPointSSID().c_str(), ESPConnect.getCaptivePortalTimeout());
        break;
      case ESPConnectState::PORTAL_STARTED:
        Mycila::Logger.info(TAG, "Captive Portal started at %s with IP address %s", ESPConnect.getWiFiSSID().c_str(), ESPConnect.getIPAddress().toString().c_str());
        break;
      case ESPConnectState::PORTAL_COMPLETE: {
        bool ap = ESPConnect.hasConfiguredAPMode();
        if (ap) {
          Mycila::Logger.info(TAG, "Captive Portal: Access Point configured");
          Mycila::Config.setBool(KEY_AP_MODE_ENABLE, true);
        } else {
          Mycila::Logger.info(TAG, "Captive Portal: WiFi configured");
          Mycila::Config.setBool(KEY_AP_MODE_ENABLE, false);
          Mycila::Config.set(KEY_WIFI_SSID, ESPConnect.getConfiguredWiFiSSID());
          Mycila::Config.set(KEY_WIFI_PASSWORD, ESPConnect.getConfiguredWiFiPassword());
        }
        break;
      }
      case ESPConnectState::PORTAL_TIMEOUT:
        Mycila::Logger.warn(TAG, "Captive Portal: timed out.");
        break;
      case ESPConnectState::NETWORK_TIMEOUT:
        Mycila::Logger.warn(TAG, "Unable to connect!");
        break;
      case ESPConnectState::NETWORK_RECONNECTING:
        Mycila::Logger.info(TAG, "Trying to reconnect...");
        break;
      default:
        break;
    }
  });

  Mycila::Config.listen([this](const String& key, const String& oldValue, const String& newValue) {
    Mycila::Logger.info(TAG, "Set %s: '%s' => '%s'", key.c_str(), oldValue.c_str(), newValue.c_str());
    if (key == KEY_AP_MODE_ENABLE && (ESPConnect.getState() == ESPConnectState::AP_STARTED || ESPConnect.getState() == ESPConnectState::NETWORK_CONNECTING || ESPConnect.getState() == ESPConnectState::NETWORK_CONNECTED || ESPConnect.getState() == ESPConnectState::NETWORK_TIMEOUT || ESPConnect.getState() == ESPConnectState::NETWORK_DISCONNECTED || ESPConnect.getState() == ESPConnectState::NETWORK_RECONNECTING))
      configureNetworkTask.resume();
    else if (key == KEY_DEBUG_ENABLE) {
      configureDebugTask.resume();
      esp_log_level_set("*", static_cast<esp_log_level_t>(Mycila::Logger.getLevel()));
    } else if (key == KEY_TEMPERATURE_ENABLE)
      configureSystemTemperatureSensorTask.resume();
  });

  Mycila::Config.listen([this]() {
    Mycila::Logger.info(TAG, "Configuration restored.");
    restartTask.resume();
  });

  systemTemperatureSensor.listen([this](float temperature) {
    Mycila::Logger.info(TAG, "Temperature changed to %.02f °C", temperature);
    refresh();
  });
}
