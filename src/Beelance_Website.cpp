// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <BeelanceWebsite.h>

#include <string>

#define TAG "WEBSITE"

extern const uint8_t logo_jpeg_gz_start[] asm("_binary__pio_data_logo_jpeg_gz_start");
extern const uint8_t logo_jpeg_gz_end[] asm("_binary__pio_data_logo_jpeg_gz_end");
extern const uint8_t config_html_gz_start[] asm("_binary__pio_data_config_html_gz_start");
extern const uint8_t config_html_gz_end[] asm("_binary__pio_data_config_html_gz_end");

static dash::StatisticValue<const char*> _firmName(dashboard, "Application: Name");
static dash::StatisticValue<const char*> _firmVersionStat(dashboard, "Application: Version");
static dash::StatisticValue<const char*> _firmManufacturerStat(dashboard, "Application: Manufacturer");

static dash::StatisticValue<const char*> _firmFilenameStat(dashboard, "Firmware: Filename");
static dash::StatisticValue<const char*> _firmHashStat(dashboard, "Firmware: Build Hash");
static dash::StatisticValue<const char*> _firmTimeStat(dashboard, "Firmware: Build Timestamp");

static dash::StatisticValue _deviceIdStat(dashboard, "Device: Id");
static dash::StatisticValue _cpuModelStat(dashboard, "Device: CPU Model");
static dash::StatisticValue<int8_t> _cpuCoresStat(dashboard, "Device: CPU Cores");
static dash::StatisticValue<uint32_t> _bootCountStat(dashboard, "Device: Reboot Count");
static dash::StatisticValue<const char*> _bootReasonStat(dashboard, "Device: Reboot Reason");
static dash::StatisticValue<size_t> _heapMemoryTotalStat(dashboard, "Device: Heap Memory Total (bytes)");
static dash::StatisticValue<size_t> _heapMemoryUsedStat(dashboard, "Device: Heap Memory Used (bytes)");
static dash::StatisticValue<float, 2> _heapMemoryUsageStat(dashboard, "Device: Heap Memory Usage (%)");

static dash::StatisticValue<int32_t> _hx711WeightStat(dashboard, "HX711: Weight (g)");
static dash::StatisticValue<int32_t> _hx711TareStat(dashboard, "HX711: Tare (g)");
static dash::StatisticValue<int32_t> _hx711OffsetStat(dashboard, "HX711: Offset");
static dash::StatisticValue<float, 6> _hx711ScaleStat(dashboard, "HX711: Scale");

static dash::StatisticValue _modemModelStat(dashboard, "Modem: Model");
static dash::StatisticValue _modemICCIDStat(dashboard, "Modem: ICCID");
static dash::StatisticValue _modemIMEIStat(dashboard, "Modem: IMEI");
static dash::StatisticValue _modemIMSIStat(dashboard, "Modem: IMSI");
static dash::StatisticValue _modemModePrefStat(dashboard, "Modem: Preferred Mode");
static dash::StatisticValue _modemLTEMBandsStat(dashboard, "Modem: LTE-M Bands");
static dash::StatisticValue _modemNBIoTBandsStat(dashboard, "Modem: NB-IoT Bands");
static dash::StatisticValue _modemIpStat(dashboard, "Modem: Local IP Address");

static dash::StatisticValue _hostnameStat(dashboard, "Network: Hostname");
static dash::StatisticValue _apIPStat(dashboard, "Network: Access Point IP Address");
static dash::StatisticValue _apMACStat(dashboard, "Network: Access Point MAC Address");
static dash::StatisticValue _wifiIPStat(dashboard, "Network: WiFi IP Address");
static dash::StatisticValue _wifiMACStat(dashboard, "Network: WiFi MAC Address");
static dash::StatisticValue _wifiSSIDStat(dashboard, "Network: WiFi SSID");
static dash::StatisticValue<int8_t> _wifiRSSIStat(dashboard, "Network: WiFi RSSI (dBm)");
static dash::StatisticValue<int8_t> _wifiSignalStat(dashboard, "Network: WiFi Signal (%)");

static dash::StatisticValue<int8_t> _pmuLowBatShutThreshold(dashboard, "PMU: Low battery shutdown threshold (%)");

