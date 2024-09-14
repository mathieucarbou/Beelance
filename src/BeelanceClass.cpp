// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <Beelance.h>

#include <BeelanceWebsite.h>
#include <LittleFS.h>

#include <algorithm>

#define TAG "BEELANCE"

Beelance::BeelanceClass::BeelanceClass() {
  latestHistory.reserve(BEELANCE_MAX_HISTORY_SIZE);
  hourlyHistory.reserve(BEELANCE_MAX_HISTORY_SIZE);
  dailyHistory.reserve(BEELANCE_MAX_HISTORY_SIZE);
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

  const int inRange = Mycila::Time::timeInRange(timeInfo, config.get(KEY_NIGHT_START_TIME).c_str(), config.get(KEY_NIGHT_STOP_TIME).c_str());
  return inRange != -1 && inRange;
}

uint32_t Beelance::BeelanceClass::getDelayUntilNextSend() const {
  const int itvl = config.get(KEY_SEND_INTERVAL).toInt() < BEELANCE_MIN_SEND_DELAY ? BEELANCE_MIN_SEND_DELAY : config.get(KEY_SEND_INTERVAL).toInt();

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
    } while (Mycila::Time::timeInRange(timeInfo, config.get(KEY_NIGHT_START_TIME).c_str(), config.get(KEY_NIGHT_STOP_TIME).c_str()) == 1);

    if (total == itvl) {
      // the next send time is not within the night time range
      return total;
    }
  }

  // total is after the night time range
  // total - itvl is still within the night time range
  const int stopTimeMins = Mycila::Time::toMinutes(config.get(KEY_NIGHT_STOP_TIME).c_str());

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

  Mycila::System::deepSleep(seconds * (uint64_t)1000000ULL);
}

bool Beelance::BeelanceClass::sendMeasurements() {
  if (!Mycila::Modem.isReady()) {
    logger.error(TAG, "Unable to send measurements: modem not ready");
    return false;
  }

  if (config.get(KEY_SEND_URL).isEmpty() && Mycila::Modem.getAPN() != "onomondo") {
    logger.error(TAG, "Unable to send measurements: no URL defined");
    return false;
  }

  JsonDocument doc;
  String payload;
  payload.reserve(512);
  toJson(doc.to<JsonObject>());
  serializeJson(doc, payload);

  _recordMeasurement(doc["ts"].as<time_t>(), doc["temp"].as<float>(), doc["wt"].as<int32_t>());
  _saveHistory();

  if (!config.get(KEY_SEND_URL).isEmpty()) {
    const String url = config.get(KEY_SEND_URL);
    logger.info(TAG, "Sending measurements to %s...", url.c_str());
    switch (Mycila::Modem.httpPOST(url, payload)) {
      case ESP_OK:
        logger.info(TAG, "Measurements sent successfully");
        return true;
      case ESP_ERR_INVALID_ARG:
        logger.error(TAG, "Unable to send measurements: invalid URL %s", url.c_str());
        return false;
      case ESP_ERR_TIMEOUT:
        logger.error(TAG, "Unable to send measurements: timeout connecting to %s", url.c_str());
        return false;
      case ESP_ERR_INVALID_STATE:
        logger.error(TAG, "Unable to send measurements: unable to connect");
        return false;
      case ESP_ERR_INVALID_RESPONSE:
        logger.error(TAG, "Unable to send measurements: invalid response from server");
        return false;
      default:
        logger.error(TAG, "Unable to send measurements: unknown error");
        return false;
    }
  }

  if (Mycila::Modem.getAPN() == "onomondo") {
    logger.info(TAG, "Sending measurements using Onomondo Connector...");
    switch (Mycila::Modem.sendTCP("1.2.3.4", 1234, payload)) {
      case ESP_OK:
        logger.info(TAG, "Measurements sent successfully using Onomondo Connector");
        return true;
      case ESP_ERR_TIMEOUT:
        logger.error(TAG, "Unable to send measurements: timeout connecting to Onomondo Platform");
        return false;
      case ESP_ERR_INVALID_STATE:
        logger.error(TAG, "Unable to send measurements: unable to connect");
        return false;
      case ESP_ERR_INVALID_RESPONSE:
        logger.error(TAG, "Unable to send measurements: invalid response from server");
        return false;
      default:
        logger.error(TAG, "Unable to send measurements: unknown error");
        return false;
    }
  }

  assert(false); // Should never happen
  return false;
}

