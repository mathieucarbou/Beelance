// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#pragma once

#include <Beelance.h>

namespace Beelance {
  class WebsiteClass {
    public:
      void init();
      void update() { _update(false); }
      void requestChartUpdate() { _requestChartUpdate = true; }
      void disableTemperature();

    private:
      Statistic _firmName = Statistic(&dashboard, "Application: Name");
      Statistic _firmVersionStat = Statistic(&dashboard, "Application: Version");
      Statistic _firmManufacturerStat = Statistic(&dashboard, "Application: Manufacturer");

      Statistic _firmFilenameStat = Statistic(&dashboard, "Firmware: Filename");
      Statistic _firmHashStat = Statistic(&dashboard, "Firmware: Build Hash");
      Statistic _firmTimeStat = Statistic(&dashboard, "Firmware: Build Timestamp");

      Statistic _deviceIdStat = Statistic(&dashboard, "Device: Id");
      Statistic _cpuModelStat = Statistic(&dashboard, "Device: CPU Model");
      Statistic _cpuCoresStat = Statistic(&dashboard, "Device: CPU Cores");
      Statistic _bootCountStat = Statistic(&dashboard, "Device: Reboot Count");
      Statistic _bootReasonStat = Statistic(&dashboard, "Device: Reboot Reason");
      Statistic _heapMemoryTotalStat = Statistic(&dashboard, "Device: Heap Memory Total");
      Statistic _heapMemoryUsageStat = Statistic(&dashboard, "Device: Heap Memory Usage");
      Statistic _heapMemoryUsedStat = Statistic(&dashboard, "Device: Heap Memory Used");

      Statistic _hx711WeightStat = Statistic(&dashboard, "HX711: Weight");
      Statistic _hx711TareStat = Statistic(&dashboard, "HX711: Tare");
      Statistic _hx711OffsetStat = Statistic(&dashboard, "HX711: Offset");
      Statistic _hx711ScaleStat = Statistic(&dashboard, "HX711: Scale");

      Statistic _modemModelStat = Statistic(&dashboard, "Modem: Model");
      Statistic _modemICCIDStat = Statistic(&dashboard, "Modem: ICCID");
      Statistic _modemIMEIStat = Statistic(&dashboard, "Modem: IMEI");
      Statistic _modemIMSIStat = Statistic(&dashboard, "Modem: IMSI");
      Statistic _modemModePrefStat = Statistic(&dashboard, "Modem: Preferred Mode");
      Statistic _modemLTEMBandsStat = Statistic(&dashboard, "Modem: LTE-M Bands");
      Statistic _modemNBIoTBandsStat = Statistic(&dashboard, "Modem: NB-IoT Bands");
      Statistic _modemIpStat = Statistic(&dashboard, "Modem: Local IP Address");

      Statistic _hostnameStat = Statistic(&dashboard, "Network: Hostname");
      Statistic _apIPStat = Statistic(&dashboard, "Network: Access Point IP Address");
      Statistic _apMACStat = Statistic(&dashboard, "Network: Access Point MAC Address");
      Statistic _wifiIPStat = Statistic(&dashboard, "Network: WiFi IP Address");
      Statistic _wifiMACStat = Statistic(&dashboard, "Network: WiFi MAC Address");
      Statistic _wifiSSIDStat = Statistic(&dashboard, "Network: WiFi SSID");
      Statistic _wifiRSSIStat = Statistic(&dashboard, "Network: WiFi RSSI");
      Statistic _wifiSignalStat = Statistic(&dashboard, "Network: WiFi Signal");

      Statistic _pmuLowBatShutThreshold = Statistic(&dashboard, "PMU: Low battery shutdown threshold");

      // overview

      Card _bhName = Card(&dashboard, GENERIC_CARD, "Beehive");
      Card _power = Card(&dashboard, GENERIC_CARD, "Power", "%");
      Card _volt = Card(&dashboard, GENERIC_CARD, "Voltage", "V");
      Card _uptime = Card(&dashboard, GENERIC_CARD, "Uptime", "s");

      Card _nextSend = Card(&dashboard, GENERIC_CARD, "Next Update", "min");
      Card _temperature = Card(&dashboard, TEMPERATURE_CARD, "Temperature", "°C");
      Card _weight = Card(&dashboard, SLIDER_CARD, "Weight", "g", 0, 200000, 100);
      Card _tare = Card(&dashboard, BUTTON_CARD, "Tare");

      Card _modemAPN = Card(&dashboard, STATUS_CARD, "APN");
      Card _modemState = Card(&dashboard, STATUS_CARD, "Modem");
      Card _modemOperator = Card(&dashboard, STATUS_CARD, "Operator");
      Card _modemSignal = Card(&dashboard, PROGRESS_CARD, "Signal Quality", "%", 0, 100);

      Card _time = Card(&dashboard, STATUS_CARD, "Time");
      Card _latitude = Card(&dashboard, STATUS_CARD, "Latitude");
      Card _longitude = Card(&dashboard, STATUS_CARD, "Longitude");
      Card _altitude = Card(&dashboard, STATUS_CARD, "Altitude");

      // actions

      Card _sendNow = Card(&dashboard, BUTTON_CARD, "Send now and sleep!");
      Card _noSleepMode = Card(&dashboard, BUTTON_CARD, "Prevent Sleep");
      Card _restart = Card(&dashboard, BUTTON_CARD, "Restart");
      Card _resetHistory = Card(&dashboard, BUTTON_CARD, "Reset Graph History");

      // graphs

      Chart _chartLatestWeight = Chart(&dashboard, BAR_CHART, "Weight (g) - Latest");
      Chart _chartHourlyWeight = Chart(&dashboard, BAR_CHART, "Weight (g) - Hourly Max");
      Chart _chartDailyWeight = Chart(&dashboard, BAR_CHART, "Weight (g) - Daily Max");

      Chart _chartLatestTemp = Chart(&dashboard, BAR_CHART, "Temperature (C) - Latest");
      Chart _chartHourlyTemp = Chart(&dashboard, BAR_CHART, "Temperature (C) - Hourly Max");
      Chart _chartDailyTemp = Chart(&dashboard, BAR_CHART, "Temperature (C) - Daily Max");

      String _chartLatestX[BEELANCE_MAX_HISTORY_SIZE];
      float _chartLatestWeightY[BEELANCE_MAX_HISTORY_SIZE];
      float _chartLatestTempY[BEELANCE_MAX_HISTORY_SIZE];

      String _chartHourlyX[BEELANCE_MAX_HISTORY_SIZE];
      float _chartHourlyWeightY[BEELANCE_MAX_HISTORY_SIZE];
      float _chartHourlyTempY[BEELANCE_MAX_HISTORY_SIZE];

      String _chartDailyX[BEELANCE_MAX_HISTORY_SIZE];
      float _chartDailyWeightY[BEELANCE_MAX_HISTORY_SIZE];
      float _chartDailyTempY[BEELANCE_MAX_HISTORY_SIZE];

      bool _requestChartUpdate = true;

    private:
      void _update(bool skipWebSocketPush);
      void _boolConfig(Card* card, const char* key);
  };

  extern WebsiteClass Website;
} // namespace Beelance
