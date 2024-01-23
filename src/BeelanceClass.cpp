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
  if (!Mycila::Modem.isConnected()) {
    Mycila::Logger.error(TAG, "Unable to send measurements: modem not connected");
    return false;
  }

  if (!Mycila::Config.get(KEY_SEND_URL).isEmpty()) {
    // TODO: http post
    return true;

  } else if (Mycila::Modem.getAPN() == "onomondo") {
    Mycila::Logger.info(TAG, "Sending measurements using Onomondo Connector...");

    TinyGsmClient client(*Mycila::Modem.getModem(), 0);
    client.setTimeout(10000);
    if (client.connect("1.2.3.4", 4321, 10)) {
      JsonDocument doc;
      String buffer;
      buffer.reserve(512);
      toJson(doc.to<JsonObject>());
      serializeJson(doc, buffer);
      client.print(buffer);
      client.flush();
      client.stop();
      return true;

    } else {
      Mycila::Logger.error(TAG, "Unable to send measurements: unable to connect to Onomondo Platform");
      return false;
    }

  } else {
    Mycila::Logger.error(TAG, "Unable to send measurements: no URL defined");
    return false;
  }
}

void Beelance::BeelanceClass::toJson(const JsonObject& root) {
  root["altitude"] = Mycila::Modem.getGPSData().altitude;
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
}

namespace Beelance {
  BeelanceClass Beelance;
} // namespace Beelance