void Beelance::BeelanceClass::toJson(const JsonObject& root) const {
  // time
  root["ts"] = Mycila::Time::getUnixTime();
  // beehive
  root["bh"] = config.get(KEY_BEEHIVE_NAME);
  // sensors
  root["temp"] = _round2(temperatureSensor.getTemperature().value_or(0));
  root["wt"] = hx711.isValid() ? static_cast<int32_t>(hx711.getWeight()) : 0;
  // gps
  root["lat"] = Mycila::Modem.getGPSData().latitude;
  root["long"] = Mycila::Modem.getGPSData().longitude;
  root["alt"] = _round2(Mycila::Modem.getGPSData().altitude);
  // sim
  root["sim"] = Mycila::Modem.getICCID();
  root["op"] = Mycila::Modem.getOperator();
  // device
  root["dev"] = Mycila::System::getChipIDStr();
  root["boot"] = Mycila::System::getBootCount();
  root["ver"] = Mycila::AppInfo.version;
  root["up"] = Mycila::System::getUptime();
  // power
  root["pow"] = Mycila::PMU.isBatteryDischarging() ? "bat" : "ext";
  root["bat"] = _round2(Mycila::PMU.getBatteryLevel());
  root["volt"] = _round2(Mycila::PMU.getBatteryVoltage());
}

void Beelance::BeelanceClass::historyToJson(const JsonObject& root) const {
  // latest
  JsonArray latest = root["latest"].to<JsonArray>();
  for (const auto& entry : latestHistory) {
    JsonObject o = latest.add<JsonObject>();
    o["time"] = entry.time;
    o["temp"] = entry.temperature;
    o["wt"] = entry.weight;
  }
  // hourly
  JsonArray hourly = root["hourly"].to<JsonArray>();
  for (const auto& entry : hourlyHistory) {
    JsonObject o = hourly.add<JsonObject>();
    o["time"] = entry.time;
    o["temp"] = entry.temperature;
    o["wt"] = entry.weight;
  }
  // daily
  JsonArray daily = root["daily"].to<JsonArray>();
  for (const auto& entry : dailyHistory) {
    JsonObject o = daily.add<JsonObject>();
    o["time"] = entry.time;
    o["temp"] = entry.temperature;
    o["wt"] = entry.weight;
  }
}

void Beelance::BeelanceClass::clearHistory() {
  latestHistory.clear();
  hourlyHistory.clear();
  dailyHistory.clear();
  if (LittleFS.exists(FILE_HISTORY))
    LittleFS.remove(FILE_HISTORY);
  Beelance::Website.requestChartUpdate();
}

bool Beelance::BeelanceClass::mustSleep() const {
  return !config.getBool(KEY_NO_SLEEP_ENABLE);
  // return !config.getBool(KEY_NO_SLEEP_ENABLE) && Mycila::PMU.isBatteryPowered();
}

