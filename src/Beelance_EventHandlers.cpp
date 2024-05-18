// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */

#include <Beelance.h>
#include <BeelanceWebsite.h>

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
        startModemTask.resume();
        break;
      case ESPConnectState::AP_STARTED:
        Mycila::Logger.info(TAG, "Access Point %s started with IP address %s", ESPConnect.getWiFiSSID().c_str(), ESPConnect.getIPAddress().toString().c_str());
        startNetworkServicesTask.resume();
        startModemTask.resume();
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
          config.setBool(KEY_AP_MODE_ENABLE, true);
        } else {
          Mycila::Logger.info(TAG, "Captive Portal: WiFi configured");
          config.setBool(KEY_AP_MODE_ENABLE, false);
          config.set(KEY_WIFI_SSID, ESPConnect.getConfiguredWiFiSSID());
          config.set(KEY_WIFI_PASSWORD, ESPConnect.getConfiguredWiFiPassword());
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

  config.listen([this](const String& key, const String& oldValue, const String& newValue) {
    Mycila::Logger.info(TAG, "Set %s: '%s' => '%s'", key.c_str(), oldValue.c_str(), newValue.c_str());
    if (key == KEY_AP_MODE_ENABLE && (ESPConnect.getState() == ESPConnectState::AP_STARTED || ESPConnect.getState() == ESPConnectState::NETWORK_CONNECTING || ESPConnect.getState() == ESPConnectState::NETWORK_CONNECTED || ESPConnect.getState() == ESPConnectState::NETWORK_TIMEOUT || ESPConnect.getState() == ESPConnectState::NETWORK_DISCONNECTED || ESPConnect.getState() == ESPConnectState::NETWORK_RECONNECTING)) {
      restartTask.resume();

    } else if (key == KEY_DEBUG_ENABLE) {
      configureDebugTask.resume();
      esp_log_level_set("*", static_cast<esp_log_level_t>(Mycila::Logger.getLevel()));

    } else if (key == KEY_MODEM_APN) {
      Mycila::Modem.setAPN(config.get(KEY_MODEM_APN));

    } else if (key == KEY_MODEM_PIN) {
      Mycila::Modem.setPIN(config.get(KEY_MODEM_PIN));

    } else if (key == KEY_MODEM_BANDS_LTE_M) {
      Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_LTE_M, config.get(KEY_MODEM_BANDS_LTE_M));

    } else if (key == KEY_MODEM_BANDS_NB_IOT) {
      Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_NB_IOT, config.get(KEY_MODEM_BANDS_NB_IOT));

    } else if (key == KEY_MODEM_MODE) {
      String tech = config.get(KEY_MODEM_MODE);
      if (tech == "LTE-M") {
        Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_LTE_M);
      } else if (tech == "NB-IoT") {
        Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_NB_IOT);
      } else {
        Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_AUTO);
      }

    } else if (key == KEY_MODEM_GPS_SYNC_TIMEOUT) {
      Mycila::Modem.setGpsSyncTimeout(config.get(KEY_MODEM_GPS_SYNC_TIMEOUT).toInt());

    } else if (key == KEY_TIMEZONE_INFO) {
      Mycila::Logger.info(TAG, "Setting timezone to %s", config.get(KEY_TIMEZONE_INFO).c_str());
      Mycila::Modem.setTimeZoneInfo(config.get(KEY_TIMEZONE_INFO));
      setenv("TZ", config.get(KEY_TIMEZONE_INFO).c_str(), 1);
      tzset();

    } else if (key == KEY_HX711_OFFSET) {
      Mycila::Logger.info(TAG, "Setting HX711 offset to %d", config.get(KEY_HX711_OFFSET).toInt());
      hx711.setOffset(config.get(KEY_HX711_OFFSET).toInt());

    } else if (key == KEY_HX711_SCALE) {
      Mycila::Logger.info(TAG, "Setting HX711 scale to %f", config.get(KEY_HX711_SCALE).toFloat());
      hx711.setScale(config.get(KEY_HX711_SCALE).toFloat());

    } else if (key == KEY_PMU_CHARGING_CURRENT) {
      Mycila::Logger.info(TAG, "Setting charging current to %d mA", config.get(KEY_PMU_CHARGING_CURRENT).toInt());
      Mycila::PMU.setChargingCurrent(config.get(KEY_PMU_CHARGING_CURRENT).toInt());
    }
  });

  config.listen([this]() {
    Mycila::Logger.info(TAG, "Configuration restored.");
    restartTask.resume();
  });

  Mycila::Modem.setCallback([this](Mycila::ModemState state) {
    switch (state) {
      case Mycila::ModemState::MODEM_ERROR: {
        Mycila::Logger.info(TAG, "Modem error. Updating Watchdog to 2 minute...");
        watchdogTask.setEnabled(true);
        watchdogTask.resume(2 * Mycila::TaskDuration::MINUTES);
        break;
      }
      case Mycila::ModemState::MODEM_READY: {
        Mycila::Logger.info(TAG, "Modem is ready. Disabling watchdog...");
        watchdogTask.setEnabled(false);

        if (Beelance::Beelance.isNightModeActive()) {
          const uint32_t delay = Beelance::Beelance.getDelayUntilNextSend();
          if (Beelance::Beelance.mustSleep()) {
            Mycila::Logger.info(TAG, "Modem is ready. Night Mode is active, going to sleep for %u seconds...", delay);
            Beelance::Beelance.sleep(delay); // after sleep, ESP will be restarted
          } else {
            Mycila::Logger.info(TAG, "Modem is ready. Night Mode is active, sleep is disabled, sending next measurements in %u seconds...", delay);
            sendTask.setEnabled(true);
            sendTask.resume(delay * Mycila::TaskDuration::SECONDS);
          }
        } else {
          Mycila::Logger.info(TAG, "Modem is ready. Night Mode is off, sending measurements now....");
          sendTask.setEnabled(true);
          sendTask.requestEarlyRun();
          sendTask.resume();
        }
        break;
      }
      default: {
        Mycila::Logger.info(TAG, "Modem is not ready. Updating Watchdog to 5 minutes...");
        watchdogTask.setEnabled(true);
        watchdogTask.resume(5 * Mycila::TaskDuration::MINUTES);
        break;
      }
    }
  });
}
