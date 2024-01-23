// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <Beelance.h>

namespace Beelance {
  class WebsiteClass {
    public:
      void init();
      void update() { _update(false); }

    private:
      Statistic _firmName = Statistic(&dashboard, "Application");
      Statistic _firmModel = Statistic(&dashboard, "Application Model");
      Statistic _firmVersionStat = Statistic(&dashboard, "Application Version");
      Statistic _firmManufacturerStat = Statistic(&dashboard, "Application Manufacturer");

      Statistic _firmFilenameStat = Statistic(&dashboard, "Firmware Filename");
      Statistic _firmHashStat = Statistic(&dashboard, "Firmware Build Hash");
      Statistic _firmTimeStat = Statistic(&dashboard, "Firmware Build Timestamp");

      Statistic _deviceIdStat = Statistic(&dashboard, "Device Id");
      Statistic _cpuModelStat = Statistic(&dashboard, "Device CPU Model");
      Statistic _cpuCoresStat = Statistic(&dashboard, "Device CPU Cores");
      Statistic _bootCountStat = Statistic(&dashboard, "Device Boot Count");
      Statistic _heapMemoryTotalStat = Statistic(&dashboard, "Device Heap Memory Total");
      Statistic _heapMemoryUsageStat = Statistic(&dashboard, "Device Heap Memory Usage");
      Statistic _heapMemoryUsedStat = Statistic(&dashboard, "Device Heap Memory Used");

      Statistic _netModeStat = Statistic(&dashboard, "Network Mode Preferred");
      Statistic _apIPStat = Statistic(&dashboard, "Access Point IP Address");
      Statistic _apMACStat = Statistic(&dashboard, "Access Point MAC Address");
      Statistic _ethIPStat = Statistic(&dashboard, "Ethernet IP Address");
      Statistic _ethMACStat = Statistic(&dashboard, "Ethernet MAC Address");
      Statistic _wifiIPStat = Statistic(&dashboard, "WiFi IP Address");
      Statistic _wifiMACStat = Statistic(&dashboard, "WiFi MAC Address");
      Statistic _wifiSSIDStat = Statistic(&dashboard, "WiFi SSID");
      Statistic _wifiRSSIStat = Statistic(&dashboard, "WiFi RSSI");
      Statistic _wifiSignalStat = Statistic(&dashboard, "WiFi Signal");

      Statistic _uptimeStat = Statistic(&dashboard, "Uptime");

      // home

      Card _bhName = Card(&dashboard, GENERIC_CARD, "Beehive Name");
      Card _systemTempState = Card(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
      Card _nextSend = Card(&dashboard, GENERIC_CARD, "Next Update", "s");

      // actions

      Card _sendNow = Card(&dashboard, BUTTON_CARD, "Update Now!");
      Card _sleepMode = Card(&dashboard, BUTTON_CARD, "Sleep Mode");
      Card _debugMode = Card(&dashboard, BUTTON_CARD, "Debug Logging");
      Card _restart = Card(&dashboard, BUTTON_CARD, "Restart");

    private:
      void _update(bool skipWebSocketPush);

      void _boolConfig(Card* card, const char* key);
      void _sliderConfig(Card* card, const char* key);

      void _status(Card* card, const char* key, bool enabled, bool state = true, const char* err = "");
      void _temperature(Card* card, Mycila::TemperatureSensor* sensor);
  };

  extern WebsiteClass Website;
} // namespace Beelance
