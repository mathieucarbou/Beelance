// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <BeelanceWebsite.h>
#include <LittleFS.h>

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

  _recordMeasurement(doc["ts"].as<time_t>(), doc["temp"].as<float>(), doc["wt"].as<int32_t>());
  _saveHistory();

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
  return false;
}

void Beelance::BeelanceClass::toJson(const JsonObject& root) {
  // time
  root["ts"] = Mycila::Time::getUnixTime();
  // beehive
  root["bh"] = Mycila::Config.get(KEY_BEEHIVE_NAME);
  // sensors
  root["temp"] = temperatureSensor.isValid() ? _round2(temperatureSensor.getTemperature()) : 0;
  root["wt"] = hx711.isValid() ? static_cast<int32_t>(hx711.getWeight()) : 0;
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

void Beelance::BeelanceClass::historyToJson(const JsonObject& root) {
  JsonObject hourly = root["hourly"].to<JsonObject>();
  for (const auto& [key, value] : hourlyHistory) {
    hourly[key]["temp"] = value.temperature;
    hourly[key]["wt"] = value.weight;
  }
  JsonObject daily = root["daily"].to<JsonObject>();
  for (const auto& [key, value] : dailyHistory) {
    daily[key]["temp"] = value.temperature;
    daily[key]["wt"] = value.weight;
  }
}

void Beelance::BeelanceClass::clearHistory() {
  hourlyHistory.clear();
  dailyHistory.clear();
  if (LittleFS.exists(FILE_HISTORY))
    LittleFS.remove(FILE_HISTORY);
  Beelance::Website.requestChartUpdate();
}

void Beelance::BeelanceClass::_recordMeasurement(const time_t timestamp, const float temperature, const int32_t weight) {
  Mycila::Logger.info(TAG, "Record measurement: temperature = %.2f C, weight = %d g", temperature, weight);

  const String dt = Mycila::Time::toLocalStr(timestamp); // 2024-04-12 15:02:17
  const String day = dt.substring(5, 10);                // 2024-04-12
  // const String day = dt.substring(0, 16);                // 2024-04-12 15:02
  const String hour = dt.substring(11, 13) + ":00";      // 15:00

  bool change = false;

  if (hourlyHistory.find(hour) == hourlyHistory.end()) {
    hourlyHistory[hour] = {temperature, weight};
    change = true;
  } else {
    if (temperature > hourlyHistory[hour].temperature) {
      hourlyHistory[hour].temperature = temperature;
      change = true;
    }
    if (weight > hourlyHistory[hour].weight) {
      hourlyHistory[hour].weight = weight;
      change = true;
    }
  }

  if (dailyHistory.find(day) == dailyHistory.end()) {
    dailyHistory[day] = {temperature, weight};
    change = true;
  } else {
    if (temperature > dailyHistory[day].temperature) {
      dailyHistory[day].temperature = temperature;
      change = true;
    }
    if (weight > dailyHistory[day].weight) {
      dailyHistory[day].weight = weight;
      change = true;
    }
  }

  _prune();

  if (change) {
    Beelance::Website.requestChartUpdate();
  }
}

void Beelance::BeelanceClass::_loadHistory() {
  std::lock_guard<std::mutex> lck(_mutex);

  Mycila::Logger.info(TAG, "Load history...");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  if (LittleFS.exists(FILE_HISTORY)) {
    File file = LittleFS.open(FILE_HISTORY, "r");

    if (file) {
      deserializeJson(root, file);
      file.close();

      // hourlyHistory
      hourlyHistory.clear();
      for (JsonPair kv : root["hourly"].as<JsonObject>()) {
        JsonObject o = kv.value().as<JsonObject>();
        hourlyHistory[String(kv.key().c_str())] = {o["temp"].as<float>(), o["wt"].as<int32_t>()};
      }

      // dailyHistory
      dailyHistory.clear();
      for (JsonPair kv : root["daily"].as<JsonObject>()) {
        JsonObject o = kv.value().as<JsonObject>();
        dailyHistory[String(kv.key().c_str())] = {o["temp"].as<float>(), o["wt"].as<int32_t>()};
      }
    } else {
      Mycila::Logger.error(TAG, "Unable to open file: " FILE_HISTORY);
    }
  } else {
    Mycila::Logger.warn(TAG, "File not found: " FILE_HISTORY);
  }

  _prune();

  Beelance::Website.requestChartUpdate();
}

void Beelance::BeelanceClass::_saveHistory() {
  std::lock_guard<std::mutex> lck(_mutex);

  Mycila::Logger.info(TAG, "Save history...");

  JsonDocument doc;
  historyToJson(doc.to<JsonObject>());

  File file = LittleFS.open(FILE_HISTORY, "w");
  if (file) {
    serializeJson(doc.as<JsonObject>(), file);
    file.close();
  } else {
    Mycila::Logger.error(TAG, "Unable to save file: " FILE_HISTORY);
  }
}

void Beelance::BeelanceClass::_prune() {
  while (hourlyHistory.size() > BEELANCE_MAX_HISTORY_SIZE)
    hourlyHistory.erase(hourlyHistory.begin());
  while (dailyHistory.size() > BEELANCE_MAX_HISTORY_SIZE)
    dailyHistory.erase(dailyHistory.begin());
}

double Beelance::BeelanceClass::_round2(double value) {
  return static_cast<int32_t>(value * 100 + 0.5) / 100.0;
}

namespace Beelance {
  BeelanceClass Beelance;
} // namespace Beelance
