// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <XPowersLib.h>

#if defined(MYCILA_PMU_I2C_SDA) && defined(MYCILA_PMU_I2C_SCL)
#define MYCILA_PMU_ENABLED
#else
#define MYCILA_PMU_I2C_SDA -1
#define MYCILA_PMU_I2C_SCL -1
#endif

namespace Mycila {
  class PMUClass {
    public:
      void begin(int sda = MYCILA_PMU_I2C_SDA, int scl = MYCILA_PMU_I2C_SCL);

      bool isEnabled() const { return _enabled; }

      void enableModem();
      void enableGPS();
      void setChargingLedMode(xpowers_chg_led_mode_t mode);

    private:
      bool _enabled = false;
#ifdef MYCILA_PMU_ENABLED
      XPowersPMU _pmu;
#endif
  };

  extern PMUClass PMU;
} // namespace Mycila
