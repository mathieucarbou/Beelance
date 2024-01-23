// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <Beelance.h>

namespace Beelance {
  class BeelanceClass {
    public:
      void begin() {
        _initConfig();
        _initHTTPd();
        _initTasks();
        _initEventHandlers();
        _initWebsite();
        _initREST();
      }

      bool isNightModeActive() const;
      uint32_t getDelayUntilNextSend() const;

      void sleep(uint32_t seconds);
      void updateWebsite();
      bool sendMeasurements();
      void toJson(const JsonObject& root);

    private:
      void _initConfig();
      void _initHTTPd();
      void _initTasks();
      void _initEventHandlers();
      void _initWebsite();
      void _initREST();

      static double _round2(double v);
  };

  extern BeelanceClass Beelance;
} // namespace Beelance
