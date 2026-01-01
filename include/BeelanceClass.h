// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) Mathieu Carbou
 */
#pragma once

#include <Beelance.h>

#include <mutex>
#include <string>
#include <vector>

namespace Beelance {
  typedef struct {
      std::string time;
      float temperature;
      int32_t weight;
  } Measurement;

  class BeelanceClass {
    public:
      BeelanceClass();

      void begin() {
        _initConfig();
        _initTasks();
        _initEventHandlers();
        _initWebsite();
        _initREST();
        _loadHistory();
      }

      bool isNightModeActive() const;
      uint32_t getDelayUntilNextSend() const;

      void sleep(uint32_t seconds);
      void updateWebsite();
      bool sendMeasurements();
      void toJson(const JsonObject& root) const;
      void historyToJson(const JsonObject& root) const;
      void clearHistory();
      bool mustSleep() const;

    private:
      void _initConfig();
      void _initTasks();
      void _initEventHandlers();
      void _initWebsite();
      void _initREST();
      void _recordMeasurement(const time_t timestamp, const float temperature, const int32_t weight);
      void _loadHistory();
      void _saveHistory();

    private:
      static float _round2(float v);

    public:
      std::vector<Measurement> latestHistory;
      std::vector<Measurement> hourlyHistory;
      std::vector<Measurement> dailyHistory;
      std::mutex _mutex;
  };

  extern BeelanceClass Beelance;
} // namespace Beelance