// overview

static dash::GenericCard<const char*> _bhName(dashboard, "Beehive");
static dash::GenericCard _power(dashboard, "Power", "%");
static dash::GenericCard<float, 2> _volt(dashboard, "Voltage", "V");
static dash::GenericCard _uptime(dashboard, "Uptime", "s");

static dash::GenericCard<int32_t> _nextSend(dashboard, "Next Update", "min");
static dash::TemperatureCard _temperature(dashboard, "Temperature", "°C");
static dash::SliderCard<int32_t> _weight(dashboard, "Weight", 0, 200000, 100, "g");
static dash::ToggleButtonCard _tare(dashboard, "Tare");

static dash::FeedbackCard _modemAPN(dashboard, "APN");
static dash::FeedbackCard _modemState(dashboard, "Modem");
static dash::FeedbackCard _modemOperator(dashboard, "Operator");
static dash::ProgressCard<uint8_t> _modemSignal(dashboard, "Signal Quality", 0, 100, "%");

static dash::FeedbackCard _time(dashboard, "Time");
static dash::FeedbackCard _latitude(dashboard, "Latitude");
static dash::FeedbackCard _longitude(dashboard, "Longitude");
static dash::FeedbackCard _altitude(dashboard, "Altitude");

// actions

static dash::ToggleButtonCard _sendNow(dashboard, "Send now and sleep!");
static dash::ToggleButtonCard _noSleepMode(dashboard, "Prevent Sleep");
static dash::ToggleButtonCard _resetHistory(dashboard, "Reset Graph History");
static dash::ToggleButtonCard _restart(dashboard, "Restart");
static dash::ToggleButtonCard _safeBoot(dashboard, "Update Firmware");

// graphs

static dash::BarChart<std::string, float> _chartLatestWeight(dashboard, "Weight (g) - Latest");
static dash::BarChart<std::string, float> _chartHourlyWeight(dashboard, "Weight (g) - Hourly Max");
static dash::BarChart<std::string, float> _chartDailyWeight(dashboard, "Weight (g) - Daily Max");

static dash::BarChart<std::string, float> _chartLatestTemp(dashboard, "Temperature (C) - Latest");
static dash::BarChart<std::string, float> _chartHourlyTemp(dashboard, "Temperature (C) - Hourly Max");
static dash::BarChart<std::string, float> _chartDailyTemp(dashboard, "Temperature (C) - Daily Max");

