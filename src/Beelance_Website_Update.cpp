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

  _hostnameStat.set(Mycila::Config.get(KEY_HOSTNAME).c_str());
  _apIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::AP).toString().c_str());
  _apMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::AP).c_str());
  _wifiIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::STA).toString().c_str());
  _wifiMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::STA).c_str());
  _wifiSSIDStat.set(ESPConnect.getWiFiSSID().c_str());
  _wifiRSSIStat.set((String(ESPConnect.getWiFiRSSI()) + " dBm").c_str());
  _wifiSignalStat.set((String(ESPConnect.getWiFiSignalQuality()) + " %").c_str());

  // home

  _bhName.update(Mycila::Config.get(KEY_BEEHIVE_NAME).c_str());
  _nextSend.update(static_cast<int>(sendTask.getRemainingTme() / Mycila::TaskDuration::SECONDS));
  _uptime.update(String(Mycila::System.getUptime()));
  _modemAPN.update(Mycila::Modem.getAPN().c_str(), Mycila::Modem.getAPN().isEmpty() ? DASH_STATUS_DANGER : DASH_STATUS_SUCCESS);
  _modemSignal.update(Mycila::Modem.getSignalQuality());

  // temperature
  if (!systemTemperatureSensor.isEnabled()) {
    _temperature.update("Disabled", "");
  } else if (!systemTemperatureSensor.isValid()) {
    _temperature.update("Pending...", "");
  } else {
    _temperature.update(systemTemperatureSensor.getTemperature(), "°C");
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
    case Mycila::ModemState::MODEM_WAIT_GPS:
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
    case Mycila::ModemState::MODEM_WAIT_GPS:
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

  float batVolt = Mycila::PMU.getBatteryVoltage();
  if (batVolt < 0) {
    _batVolt.update("", DASH_STATUS_IDLE);
  } else if (batVolt < MYCILA_PMU_BATTERY_VOLTAGE_CRITICAL) {
    _batVolt.update((String(batVolt) + " V").c_str(), DASH_STATUS_DANGER);
  } else if (batVolt < MYCILA_PMU_BATTERY_VOLTAGE_NOMINAL) {
    _batVolt.update((String(batVolt) + " V").c_str(), DASH_STATUS_WARNING);
  } else {
    _batVolt.update((String(batVolt) + " V").c_str(), DASH_STATUS_SUCCESS);
  }

  float batLevel = Mycila::PMU.getBatteryLevel(batVolt);
  if (batLevel < 0)
    _batLevel.update(0);
  else
    _batLevel.update(batLevel);

  _sendNow.update(sendTask.isEarlyRunRequested() || sendTask.isRunning());
  _scanOps.update(Mycila::Modem.getState() == Mycila::ModemState::MODEM_SEARCHING);
  _noSleepMode.update(Mycila::Config.getBool(KEY_NO_SLEEP_ENABLE));
  _debugMode.update(Mycila::Config.getBool(KEY_DEBUG_ENABLE));
  _restart.update(!restartTask.isPaused());

  if (!skipWebSocketPush && dashboard.hasClient()) {
    dashboard.sendUpdates();
  }
}
