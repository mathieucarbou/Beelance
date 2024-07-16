// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <BeelanceWebsite.h>

#define TAG "WEBSITE"

void Beelance::WebsiteClass::_update(bool skipWebSocketPush) {
  if (dashboard.isAsyncAccessInProgress()) {
    return;
  }

  // stats

  Mycila::SystemMemory memory = Mycila::System.getMemory();
  _heapMemoryUsageStat.set((String(memory.usage) + " %").c_str());
  _heapMemoryUsedStat.set((String(memory.used) + " bytes").c_str());

  _modemModelStat.set(Mycila::Modem.getModel().c_str());
  _modemICCIDStat.set(Mycila::Modem.getICCID().c_str());
  _modemIMEIStat.set(Mycila::Modem.getIMEI().c_str());
  _modemIMSIStat.set(Mycila::Modem.getIMSI().c_str());
  switch (Mycila::Modem.getMode()) {
    case ::Mycila::ModemMode::MODEM_MODE_LTE_M:
      _modemModePrefStat.set("LTE-M");
      break;
    case ::Mycila::ModemMode::MODEM_MODE_NB_IOT:
      _modemModePrefStat.set("NB-IoT");
      break;
    default:
      _modemModePrefStat.set("Auto");
      break;
  }
  _modemLTEMBandsStat.set(Mycila::Modem.getBands(Mycila::ModemMode::MODEM_MODE_LTE_M).c_str());
  _modemNBIoTBandsStat.set(Mycila::Modem.getBands(Mycila::ModemMode::MODEM_MODE_NB_IOT).c_str());
  _modemIpStat.set(Mycila::Modem.getLocalIP().c_str());

  _apIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::AP).toString().c_str());
  _apMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::AP).c_str());
  _wifiIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::STA).toString().c_str());
  _wifiMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::STA).c_str());
  _wifiSSIDStat.set(ESPConnect.getWiFiSSID().c_str());
  _wifiRSSIStat.set((String(ESPConnect.getWiFiRSSI()) + " dBm").c_str());
  _wifiSignalStat.set((String(ESPConnect.getWiFiSignalQuality()) + " %").c_str());
  _hx711WeightStat.set((String(hx711.getWeight(), 0) + " g").c_str());
  _hx711TareStat.set((String(hx711.getTare(), 0) + " g").c_str());
  _hx711OffsetStat.set(String(hx711.getOffset()).c_str());
  _hx711ScaleStat.set(String(hx711.getScale(), 6).c_str());

  _pmuLowBatShutThreshold.set((String(Mycila::PMU.readLowBatteryShutdownThreshold()) + " %").c_str());

  // home

  _bhName.update(config.get(KEY_BEEHIVE_NAME).c_str());

  // weight
  if (!hx711.isEnabled()) {
    _weight.update("Disabled", "");
  } else if (!hx711.isValid()) {
    _weight.update("Pending...", "");
  } else if (calibrationWeight > 0 && !hx711ScaleTask.isPaused()) {
    _weight.update(static_cast<int>(calibrationWeight), "g");
  } else {
    _weight.update(static_cast<int>(hx711.getWeight()), "g");
  }

  // temperature
  if (!temperatureSensor.isEnabled()) {
    _temperature.update("Disabled", "");
  } else if (!temperatureSensor.isValid()) {
    _temperature.update("Pending...", "");
  } else {
    _temperature.update(temperatureSensor.getTemperature().value_or(0), "°C");
  }

  // next update
  if (sendTask.isEnabled() && !sendTask.isPaused()) {
    _nextSend.update(static_cast<int>(ceil(static_cast<double>(sendTask.getRemainingTme()) / Mycila::TaskDuration::MINUTES)), "min");
  } else {
    _nextSend.update("", "");
  }

  // time
  switch (Mycila::Modem.getTimeState()) {
    case Mycila::ModemTimeState::MODEM_TIME_OFF:
      _time.update("", DASH_STATUS_IDLE);
      break;
    case Mycila::ModemTimeState::MODEM_TIME_SYNCING:
      _time.update("Syncing...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemTimeState::MODEM_TIME_SYNCED: {
      String time = Mycila::Time::getLocalStr();
      _time.update(time.isEmpty() ? "Syncing..." : time.c_str(), time.isEmpty() ? DASH_STATUS_WARNING : DASH_STATUS_SUCCESS);
      break;
    }
    default:
      assert(false);
      break;
  }

  // GPS
  switch (Mycila::Modem.getGPSState()) {
    case Mycila::ModemGPSState::MODEM_GPS_OFF:
      _latitude.update("", DASH_STATUS_IDLE);
      _longitude.update("", DASH_STATUS_IDLE);
      _altitude.update("", DASH_STATUS_IDLE);
      break;
    case Mycila::ModemGPSState::MODEM_GPS_SYNCING:
      _latitude.update("Syncing...", DASH_STATUS_WARNING);
      _longitude.update("Syncing...", DASH_STATUS_WARNING);
      _altitude.update("Syncing...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemGPSState::MODEM_GPS_SYNCED:
      _latitude.update(String(Mycila::Modem.getGPSData().latitude, 6).c_str(), DASH_STATUS_SUCCESS);
      _longitude.update(String(Mycila::Modem.getGPSData().longitude, 6).c_str(), DASH_STATUS_SUCCESS);
      _altitude.update((String(Mycila::Modem.getGPSData().altitude) + " m").c_str(), DASH_STATUS_SUCCESS);
      break;
    case Mycila::ModemGPSState::MODEM_GPS_TIMEOUT:
      _latitude.update("Timeout!", DASH_STATUS_DANGER);
      _longitude.update("Timeout!", DASH_STATUS_DANGER);
      _altitude.update("Timeout!", DASH_STATUS_DANGER);
      break;
    default:
      assert(false);
      break;
  }

  // modem
  switch (Mycila::Modem.getState()) {
    case Mycila::ModemState::MODEM_OFF:
      _modemState.update("", DASH_STATUS_IDLE);
      break;
    case Mycila::ModemState::MODEM_STARTING:
      _modemState.update("Starting...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemState::MODEM_WAIT_REGISTRATION:
      _modemState.update("Registering...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemState::MODEM_SEARCHING:
      _modemState.update("Searching for operators...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemState::MODEM_GPS:
      _modemState.update("Waiting for GPS...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemState::MODEM_CONNECTING:
      _modemState.update("Connecting...", DASH_STATUS_WARNING);
      break;
    case Mycila::ModemState::MODEM_READY:
      _modemState.update("Ready", DASH_STATUS_SUCCESS);
      break;
    case Mycila::ModemState::MODEM_ERROR:
      _modemState.update(Mycila::Modem.getError().c_str(), DASH_STATUS_DANGER);
      break;
    default:
      assert(false);
      break;
  }

  _modemAPN.update(Mycila::Modem.getAPN().c_str(), Mycila::Modem.getAPN().isEmpty() ? DASH_STATUS_DANGER : DASH_STATUS_SUCCESS);

  // operator
  switch (Mycila::Modem.getState()) {
    case Mycila::ModemState::MODEM_OFF:
    case Mycila::ModemState::MODEM_STARTING:
      _modemOperator.update("", DASH_STATUS_IDLE);
      break;
    case Mycila::ModemState::MODEM_WAIT_REGISTRATION:
    case Mycila::ModemState::MODEM_SEARCHING: {
      const Mycila::ModemOperatorSearchResult* candidate = Mycila::Modem.getCandidate();
      _modemOperator.update(candidate ? (candidate->name + " (" + candidate->mode + ") ?").c_str() : "", candidate ? DASH_STATUS_WARNING : DASH_STATUS_IDLE);
      break;
    }
    case Mycila::ModemState::MODEM_GPS:
    case Mycila::ModemState::MODEM_CONNECTING:
      if (Mycila::Modem.getOperator().isEmpty())
        _modemOperator.update("", DASH_STATUS_WARNING);
      else
        _modemOperator.update(Mycila::Modem.getOperator().c_str(), DASH_STATUS_SUCCESS);
      break;
    case Mycila::ModemState::MODEM_READY:
      _modemOperator.update(Mycila::Modem.getOperator().c_str(), DASH_STATUS_SUCCESS);
      break;
    case Mycila::ModemState::MODEM_ERROR:
      _modemOperator.update("", DASH_STATUS_IDLE);
      break;
    default:
      assert(false);
      break;
  }

  _modemSignal.update(Mycila::Modem.getSignalQuality());

  if (Mycila::PMU.isBatteryCharging()) {
    float level = Mycila::PMU.getBatteryLevel();
    if (level > 0) {
      _power.update((String("Bat. charging: ") + static_cast<int>(floor(level))).c_str(), "%");
    } else {
      _power.update("Bat. charging...", "");
    }
  } else if (Mycila::PMU.isBatteryDischarging()) {
    float level = Mycila::PMU.getBatteryLevel();
    if (level > 0) {
      _power.update((String("Bat. discharging: ") + static_cast<int>(floor(level))).c_str(), "%");
    } else {
      _power.update("Bat. discharging...", "");
    }
  } else {
    _power.update("External", "");
  }
  _volt.update(Mycila::PMU.getBatteryVoltage());

  _uptime.update(Mycila::Time::toDHHMMSS(Mycila::System.getUptime()).c_str());
  _restart.update(!restartTask.isPaused());

  _sendNow.update(sendTask.isRunning() || (sendTask.isEnabled() && !sendTask.isPaused() && sendTask.isEarlyRunRequested()));
  _noSleepMode.update(config.getBool(KEY_NO_SLEEP_ENABLE));
  _tare.update(hx711TareTask.isRunning() || (hx711TareTask.isEnabled() && !hx711TareTask.isPaused()));

  if (_requestChartUpdate || skipWebSocketPush) {
    _requestChartUpdate = false;

    int idx = 0;
    while (idx < BEELANCE_MAX_HISTORY_SIZE) {
      _chartLatestX[idx] = emptyString;
      _chartLatestTempY[idx] = 0;
      _chartLatestWeightY[idx] = 0;

      _chartHourlyX[idx] = emptyString;
      _chartHourlyTempY[idx] = 0;
      _chartHourlyWeightY[idx] = 0;

      _chartDailyX[idx] = emptyString;
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

    _chartLatestWeight.updateX(_chartLatestX, BEELANCE_MAX_HISTORY_SIZE);
    _chartLatestWeight.updateY(_chartLatestWeightY, BEELANCE_MAX_HISTORY_SIZE);

    _chartLatestTemp.updateX(_chartLatestX, BEELANCE_MAX_HISTORY_SIZE);
    _chartLatestTemp.updateY(_chartLatestTempY, BEELANCE_MAX_HISTORY_SIZE);

    _chartHourlyWeight.updateX(_chartHourlyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartHourlyWeight.updateY(_chartHourlyWeightY, BEELANCE_MAX_HISTORY_SIZE);

    _chartHourlyTemp.updateX(_chartHourlyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartHourlyTemp.updateY(_chartHourlyTempY, BEELANCE_MAX_HISTORY_SIZE);

    _chartDailyWeight.updateX(_chartDailyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartDailyWeight.updateY(_chartDailyWeightY, BEELANCE_MAX_HISTORY_SIZE);

    _chartDailyTemp.updateX(_chartDailyX, BEELANCE_MAX_HISTORY_SIZE);
    _chartDailyTemp.updateY(_chartDailyTempY, BEELANCE_MAX_HISTORY_SIZE);
  }

  if (!skipWebSocketPush && dashboard.hasClient()) {
    dashboard.sendUpdates();
  }
}
