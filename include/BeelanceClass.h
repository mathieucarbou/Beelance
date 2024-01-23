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

      void updateWebsite();

      // in seconds
      void sendMeasurements();

    private:
      void _initConfig();
      void _initHTTPd();
      void _initTasks();
      void _initEventHandlers();
      void _initWebsite();
      void _initREST();
  };

  extern BeelanceClass Beelance;
} // namespace Beelance