void Beelance::WebsiteClass::init() {
  authMiddleware.setAuthType(AsyncAuthType::AUTH_DIGEST);
  authMiddleware.setRealm("YaSolR");
  authMiddleware.setUsername(BEELANCE_ADMIN_USERNAME);
  authMiddleware.setPassword(config.getString(KEY_ADMIN_PASSWORD));
  authMiddleware.generateHash();

  webServer.addMiddleware(&authMiddleware);

  webServer.on("/logo", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "image/jpeg", logo_jpeg_gz_start, logo_jpeg_gz_end - logo_jpeg_gz_start);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "public, max-age=900");
    request->send(response);
  });

  // ping

  webServer.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "pong");
    request->send(response);
  });

  // dashboard

  webServer.rewrite("/", "/dashboard").setFilter([](AsyncWebServerRequest* request) { return espConnect.getState() != Mycila::ESPConnect::State::PORTAL_STARTED; });

  // config

  webServer
    .on("/config", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/html", config_html_gz_start, config_html_gz_end - config_html_gz_start);
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

  // web console

  webSerial.begin(&webServer, "/console");
  webSerial.onMessage([](const std::string& msg) {
    if (Mycila::string::startsWith(msg, "AT+")) {
      logger.info(TAG, "Enqueue AT Command: %s...", msg.c_str());
      Mycila::Modem.enqueueAT(msg.c_str());
    }
  });
  logger.forwardTo(&webSerial);

  // app stats

  _firmName.setValue(Mycila::AppInfo.name.c_str());
  _firmVersionStat.setValue(Mycila::AppInfo.version.c_str());
  _firmManufacturerStat.setValue(Mycila::AppInfo.manufacturer.c_str());

  _firmFilenameStat.setValue(Mycila::AppInfo.firmware.c_str());
  _firmHashStat.setValue(Mycila::AppInfo.buildHash.c_str());
  _firmTimeStat.setValue(Mycila::AppInfo.buildDate.c_str());

  _deviceIdStat.setValue(Mycila::System::getChipIDStr());
  _cpuModelStat.setValue(ESP.getChipModel());
  _cpuCoresStat.setValue(ESP.getChipCores());
  _bootCountStat.setValue(Mycila::System::getBootCount());
  _bootReasonStat.setValue(Mycila::System::getLastRebootReason());
  _heapMemoryTotalStat.setValue(ESP.getHeapSize());

  _hostnameStat.setValue(Mycila::AppInfo.defaultHostname);

  // home callbacks

  _restart.onChange([this](bool value) {
    restartTask.resume();
    _restart.setValue(!restartTask.isPaused());
    dashboard.refresh(_restart);
  });

  _safeBoot.onChange([this](bool value) {
    espConnect.saveConfiguration();
    Mycila::System::restartFactory("safeboot");
    _safeBoot.setValue(true);
    dashboard.refresh(_safeBoot);
  });

  _sendNow.onChange([this](bool value) {
    if (sendTask.isEnabled()) {
      sendTask.resume();
      sendTask.requestEarlyRun();
    }
    _sendNow.setValue(sendTask.isRunning() || (sendTask.isEnabled() && !sendTask.isPaused() && sendTask.isEarlyRunRequested()));
    dashboard.refresh(_sendNow);
  });

  _tare.onChange([this](bool value) {
    hx711TareTask.resume();
    _tare.setValue(hx711TareTask.isRunning() || (hx711TareTask.isEnabled() && !hx711TareTask.isPaused()));
    dashboard.refresh(_tare);
  });

  _weight.onChange([this](uint32_t expectedWeight) {
    calibrationWeight = expectedWeight;
    hx711ScaleTask.resume(2 * Mycila::TaskDuration::SECONDS);
    _weight.setValue(expectedWeight);
    dashboard.refresh(_weight);
  });

  _resetHistory.onChange([this](bool value) {
    Beelance::Beelance.clearHistory();
    _resetHistory.setValue(false);
    dashboard.refresh(_resetHistory);
  });

  _boolConfig(&_noSleepMode, KEY_PREVENT_SLEEP_ENABLE);

  _update(true);
}

void Beelance::WebsiteClass::disableTemperature() {
  dashboard.remove(_chartLatestTemp);
  dashboard.remove(_chartHourlyTemp);
  dashboard.remove(_chartDailyTemp);
}

void Beelance::WebsiteClass::_boolConfig(dash::ToggleButtonCard* card, const char* key) {
  card->onChange([key, card, this](bool value) {
    card->setValue(value);
    dashboard.refresh(*card);
    config.setBool(key, value);
  });
}

namespace Beelance {
  WebsiteClass Website;
} // namespace Beelance

