// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <XPowersLib.h>

// https://gist.github.com/exocode/90339d7f946ad5f83dd1cf29bf5df0dc#under-load
#ifndef MYCILA_PMU_BATTERY_VOLTAGE_MAX
#define MYCILA_PMU_BATTERY_VOLTAGE_MAX 4.2
#endif

#ifndef MYCILA_PMU_BATTERY_VOLTAGE_NOMINAL
#define MYCILA_PMU_BATTERY_VOLTAGE_NOMINAL 3.7
#endif

#ifndef MYCILA_PMU_BATTERY_VOLTAGE_CRITICAL
#define MYCILA_PMU_BATTERY_VOLTAGE_CRITICAL 3.5
#endif

#ifndef MYCILA_PMU_BATTERY_VOLTAGE_MIN
#define MYCILA_PMU_BATTERY_VOLTAGE_MIN 3.2
#endif

#if defined(MYCILA_PMU_I2C_SDA) && defined(MYCILA_PMU_I2C_SCL)
#if defined(MYCILA_PMU_BATTERY_ADC_PIN)
#error "Cannot define both MYCILA_PMU_BATTERY_ADC_PIN and MYCILA_PMU_I2C_SDA/MYCILA_PMU_I2C_SCL"
#endif
#define MYCILA_XPOWERS_PMU_ENABLED
#endif

namespace Mycila {
  class PMUClass {
    public:
      void begin();

      float getVoltage();
      float getBatteryVoltage();
      float getBatteryLevel();
      bool isBatteryPowered();
      bool isExternallyPowered();

      void enableModem();
      void enableGPS();
      void enableDCPins();
      void enableSDCard();
      void enableCamera();
      void setChargingLedMode(xpowers_chg_led_mode_t mode);
      void powerOff();

    private:
      float _voltage = -1;
      uint32_t _lastVoltageRefreshTime = 0;
#ifdef MYCILA_XPOWERS_PMU_ENABLED
      XPowersPMU _pmu;
#endif
  };

  extern PMUClass PMU;
} // namespace Mycila
