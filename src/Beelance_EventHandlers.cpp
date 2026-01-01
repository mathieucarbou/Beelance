// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) Mathieu Carbou
 */

#include <Beelance.h>
#include <BeelanceWebsite.h>

#include <string>

#define TAG "EVENTS"

void refresh() {
  websiteTask.requestEarlyRun();
}

void Beelance::BeelanceClass::_initEventHandlers() {
  espConnect.listen([this](Mycila::ESPConnect::State previous, Mycila::ESPConnect::State state) {
    logger.debug(TAG, "NetworkState: %s => %s", espConnect.getStateName(previous), espConnect.getStateName(state));
    switch (state) {
      case Mycila::ESPConnect::State::NETWORK_CONNECTED:
        logger.info(TAG, "Connected with IP address %s", espConnect.getIPAddress().toString().c_str());
        startNetworkServicesTask.resume();
        startModemTask.resume();
        break;
      case Mycila::ESPConnect::State::AP_STARTED:
        logger.info(TAG, "Access Point %s started with IP address %s", espConnect.getWiFiSSID().c_str(), espConnect.getIPAddress().toString().c_str());
        startNetworkServicesTask.resume();
        startModemTask.resume();
        break;
      case Mycila::ESPConnect::State::NETWORK_DISCONNECTED:
        logger.warn(TAG, "Disconnected!");
        stopNetworkServicesTask.resume();
        break;
      case Mycila::ESPConnect::State::NETWORK_DISABLED:
        logger.info(TAG, "Disable Network...");
        break;
      case Mycila::ESPConnect::State::NETWORK_CONNECTING:
        logger.info(TAG, "Connecting to network...");
        break;
      case Mycila::ESPConnect::State::AP_STARTING:
        logger.info(TAG, "Starting Access Point %s...", espConnect.getAccessPointSSID().c_str());
        break;
      case Mycila::ESPConnect::State::PORTAL_STARTING:
        logger.info(TAG, "Starting Captive Portal %s for %u seconds...", espConnect.getAccessPointSSID().c_str(), espConnect.getCaptivePortalTimeout());
        break;
      case Mycila::ESPConnect::State::PORTAL_STARTED:
        logger.info(TAG, "Captive Portal started at %s with IP address %s", espConnect.getWiFiSSID().c_str(), espConnect.getIPAddress().toString().c_str());
        break;
      case Mycila::ESPConnect::State::PORTAL_COMPLETE: {
        if (espConnect.getConfig().apMode) {
          logger.info(TAG, "Captive Portal: Access Point configured");
          config.setBool(KEY_AP_MODE_ENABLE, true);
        } else {
          logger.info(TAG, "Captive Portal: WiFi configured");
          config.setBool(KEY_AP_MODE_ENABLE, false);
          config.setString(KEY_WIFI_SSID, espConnect.getConfig().wifiSSID);
          config.setString(KEY_WIFI_PASSWORD, espConnect.getConfig().wifiPassword);
        }
        break;
      }
      case Mycila::ESPConnect::State::PORTAL_TIMEOUT:
        logger.warn(TAG, "Captive Portal: timed out.");
        break;
      case Mycila::ESPConnect::State::NETWORK_TIMEOUT:
        logger.warn(TAG, "Unable to connect!");
        break;
      case Mycila::ESPConnect::State::NETWORK_RECONNECTING:
        logger.info(TAG, "Trying to reconnect...");
        break;
      default:
        break;
    }
  });

  config.listen([this](const char* k, const Mycila::config::Value& newValue) {
    logger.info(TAG, "'%s' => '%s'", k, newValue.as<const char*>());
    const std::string key = k;

    if (key == KEY_AP_MODE_ENABLE && (espConnect.getState() == Mycila::ESPConnect::State::AP_STARTED || espConnect.getState() == Mycila::ESPConnect::State::NETWORK_CONNECTING || espConnect.getState() == Mycila::ESPConnect::State::NETWORK_CONNECTED || espConnect.getState() == Mycila::ESPConnect::State::NETWORK_TIMEOUT || espConnect.getState() == Mycila::ESPConnect::State::NETWORK_DISCONNECTED || espConnect.getState() == Mycila::ESPConnect::State::NETWORK_RECONNECTING)) {
      restartTask.resume();

    } else if (key == KEY_DEBUG_ENABLE) {
      configureDebugTask.resume();
      esp_log_level_set("*", static_cast<esp_log_level_t>(logger.getLevel()));

    } else if (key == KEY_MODEM_APN) {
      Mycila::Modem.setAPN(config.getString(KEY_MODEM_APN));

    } else if (key == KEY_MODEM_PIN) {
      Mycila::Modem.setPIN(config.getString(KEY_MODEM_PIN));

    } else if (key == KEY_MODEM_BANDS_LTE_M) {
      Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_LTE_M, config.getString(KEY_MODEM_BANDS_LTE_M));

    } else if (key == KEY_MODEM_BANDS_NB_IOT) {
      Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_NB_IOT, config.getString(KEY_MODEM_BANDS_NB_IOT));

    } else if (key == KEY_MODEM_MODE) {
      std::string tech = config.getString(KEY_MODEM_MODE);
      if (tech == "LTE-M") {
        Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_LTE_M);
      } else if (tech == "NB-IoT") {
        Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_NB_IOT);
      } else {
        Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_AUTO);
      }

    } else if (key == KEY_MODEM_GPS_SYNC_TIMEOUT) {
      Mycila::Modem.setGpsSyncTimeout(config.getLong(KEY_MODEM_GPS_SYNC_TIMEOUT));

    } else if (key == KEY_TIMEZONE_INFO) {
      logger.info(TAG, "Setting timezone to %s", config.getString(KEY_TIMEZONE_INFO));
      Mycila::Modem.setTimeZoneInfo(config.getString(KEY_TIMEZONE_INFO));
      setenv("TZ", config.getString(KEY_TIMEZONE_INFO), 1);
      tzset();

    } else if (key == KEY_HX711_OFFSET) {
      logger.info(TAG, "Setting HX711 offset to %d", config.getLong(KEY_HX711_OFFSET));
      hx711.setOffset(config.getLong(KEY_HX711_OFFSET));

    } else if (key == KEY_HX711_SCALE) {
      logger.info(TAG, "Setting HX711 scale to %f", config.getFloat(KEY_HX711_SCALE));
      hx711.setScale(config.getFloat(KEY_HX711_SCALE));

    } else if (key == KEY_PMU_CHARGING_CURRENT) {
      logger.info(TAG, "Setting charging current to %d mA", config.getLong(KEY_PMU_CHARGING_CURRENT));
      Mycila::PMU.setChargingCurrent(config.getLong(KEY_PMU_CHARGING_CURRENT));
    }
  });

  config.listen([this]() {
    logger.info(TAG, "Configuration restored.");
    restartTask.resume();
  });

  Mycila::Modem.setCallback([this](Mycila::ModemState state) {
    switch (state) {
      case Mycila::ModemState::MODEM_ERROR: {
        logger.info(TAG, "Modem error. Updating Watchdog to 2 minute...");
        watchdogTask.setEnabled(true);
        watchdogTask.resume(2 * Mycila::TaskDuration::MINUTES);
        break;
      }
      case Mycila::ModemState::MODEM_READY: {
        logger.info(TAG, "Modem is ready. Disabling watchdog...");
        watchdogTask.setEnabled(false);

        if (Beelance::Beelance.isNightModeActive()) {
          const uint32_t delay = Beelance::Beelance.getDelayUntilNextSend();
          if (Beelance::Beelance.mustSleep()) {
            logger.info(TAG, "Modem is ready. Night Mode is active, going to sleep for %u seconds...", delay);
            Beelance::Beelance.sleep(delay); // after sleep, ESP will be restarted
          } else {
            logger.info(TAG, "Modem is ready. Night Mode is active, sleep is disabled, sending next measurements in %u seconds...", delay);
            sendTask.setEnabled(true);
            sendTask.resume(delay * Mycila::TaskDuration::SECONDS);
          }
        } else {
          logger.info(TAG, "Modem is ready. Night Mode is off, sending measurements now....");
          sendTask.setEnabled(true);
          sendTask.requestEarlyRun();
          sendTask.resume();
        }
        break;
      }
      default: {
        logger.info(TAG, "Modem is not ready. Updating Watchdog to 5 minutes...");
        watchdogTask.setEnabled(true);
        watchdogTask.resume(5 * Mycila::TaskDuration::MINUTES);
        break;
      }
    }
  });
}
