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

bool Beelance::BeelanceClass::isNightModeActive() const {
  if (!Mycila::Modem.isTimeSynced()) {
    // modem time not synced
    return false;
  }

  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 5)) {
    // ESP32 time not yet available
    return false;
  }

  const int inRange = Mycila::Time::timeInRange(&timeInfo, Mycila::Config.get(KEY_NIGHT_START_TIME), Mycila::Config.get(KEY_NIGHT_STOP_TIME));
  return inRange != -1 && inRange;
}

uint32_t Beelance::BeelanceClass::getDelayUntilNextSend() const {
  const int itvl = Mycila::Config.get(KEY_SEND_INTERVAL).toInt() < BEELANCE_MIN_SEND_DELAY ? BEELANCE_MIN_SEND_DELAY : Mycila::Config.get(KEY_SEND_INTERVAL).toInt();

  if (!Mycila::Modem.isTimeSynced()) {
    // modem time not synced
    return itvl;
  }

  const time_t now = Mycila::Time::getUnixTime();
  if (!now) {
    // ESP32 time not yet available
    return itvl;
  }

  int total = 0;

  {
    struct tm timeInfo;
    do {
      total += itvl;
      time_t unixTime = now + total;
      localtime_r(&unixTime, &timeInfo);
    } while (Mycila::Time::timeInRange(&timeInfo, Mycila::Config.get(KEY_NIGHT_START_TIME), Mycila::Config.get(KEY_NIGHT_STOP_TIME)) == 1);

    if (total == itvl) {
      // the next send time is not within the night time range
      return total;
    }
  }

  // total is after the night time range
  // total - itvl is still within the night time range
  const int stopTimeMins = Mycila::Time::toMinutes(Mycila::Config.get(KEY_NIGHT_STOP_TIME));

  const time_t unixTime = now + total - itvl;
  struct tm timeInfo;
  localtime_r(&unixTime, &timeInfo);
  timeInfo.tm_min = stopTimeMins % 60;
  timeInfo.tm_hour = (stopTimeMins - timeInfo.tm_min) / 60;
  const time_t unixTimeAtStopTime = mktime(&timeInfo);

  return unixTimeAtStopTime < unixTime ? unixTimeAtStopTime + 86400 - now : unixTimeAtStopTime - now;
}

void Beelance::BeelanceClass::sleep(uint32_t seconds) {
  digitalWrite(temperatureSensor.getPin(), LOW);
  temperatureSensor.end();

  digitalWrite(hx711.getDataPin(), LOW);
  digitalWrite(hx711.getClockPin(), LOW);
  hx711.end();

  modemTaskManager.pause();
  loopTaskManager.pause();

  Mycila::Modem.powerOff();
  Mycila::PMU.powerOff();

  Wire1.end();
  Wire.end();

  Mycila::System.deepSleep(seconds * (uint64_t)1000000ULL);
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
        Mycila::Logger.info(TAG, "Measurements sent successfully");
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
        Mycila::Logger.info(TAG, "Measurements sent successfully using Onomondo Connector");
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
  // time
  root["ts"] = Mycila::Time::getUnixTime();
  // beehive
  root["bh"] = Mycila::Config.get(KEY_BEEHIVE_NAME);
  // sensors
  root["temp"] = temperatureSensor.isValid() ? _round2(temperatureSensor.getTemperature()) : 0;
  root["wt"] = hx711.isValid() ? static_cast<int>(hx711.getWeight()) : 0;
  // gps
  root["lat"] = Mycila::Modem.getGPSData().latitude;
  root["long"] = Mycila::Modem.getGPSData().longitude;
  root["alt"] = _round2(Mycila::Modem.getGPSData().altitude);
  // sim
  root["sim"] = Mycila::Modem.getICCID();
  root["op"] = Mycila::Modem.getOperator();
  // device
  root["dev"] = Mycila::AppInfo.id;
  root["boot"] = Mycila::System.getBootCount();
  root["ver"] = Mycila::AppInfo.version;
  root["up"] = Mycila::System.getUptime();
  // power
  root["pow"] = Mycila::PMU.isBatteryPowered() ? "bat" : "ext";
  root["bat"] = _round2(Mycila::PMU.getBatteryLevel());
  root["volt"] = _round2(Mycila::PMU.getVoltage());
}

double Beelance::BeelanceClass::_round2(double value) {
  return static_cast<int>(value * 100 + 0.5) / 100.0;
}

namespace Beelance {
  BeelanceClass Beelance;
} // namespace Beelance
