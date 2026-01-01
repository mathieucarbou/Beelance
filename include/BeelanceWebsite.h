// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#include <Beelance.h>

#include <string>

namespace Beelance {
  class WebsiteClass {
    public:
      void init();
      void update() { _update(false); }
      void requestChartUpdate() { _requestChartUpdate = true; }
      void disableTemperature();

    private:
      std::string _chartLatestX[BEELANCE_MAX_HISTORY_SIZE];
      float _chartLatestWeightY[BEELANCE_MAX_HISTORY_SIZE];
      float _chartLatestTempY[BEELANCE_MAX_HISTORY_SIZE];

      std::string _chartHourlyX[BEELANCE_MAX_HISTORY_SIZE];
      float _chartHourlyWeightY[BEELANCE_MAX_HISTORY_SIZE];
      float _chartHourlyTempY[BEELANCE_MAX_HISTORY_SIZE];

      std::string _chartDailyX[BEELANCE_MAX_HISTORY_SIZE];
      float _chartDailyWeightY[BEELANCE_MAX_HISTORY_SIZE];
      float _chartDailyTempY[BEELANCE_MAX_HISTORY_SIZE];

      bool _requestChartUpdate = true;

    private:
      void _update(bool skipWebSocketPush);
      void _boolConfig(dash::ToggleButtonCard* card, const char* key);
  };

  extern WebsiteClass Website;
} // namespace Beelance
