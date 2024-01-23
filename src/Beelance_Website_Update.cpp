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
  ESPConnectMode mode = ESPConnect.getMode();
  _apIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::AP).toString().c_str());
  _apMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::AP).c_str());
  _ethIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::ETH).toString().c_str());
  _ethMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::ETH).c_str());
  _heapMemoryUsageStat.set((String(memory.usage) + " %").c_str());
  _heapMemoryUsedStat.set((String(memory.used) + " bytes").c_str());
  _netModeStat.set(mode == ESPConnectMode::AP ? "AP" : (mode == ESPConnectMode::STA ? "WiFi" : (mode == ESPConnectMode::ETH ? "Ethernet" : "")));
  _uptimeStat.set((String(Mycila::System.getUptime()) + " s").c_str());
  _wifiIPStat.set(ESPConnect.getIPAddress(ESPConnectMode::STA).toString().c_str());
  _wifiMACStat.set(ESPConnect.getMACAddress(ESPConnectMode::STA).c_str());
  _wifiRSSIStat.set((String(ESPConnect.getWiFiRSSI()) + " dBm").c_str());
  _wifiSignalStat.set((String(ESPConnect.getWiFiSignalQuality()) + " %").c_str());
  _wifiSSIDStat.set(ESPConnect.getWiFiSSID().c_str());

  // home

  _temperature(&_systemTempState, &systemTemperatureSensor);
  _debugMode.update(Mycila::Config.getBool(KEY_DEBUG_ENABLE));
  _sleepMode.update(Mycila::Config.getBool(KEY_SLEEP_ENABLE));
  _nextSend.update(static_cast<int>(sendTask.getRemainingTme() / Mycila::TaskDuration::SECONDS));

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

void Beelance::WebsiteClass::_status(Card* card, const char* key, bool enabled, bool active, const char* err) {
  const bool configEnabled = Mycila::Config.getBool(key);
  if (!configEnabled)
    card->update("Disabled", DASH_STATUS_IDLE);
  else if (!enabled)
    card->update("Unable to start", DASH_STATUS_DANGER);
  else if (!active)
    card->update(err, DASH_STATUS_WARNING);
  else
    card->update("Enabled", DASH_STATUS_SUCCESS);
}
