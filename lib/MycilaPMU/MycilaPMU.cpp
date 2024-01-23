// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <MycilaPMU.h>

#define TAG "PMU"

void Mycila::PMUClass::begin(int sda, int scl) {
  if (_enabled)
    return;

  if (sda < 0 || scl < 0) {
    ESP_LOGW(TAG, "PMU not enabled: I2C pins not defined");
    return;
  }

#ifdef MYCILA_PMU_ENABLED
  if (!_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, sda, scl)) {
    return;
  }

  _pmu.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

  // ESP32S3 power supply cannot be turned off
  // _pmu.enableDC1();   // ESP32S3
  _pmu.disableDC2();     // Free power port
  _pmu.disableDC3();     // MODEM
  _pmu.disableDC4();     // Free power port
  _pmu.disableDC5();     // Free power port
  _pmu.disableALDO1();   // CAMERA DVDD
  _pmu.disableALDO2();   // CAMERA DVDD
  _pmu.disableALDO3();   // SDCARD
  _pmu.disableALDO4();   // CAMERA AVDD
  _pmu.disableBLDO1();   // Level conversion
  _pmu.disableBLDO2();   // GPS
  _pmu.disableCPUSLDO(); // ???
  _pmu.disableDLDO1();   // Switch Function
  _pmu.disableDLDO2();   // Switch Function

  // TS Pin detection must be disable, otherwise it cannot be charged
  _pmu.disableTSPinMeasure();
#endif

  _enabled = true;
}

void Mycila::PMUClass::enableModem() {
  if (!_enabled)
    return;

#ifdef MYCILA_PMU_ENABLED
  _pmu.enableDC3();
  _pmu.setDC3Voltage(3000); // SIM7080 Modem main power channel 2700~ 3400V

  _pmu.enableBLDO1();
  _pmu.setBLDO1Voltage(3300); // Set the power supply for level conversion to 3300mV
#endif
}

void Mycila::PMUClass::enableGPS() {
  if (!_enabled)
    return;

#ifdef MYCILA_PMU_ENABLED
  _pmu.enableBLDO2(); // The antenna power must be turned on to use the GPS function
  _pmu.setBLDO2Voltage(3300);
#endif
}

void Mycila::PMUClass::setChargingLedMode(xpowers_chg_led_mode_t mode) {
  if (!_enabled)
    return;

#ifdef MYCILA_PMU_ENABLED
  _pmu.setChargingLedMode(mode);
#endif
}

namespace Mycila {
  PMUClass PMU;
} // namespace Mycila
