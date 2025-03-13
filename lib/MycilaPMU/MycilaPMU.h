// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#pragma once

#include <ArduinoJson.h>

// https://gist.github.com/exocode/90339d7f946ad5f83dd1cf29bf5df0dc#under-load
#ifndef MYCILA_PMU_BATTERY_VOLTAGE_MAX
  #define MYCILA_PMU_BATTERY_VOLTAGE_MAX 4.2f
#endif

#ifndef MYCILA_PMU_BATTERY_VOLTAGE_NOMINAL
  #define MYCILA_PMU_BATTERY_VOLTAGE_NOMINAL 3.7f
#endif

#ifndef MYCILA_PMU_BATTERY_VOLTAGE_CRITICAL
  #define MYCILA_PMU_BATTERY_VOLTAGE_CRITICAL 3.5f
#endif

#ifndef MYCILA_PMU_BATTERY_VOLTAGE_MIN
  #define MYCILA_PMU_BATTERY_VOLTAGE_MIN 3.2f
#endif

#if defined(MYCILA_PMU_I2C_SDA) && defined(MYCILA_PMU_I2C_SCL)
  #if defined(MYCILA_PMU_BATTERY_ADC_PIN)
    #error "Cannot define both MYCILA_PMU_BATTERY_ADC_PIN and MYCILA_PMU_I2C_SDA/MYCILA_PMU_I2C_SCL"
  #endif
  #include <XPowersLib.h>
  #define MYCILA_XPOWERS_PMU_ENABLED 1
#else
typedef enum __xpowers_chg_led_mode {
  XPOWERS_CHG_LED_OFF,
  XPOWERS_CHG_LED_BLINK_1HZ,
  XPOWERS_CHG_LED_BLINK_4HZ,
  XPOWERS_CHG_LED_ON,
  XPOWERS_CHG_LED_CTRL_CHG, // The charging indicator is controlled by the charger
} xpowers_chg_led_mode_t;
#endif

namespace Mycila {
  class PMUClass {
    public:
      void begin();

      float getBatteryVoltage() const { return _batteryVoltage; }
      float getBatteryLevel() const;
      bool isBatteryCharging() const;
      bool isBatteryDischarging() const;
      bool isBatteryConnected() const;

      uint8_t readLowBatteryShutdownThreshold();
      float read();

      void enableModem();
      void enableGPS();
      void enableDCPins();
      void enableSDCard();
      void enableCamera();
      void setChargingLedMode(xpowers_chg_led_mode_t mode);
      void setChargingCurrent(int current);
      void powerOff();
      void reset();

      void toJson(const JsonObject& root) const;

    private:
      float _batteryVoltage = 0;
#ifdef MYCILA_XPOWERS_PMU_ENABLED
      XPowersPMU _pmu;
      bool _pmuBatteryConnected = false;
      bool _pmuBatteryCharging = false;
      bool _pmuBatteryDischarging = false;
      xpowers_axp2101_chg_curr_t _chargingCurrent = xpowers_axp2101_chg_curr_t::XPOWERS_AXP2101_CHG_CUR_500MA;
#endif
  };

  extern PMUClass PMU;
} // namespace Mycila
