// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <Beelance.h>

#include <map>
#include <mutex>

namespace Beelance {
  typedef struct {
      float temperature;
      int32_t weight;
  } Measurement;

  class BeelanceClass {
    public:
      void begin() {
        _initConfig();
        _initHTTPd();
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
      void toJson(const JsonObject& root);
      void historyToJson(const JsonObject& root);
      void clearHistory();

    private:
      void _initConfig();
      void _initHTTPd();
      void _initTasks();
      void _initEventHandlers();
      void _initWebsite();
      void _initREST();
      void _recordMeasurement(const time_t timestamp, const float temperature, const int32_t weight);
      void _loadHistory();
      void _saveHistory();
      void _prune();

    private:
      static double _round2(double v);

    public:
      std::map<const String, Measurement> hourlyHistory;
      std::map<const String, Measurement> dailyHistory;
      std::mutex _mutex;
  };

  extern BeelanceClass Beelance;
} // namespace Beelance