void Beelance::BeelanceClass::_recordMeasurement(const time_t timestamp, const float temperature, const int32_t weight) {
  logger.info(TAG, "Record measurement: temperature = %.2f C, weight = %d g", temperature, weight);

  const String dt = Mycila::Time::toLocalStr(timestamp); // 2024-04-12 15:02:17
  const String hhmm = dt.substring(11, 16);              // 15:02
  const String hour = dt.substring(11, 13) + ":00";      // 15:00
  const String day = dt.substring(5, 10);                // 2024-04-12

  bool found = false;
  for (auto& entry : latestHistory) {
    if (entry.time == hhmm) {
      found = true;
      if (temperature > entry.temperature)
        entry.temperature = temperature;
      if (weight > entry.weight)
        entry.weight = weight;
      break;
    }
  }
  if (!found) {
    while (latestHistory.size() >= BEELANCE_MAX_HISTORY_SIZE)
      latestHistory.erase(latestHistory.begin());
    latestHistory.push_back({hhmm, temperature, weight});
  }

  found = false;
  for (auto& entry : hourlyHistory) {
    if (entry.time == hour) {
      found = true;
      if (temperature > entry.temperature)
        entry.temperature = temperature;
      if (weight > entry.weight)
        entry.weight = weight;
      break;
    }
  }
  if (!found) {
    while (hourlyHistory.size() >= BEELANCE_MAX_HISTORY_SIZE)
      hourlyHistory.erase(hourlyHistory.begin());
    hourlyHistory.push_back({hour, temperature, weight});
  }

  found = false;
  for (auto& entry : dailyHistory) {
    if (entry.time == day) {
      found = true;
      if (temperature > entry.temperature)
        entry.temperature = temperature;
      if (weight > entry.weight)
        entry.weight = weight;
      break;
    }
  }
  if (!found) {
    while (dailyHistory.size() >= BEELANCE_MAX_HISTORY_SIZE)
      dailyHistory.erase(dailyHistory.begin());
    dailyHistory.push_back({day, temperature, weight});
  }

  Beelance::Website.requestChartUpdate();
}

void Beelance::BeelanceClass::_loadHistory() {
  std::lock_guard<std::mutex> lck(_mutex);

  logger.info(TAG, "Load history...");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  if (LittleFS.exists(FILE_HISTORY)) {
    File file = LittleFS.open(FILE_HISTORY, "r");

    if (file) {
      deserializeJson(root, file);
      file.close();

      // latestHistory
      latestHistory.clear();
      JsonArray latest = root["latest"].as<JsonArray>();
      for (int len = latest.size(), i = max(0, len - BEELANCE_MAX_HISTORY_SIZE); i < len; i++)
        latestHistory.push_back({latest[i]["time"].as<String>(), latest[i]["temp"].as<float>(), latest[i]["wt"].as<int32_t>()});

      // hourlyHistory
      hourlyHistory.clear();
      JsonArray hourly = root["hourly"].as<JsonArray>();
      for (int len = hourly.size(), i = max(0, len - BEELANCE_MAX_HISTORY_SIZE); i < len; i++)
        hourlyHistory.push_back({hourly[i]["time"].as<String>(), hourly[i]["temp"].as<float>(), hourly[i]["wt"].as<int32_t>()});

      // dailyHistory
      dailyHistory.clear();
      JsonArray daily = root["daily"].as<JsonArray>();
      for (int len = daily.size(), i = max(0, len - BEELANCE_MAX_HISTORY_SIZE); i < len; i++)
        dailyHistory.push_back({daily[i]["time"].as<String>(), daily[i]["temp"].as<float>(), daily[i]["wt"].as<int32_t>()});

    } else {
      logger.error(TAG, "Unable to open file: " FILE_HISTORY);
    }

  } else {
    logger.warn(TAG, "File not found: " FILE_HISTORY);
  }

  Beelance::Website.requestChartUpdate();
}

void Beelance::BeelanceClass::_saveHistory() {
  std::lock_guard<std::mutex> lck(_mutex);

  logger.info(TAG, "Save history...");

  JsonDocument doc;
  historyToJson(doc.to<JsonObject>());

  File file = LittleFS.open(FILE_HISTORY, "w");
  if (file) {
    serializeJson(doc.as<JsonObject>(), file);
    file.close();
  } else {
    logger.error(TAG, "Unable to save file: " FILE_HISTORY);
  }
}

double Beelance::BeelanceClass::_round2(double value) {
  return static_cast<int32_t>(value * 100 + 0.5) / 100.0;
}

namespace Beelance {
  BeelanceClass Beelance;
} // namespace Beelance
