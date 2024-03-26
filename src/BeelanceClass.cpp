// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <BeelanceWebsite.h>

#define TAG "BEELANCE"

void Beelance::BeelanceClass::_initHTTPd() {
  Mycila::HTTPd.init(&webServer, BEELANCE_ADMIN_USERNAME, Mycila::Config.get(KEY_ADMIN_PASSWORD));
}

void Beelance::BeelanceClass::_initWebsite() {
  Beelance::Website.init();
}

void Beelance::BeelanceClass::updateWebsite() {
  Beelance::Website.update();
}

bool Beelance::BeelanceClass::sendMeasurements() {
  if (!Mycila::Modem.isReady()) {
    Mycila::Logger.error(TAG, "Unable to send measurements: modem not ready");
    return false;
  }

  if (Mycila::Config.get(KEY_SEND_URL).isEmpty() && Mycila::Modem.getAPN() != "onomondo") {
    Mycila::Logger.error(TAG, "Unable to send measurements: no URL defined");
    return false;
  }

  JsonDocument doc;
  String payload;
  payload.reserve(512);
  toJson(doc.to<JsonObject>());
  serializeJson(doc, payload);

  if (!Mycila::Config.get(KEY_SEND_URL).isEmpty()) {
    const String url = Mycila::Config.get(KEY_SEND_URL);
    Mycila::Logger.info(TAG, "Sending measurements to %s...", url.c_str());
    switch (Mycila::Modem.httpPOST(url, payload)) {
      case ESP_OK:
        return true;
      case ESP_ERR_INVALID_ARG:
        Mycila::Logger.error(TAG, "Unable to send measurements: invalid URL %s", url.c_str());
        return false;
      case ESP_ERR_TIMEOUT:
        Mycila::Logger.error(TAG, "Unable to send measurements: timeout connecting to %s", url.c_str());
        return false;
      case ESP_ERR_INVALID_STATE:
        Mycila::Logger.error(TAG, "Unable to send measurements: unable to connect");
        return false;
      case ESP_ERR_INVALID_RESPONSE:
        Mycila::Logger.error(TAG, "Unable to send measurements: invalid response from server");
        return false;
      default:
        Mycila::Logger.error(TAG, "Unable to send measurements: unknown error");
        return false;
    }
  }

  if (Mycila::Modem.getAPN() == "onomondo") {
    Mycila::Logger.info(TAG, "Sending measurements using Onomondo Connector...");
    switch (Mycila::Modem.sendTCP("1.2.3.4", 1234, payload)) {
      case ESP_OK:
        return true;
      case ESP_ERR_TIMEOUT:
        Mycila::Logger.error(TAG, "Unable to send measurements: timeout connecting to Onomondo Platform");
        return false;
      case ESP_ERR_INVALID_STATE:
        Mycila::Logger.error(TAG, "Unable to send measurements: unable to connect");
        return false;
      case ESP_ERR_INVALID_RESPONSE:
        Mycila::Logger.error(TAG, "Unable to send measurements: invalid response from server");
        return false;
      default:
        Mycila::Logger.error(TAG, "Unable to send measurements: unknown error");
        return false;
    }
  }

  assert(false); // Should never happen
}

void Beelance::BeelanceClass::toJson(const JsonObject& root) {
  float voltage = Mycila::PMU.getBatteryVoltage();
  root["altitude"] = Mycila::Modem.getGPSData().altitude;
  root["battery_level"] = Mycila::PMU.getBatteryLevel(voltage);
  root["battery_voltage"] = voltage;
  root["beehive"] = Mycila::Config.get(KEY_BEEHIVE_NAME);
  root["boot"] = Mycila::System.getBootCount();
  root["build"] = Mycila::AppInfo.buildHash;
  root["device"] = Mycila::AppInfo.id;
  root["firmware"] = Mycila::AppInfo.firmware;
  root["iccid"] = Mycila::Modem.getICCID();
  root["imei"] = Mycila::Modem.getIMEI();
  root["latitude"] = Mycila::Modem.getGPSData().latitude;
  root["longitude"] = Mycila::Modem.getGPSData().longitude;
  root["modem"] = Mycila::Modem.getModel();
  root["operator"] = Mycila::Modem.getOperator();
  root["temperature"] = systemTemperatureSensor.isValid() ? systemTemperatureSensor.getTemperature() : 0;
  root["timestamp"] = Mycila::Time::getISO8601Str();
  root["uptime"] = Mycila::System.getUptime();
  root["version"] = Mycila::AppInfo.version;
  root["weight"] = 0;
}

namespace Beelance {
  BeelanceClass Beelance;
} // namespace Beelance