void Beelance::WebsiteClass::_update(bool skipWebSocketPush) {
  if (dashboard.isAsyncAccessInProgress()) {
    return;
  }

  // stats

  Mycila::System::Memory memory;
  Mycila::System::getMemory(memory);
  _heapMemoryUsageStat.setValue(memory.usage);
  _heapMemoryUsedStat.setValue(memory.used);

  _modemModelStat.setValue(Mycila::Modem.getModel());
  _modemICCIDStat.setValue(Mycila::Modem.getICCID());
  _modemIMEIStat.setValue(Mycila::Modem.getIMEI());
  _modemIMSIStat.setValue(Mycila::Modem.getIMSI());
  switch (Mycila::Modem.getMode()) {
    case ::Mycila::ModemMode::MODEM_MODE_LTE_M:
      _modemModePrefStat.setValue("LTE-M");
      break;
    case ::Mycila::ModemMode::MODEM_MODE_NB_IOT:
      _modemModePrefStat.setValue("NB-IoT");
      break;
    default:
      _modemModePrefStat.setValue("Auto");
      break;
  }
  _modemLTEMBandsStat.setValue(Mycila::Modem.getBands(Mycila::ModemMode::MODEM_MODE_LTE_M));
  _modemNBIoTBandsStat.setValue(Mycila::Modem.getBands(Mycila::ModemMode::MODEM_MODE_NB_IOT));
  _modemIpStat.setValue(Mycila::Modem.getLocalIP());

  _apIPStat.setValue(espConnect.getIPAddress(Mycila::ESPConnect::Mode::AP).toString().c_str());
  _apMACStat.setValue(espConnect.getMACAddress(Mycila::ESPConnect::Mode::AP));
  _wifiIPStat.setValue(espConnect.getIPAddress(Mycila::ESPConnect::Mode::STA).toString().c_str());
  _wifiMACStat.setValue(espConnect.getMACAddress(Mycila::ESPConnect::Mode::STA));
  _wifiSSIDStat.setValue(espConnect.getWiFiSSID());
  _wifiRSSIStat.setValue(espConnect.getWiFiRSSI());
  _wifiSignalStat.setValue(espConnect.getWiFiSignalQuality());
  _hx711WeightStat.setValue(static_cast<int32_t>(hx711.getWeight()));
  _hx711TareStat.setValue(static_cast<int32_t>(hx711.getTare()));
  _hx711OffsetStat.setValue(hx711.getOffset());
  _hx711ScaleStat.setValue(hx711.getScale());

  _pmuLowBatShutThreshold.setValue(Mycila::PMU.readLowBatteryShutdownThreshold());

  // home

  _bhName.setValue(config.getString(KEY_BEEHIVE_NAME));

  // weight
  if (!hx711.isEnabled()) {
    _weight.setValue(0);
    _weight.setUnit("(disabled)");
  } else if (!hx711.isValid()) {
    _weight.setValue(0);
    _weight.setUnit("(pending)");
  } else if (calibrationWeight > 0 && !hx711ScaleTask.isPaused()) {
    _weight.setValue(static_cast<int32_t>(calibrationWeight));
    _weight.setUnit("g");
  } else {
    _weight.setValue(static_cast<int32_t>(hx711.getWeight()));
    _weight.setUnit("g");
  }

  // temperature
  if (!temperatureSensor.isEnabled()) {
    _temperature.setValue(0);
    _temperature.setUnit("(disabled)");
  } else if (!temperatureSensor.isValid()) {
    _temperature.setValue(0);
    _temperature.setUnit("(pending)");
  } else {
    _temperature.setValue(temperatureSensor.getTemperature().value_or(0.0f));
    _temperature.setUnit("°C");
  }

  // next update
  if (sendTask.isEnabled() && !sendTask.isPaused()) {
    _nextSend.setValue(static_cast<int32_t>(ceil(static_cast<double>(sendTask.getRemainingTme()) / Mycila::TaskDuration::MINUTES)));
    _nextSend.setSymbol("min");
  } else {
    _nextSend.setValue(0);
    _nextSend.setSymbol("(paused)");
  }

  // time
  switch (Mycila::Modem.getTimeState()) {
    case Mycila::ModemTimeState::MODEM_TIME_OFF:
      _time.setFeedback("", dash::Status::INFO);
      break;
    case Mycila::ModemTimeState::MODEM_TIME_SYNCING:
      _time.setFeedback("Syncing...", dash::Status::WARNING);
      break;
    case Mycila::ModemTimeState::MODEM_TIME_SYNCED: {
      std::string time = Mycila::Time::getLocalStr();
      if (time.empty())
        _time.setFeedback("Syncing...", dash::Status::WARNING);
      else
        _time.setFeedback(time, dash::Status::SUCCESS);
      break;
    }
    default:
      assert(false);
      break;
  }

  // GPS
  switch (Mycila::Modem.getGPSState()) {
    case Mycila::ModemGPSState::MODEM_GPS_OFF:
      _latitude.setFeedback("", dash::Status::INFO);
      _longitude.setFeedback("", dash::Status::INFO);
      _altitude.setFeedback("", dash::Status::INFO);
      break;
    case Mycila::ModemGPSState::MODEM_GPS_SYNCING:
      _latitude.setFeedback("Syncing...", dash::Status::WARNING);
      _longitude.setFeedback("Syncing...", dash::Status::WARNING);
      _altitude.setFeedback("Syncing...", dash::Status::WARNING);
      break;
    case Mycila::ModemGPSState::MODEM_GPS_SYNCED:
      _latitude.setFeedback(Mycila::string::to_string(Mycila::Modem.getGPSData().latitude, 6), dash::Status::SUCCESS);
      _longitude.setFeedback(Mycila::string::to_string(Mycila::Modem.getGPSData().longitude, 6), dash::Status::SUCCESS);
      _altitude.setFeedback(std::to_string(Mycila::Modem.getGPSData().altitude) + " m", dash::Status::SUCCESS);
      break;
    case Mycila::ModemGPSState::MODEM_GPS_TIMEOUT:
      _latitude.setFeedback("Timeout!", dash::Status::DANGER);
      _longitude.setFeedback("Timeout!", dash::Status::DANGER);
      _altitude.setFeedback("Timeout!", dash::Status::DANGER);
      break;
    default:
      assert(false);
      break;
  }

  // modem
  switch (Mycila::Modem.getState()) {
    case Mycila::ModemState::MODEM_OFF:
      _modemState.setFeedback("", dash::Status::INFO);
      break;
    case Mycila::ModemState::MODEM_STARTING:
      _modemState.setFeedback("Starting...", dash::Status::WARNING);
      break;
    case Mycila::ModemState::MODEM_WAIT_REGISTRATION:
      _modemState.setFeedback("Registering...", dash::Status::WARNING);
      break;
    case Mycila::ModemState::MODEM_SEARCHING:
      _modemState.setFeedback("Searching for operators...", dash::Status::WARNING);
      break;
    case Mycila::ModemState::MODEM_GPS:
      _modemState.setFeedback("Waiting for GPS...", dash::Status::WARNING);
      break;
    case Mycila::ModemState::MODEM_CONNECTING:
      _modemState.setFeedback("Connecting...", dash::Status::WARNING);
      break;
    case Mycila::ModemState::MODEM_READY:
      _modemState.setFeedback("Ready", dash::Status::SUCCESS);
      break;
    case Mycila::ModemState::MODEM_ERROR:
      _modemState.setFeedback(Mycila::Modem.getError(), dash::Status::DANGER);
      break;
    default:
      assert(false);
      break;
  }

  _modemAPN.setFeedback(Mycila::Modem.getAPN(), Mycila::Modem.getAPN().empty() ? dash::Status::DANGER : dash::Status::SUCCESS);

  // operator
  switch (Mycila::Modem.getState()) {
    case Mycila::ModemState::MODEM_OFF:
    case Mycila::ModemState::MODEM_STARTING:
      _modemOperator.setFeedback("", dash::Status::INFO);
      break;
    case Mycila::ModemState::MODEM_WAIT_REGISTRATION:
    case Mycila::ModemState::MODEM_SEARCHING: {
      const Mycila::ModemOperatorSearchResult* candidate = Mycila::Modem.getCandidate();
      _modemOperator.setFeedback(candidate ? candidate->name + " (" + std::to_string(candidate->mode) + ") ?" : std::string(), candidate ? dash::Status::WARNING : dash::Status::INFO);
      break;
    }
    case Mycila::ModemState::MODEM_GPS:
    case Mycila::ModemState::MODEM_CONNECTING:
      if (Mycila::Modem.getOperator().empty())
        _modemOperator.setFeedback("", dash::Status::WARNING);
      else
        _modemOperator.setFeedback(Mycila::Modem.getOperator(), dash::Status::SUCCESS);
      break;
    case Mycila::ModemState::MODEM_READY:
      _modemOperator.setFeedback(Mycila::Modem.getOperator(), dash::Status::SUCCESS);
      break;
    case Mycila::ModemState::MODEM_ERROR:
      _modemOperator.setFeedback("", dash::Status::INFO);
      break;
    default:
      assert(false);
      break;
  }

  _modemSignal.setValue(Mycila::Modem.getSignalQuality());

  if (Mycila::PMU.isBatteryCharging()) {
    float level = Mycila::PMU.getBatteryLevel();
    if (level > 0) {
      _power.setValue(std::string("Bat. charging: ") + Mycila::string::to_string(floor(level), 2));
      _power.setSymbol("%");
    } else {
      _power.setValue("Bat. charging...");
      _power.setSymbol("");
    }
  } else if (Mycila::PMU.isBatteryDischarging()) {
    float level = Mycila::PMU.getBatteryLevel();
    if (level > 0) {
      _power.setValue(std::string("Bat. discharging: ") + Mycila::string::to_string(floor(level), 2));
      _power.setSymbol("%");
    } else {
      _power.setValue("Bat. discharging...");
      _power.setSymbol("");
    }
  } else {
    _power.setValue("External");
    _power.setSymbol("");
  }
  _volt.setValue(Mycila::PMU.getBatteryVoltage());

  _uptime.setValue(Mycila::Time::toDHHMMSS(Mycila::System::getUptime()));
  _restart.setValue(!restartTask.isPaused());

  _sendNow.setValue(sendTask.isRunning() || (sendTask.isEnabled() && !sendTask.isPaused() && sendTask.isEarlyRunRequested()));
  _noSleepMode.setValue(config.getBool(KEY_PREVENT_SLEEP_ENABLE));
  _tare.setValue(hx711TareTask.isRunning() || (hx711TareTask.isEnabled() && !hx711TareTask.isPaused()));

  if (_requestChartUpdate || skipWebSocketPush) {
    _requestChartUpdate = false;

    int idx = 0;
    while (idx < BEELANCE_MAX_HISTORY_SIZE) {
      _chartLatestX[idx] = std::string();
      _chartLatestTempY[idx] = 0;
      _chartLatestWeightY[idx] = 0;

      _chartHourlyX[idx] = std::string();
      _chartHourlyTempY[idx] = 0;
      _chartHourlyWeightY[idx] = 0;

      _chartDailyX[idx] = std::string();
      _chartDailyTempY[idx] = 0;
      _chartDailyWeightY[idx] = 0;

      idx++;
    }

    idx = 0;
    for (const auto& entry : Beelance::Beelance.latestHistory) {
      if (idx < BEELANCE_MAX_HISTORY_SIZE) {
        _chartLatestX[idx] = entry.time;
        _chartLatestTempY[idx] = entry.temperature;
        _chartLatestWeightY[idx] = entry.weight;
        idx++;
      }
    }

    idx = 0;
    for (const auto& entry : Beelance::Beelance.hourlyHistory) {
      if (idx < BEELANCE_MAX_HISTORY_SIZE) {
        _chartHourlyX[idx] = entry.time;
        _chartHourlyTempY[idx] = entry.temperature;
        _chartHourlyWeightY[idx] = entry.weight;
        idx++;
      }
    }

    idx = 0;
    for (const auto& entry : Beelance::Beelance.dailyHistory) {
      if (idx < BEELANCE_MAX_HISTORY_SIZE) {
        _chartDailyX[idx] = entry.time;
        _chartDailyTempY[idx] = entry.temperature;
        _chartDailyWeightY[idx] = entry.weight;
        idx++;
      }
    }

    _chartLatestWeight.setX(_chartLatestX, BEELANCE_MAX_HISTORY_SIZE);
    _chartLatestWeight.setY(_chartLatestWeightY, BEELANCE_MAX_HISTORY_SIZE);

    _chartLatestTemp.setX(_chartLatestX, BEELANCE_MAX_HISTORY_SIZE);
    _chartLatestTemp.setY(_chartLatestTempY, BEELANCE_MAX_HISTORY_SIZE);

    _chartHourlyWeight.setX(_chartHourlyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartHourlyWeight.setY(_chartHourlyWeightY, BEELANCE_MAX_HISTORY_SIZE);

    _chartHourlyTemp.setX(_chartHourlyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartHourlyTemp.setY(_chartHourlyTempY, BEELANCE_MAX_HISTORY_SIZE);

    _chartDailyWeight.setX(_chartDailyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartDailyWeight.setY(_chartDailyWeightY, BEELANCE_MAX_HISTORY_SIZE);

    _chartDailyTemp.setX(_chartDailyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartDailyTemp.setY(_chartDailyTempY, BEELANCE_MAX_HISTORY_SIZE);
  }

  if (!skipWebSocketPush && dashboard.hasClient()) {
    dashboard.sendUpdates();
  }
}
