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

      Statistic _modemModelStat = Statistic(&dashboard, "Modem Model");
      Statistic _modemICCIDStat = Statistic(&dashboard, "Modem ICCID");
      Statistic _modemIMEIStat = Statistic(&dashboard, "Modem IMEI");
      Statistic _modemIMSIStat = Statistic(&dashboard, "Modem IMSI");
      Statistic _modemModePrefStat = Statistic(&dashboard, "Modem Preferred Mode");
      Statistic _modemLTEMBandsStat = Statistic(&dashboard, "Modem LTE-M Bands");
      Statistic _modemNBIoTBandsStat = Statistic(&dashboard, "Modem NB-IoT Bands");
      Statistic _modemIpStat = Statistic(&dashboard, "Modem Local IP Address");

      Statistic _hostnameStat = Statistic(&dashboard, "Hostname");
      Statistic _apIPStat = Statistic(&dashboard, "Access Point IP Address");
      Statistic _apMACStat = Statistic(&dashboard, "Access Point MAC Address");
      Statistic _wifiIPStat = Statistic(&dashboard, "WiFi IP Address");
      Statistic _wifiMACStat = Statistic(&dashboard, "WiFi MAC Address");
      Statistic _wifiSSIDStat = Statistic(&dashboard, "WiFi SSID");
      Statistic _wifiRSSIStat = Statistic(&dashboard, "WiFi RSSI");
      Statistic _wifiSignalStat = Statistic(&dashboard, "WiFi Signal");

      // overview

      Card _bhName = Card(&dashboard, GENERIC_CARD, "Beehive");
      Card _weight = Card(&dashboard, GENERIC_CARD, "Weight", "kg");
      Card _temperature = Card(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
      Card _nextSend = Card(&dashboard, GENERIC_CARD, "Next Update", "s");

      Card _time = Card(&dashboard, STATUS_CARD, "Time");
      Card _latitude = Card(&dashboard, STATUS_CARD, "Latitude");
      Card _longitude = Card(&dashboard, STATUS_CARD, "Longitude");
      Card _altitude = Card(&dashboard, STATUS_CARD, "Altitude");

      Card _modemState = Card(&dashboard, STATUS_CARD, "Modem");
      Card _modemAPN = Card(&dashboard, STATUS_CARD, "APN");
      Card _modemOperator = Card(&dashboard, STATUS_CARD, "Operator");
      Card _modemSignal = Card(&dashboard, PROGRESS_CARD, "Signal Quality", "%", 0, 100);

      Card _batVolt = Card(&dashboard, STATUS_CARD, "Battery Voltage");
      Card _batLevel = Card(&dashboard, PROGRESS_CARD, "Battery Level", "%", 0, 100);
      Card _uptime = Card(&dashboard, GENERIC_CARD, "Uptime", "s");
      Card _restart = Card(&dashboard, BUTTON_CARD, "Restart");

      Card _scanOps = Card(&dashboard, BUTTON_CARD, "Scan Operators");
      Card _sendNow = Card(&dashboard, BUTTON_CARD, "Send now and sleep!");
      Card _noSleepMode = Card(&dashboard, BUTTON_CARD, "Prevent Sleep");
      Card _debugMode = Card(&dashboard, BUTTON_CARD, "Debug Logging");

    private:
      void _update(bool skipWebSocketPush);
      void _boolConfig(Card* card, const char* key);
  };

  extern WebsiteClass Website;
} // namespace Beelance
