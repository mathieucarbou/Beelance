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
  _temperature(&_systemTempState, &systemTemperatureSensor);
  _nextSend.update(static_cast<int>(sendTask.getRemainingTme() / Mycila::TaskDuration::SECONDS));
  _uptime.update(String(Mycila::System.getUptime()));

  // pmu

  if (Mycila::PMU.isEnabled())
    _pmuStatus.update("Enabled", DASH_STATUS_SUCCESS);
  else
    _pmuStatus.update("Disabled", DASH_STATUS_IDLE);

  _modemAPN.update(Mycila::Modem.getAPN().c_str(), Mycila::Modem.getAPN().isEmpty() ? DASH_STATUS_DANGER : DASH_STATUS_SUCCESS);

  // modem & operator

  switch (Mycila::Modem.getState()) {
    case Mycila::ModemState::MODEM_OFF: {
      _modemStatus.update("Off", DASH_STATUS_IDLE);
      _modemOperator.update("", DASH_STATUS_IDLE);
      break;
    }
    case Mycila::ModemState::MODEM_STARTING: {
      _modemStatus.update("Starting...", DASH_STATUS_WARNING);
      _modemOperator.update("", DASH_STATUS_IDLE);
      break;
    }
    case Mycila::ModemState::MODEM_REGISTERING: {
      _modemStatus.update("Registering...", DASH_STATUS_WARNING);
      const Mycila::ModemOperatorSearchResult* candidate = Mycila::Modem.getCandidate();
      if (candidate) {
        String buffer = candidate->name;
        buffer += " (";
        switch (candidate->mode) {
          case Mycila::ModemMode::MODEM_MODE_LTE_M:
            buffer += "LTE-M";
            break;
          case Mycila::ModemMode::MODEM_MODE_NB_IOT:
            buffer += "NB-IoT";
            break;
          default:
            buffer += "LTE-M or NB-IoT";
            break;
        }
        buffer += ")";
        _modemOperator.update(buffer.c_str(), DASH_STATUS_WARNING);
      } else {
        _modemOperator.update("Auto select", DASH_STATUS_IDLE);
      }
      break;
    }
    case Mycila::ModemState::MODEM_SEARCHING: {
      _modemStatus.update("Searching for operators...", DASH_STATUS_WARNING);
      _modemOperator.update("Auto select", DASH_STATUS_IDLE);
      break;
    }
    case Mycila::ModemState::MODEM_CONNECTING: {
      _modemStatus.update("Connecting...", DASH_STATUS_WARNING);
      _modemOperator.update(Mycila::Modem.getOperator().c_str(), DASH_STATUS_SUCCESS);
      break;
    }
    case Mycila::ModemState::MODEM_CONNECTED: {
      _modemStatus.update("Connected", DASH_STATUS_SUCCESS);
      _modemOperator.update(Mycila::Modem.getOperator().c_str(), DASH_STATUS_SUCCESS);
      break;
    }
    case Mycila::ModemState::MODEM_ERROR: {
      _modemStatus.update(Mycila::Modem.getError().c_str(), DASH_STATUS_DANGER);
      _modemOperator.update("", DASH_STATUS_IDLE);
      break;
    }
    default:
      assert(false);
      break;
  }

  // time

  if (Mycila::Modem.getState() < Mycila::ModemState::MODEM_CONNECTING) {
    _time.update("", DASH_STATUS_IDLE);
  } else if (!Mycila::Modem.isTimeSynced()) {
    _time.update("Syncing...", DASH_STATUS_WARNING);
  } else {
    String time = Mycila::Time::getLocalStr();
    if (time.isEmpty())
      _time.update("Syncing...", DASH_STATUS_WARNING);
    else
      _time.update(time.c_str(), DASH_STATUS_SUCCESS);
  }

  // GPS

  if (Mycila::Modem.getState() < Mycila::ModemState::MODEM_CONNECTING) {
    _latitude.update("", DASH_STATUS_IDLE);
    _longitude.update("", DASH_STATUS_IDLE);
    _altitude.update("", DASH_STATUS_IDLE);
  } else if (!Mycila::Modem.isGPSSynced()) {
    _latitude.update("Syncing...", DASH_STATUS_WARNING);
    _longitude.update("Syncing...", DASH_STATUS_WARNING);
    _altitude.update("Syncing...", DASH_STATUS_WARNING);
  } else {
    _latitude.update(String(Mycila::Modem.getGPSData().latitude, 6).c_str(), DASH_STATUS_SUCCESS);
    _longitude.update(String(Mycila::Modem.getGPSData().longitude, 6).c_str(), DASH_STATUS_SUCCESS);
    _altitude.update((String(Mycila::Modem.getGPSData().altitude) + " m").c_str(), DASH_STATUS_SUCCESS);
  }

  _sendNow.update(sendTask.isEarlyRunRequested() || sendTask.isRunning());
  _noSleepMode.update(Mycila::Config.getBool(KEY_NO_SLEEP_ENABLE));
  _debugMode.update(Mycila::Config.getBool(KEY_DEBUG_ENABLE));

  if (!skipWebSocketPush && dashboard.hasClient()) {
    dashboard.sendUpdates();
  }
}

void Beelance::WebsiteClass::_temperature(Card* card, Mycila::TemperatureSensor* sensor) {
  if (!sensor->isEnabled()) {
    card->update("Disabled", "");
  } else if (!sensor->isValid()) {
    card->update("Pending...", "");
  } else {
    card->update(sensor->getTemperature(), "°C");
  }
}
