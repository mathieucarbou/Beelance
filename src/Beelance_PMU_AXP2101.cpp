// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#define TAG "BEELANCE"

#ifdef XPOWERS_CHIP_AXP2101

// TODO: Move PMU config to task ?
void Beelance::BeelanceClass::_initPMU() {
  Mycila::Logger.info(TAG, "Initializing PMU...");

  if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, PMU_I2C_SDA, PMU_I2C_SCL)) {
    Mycila::Logger.error(TAG, "Failed to initialize PMU.....");
    assert(0);
  }

  // Set the led light on to indicate that the board has been turned on
  // pmu.setChargingLedMode(XPOWERS_CHG_LED_ON);
  // pmu.setChargingLedMode(XPOWERS_CHG_LED_OFF);
  pmu.setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ); // 1 blink per second
  // pmu.setChargingLedMode(XPOWERS_CHG_LED_BLINK_4HZ); // 4 blink per second

  // ESP32S3 power supply cannot be turned off
  // pmu.enableDC1();   // ESP32S3
  pmu.disableDC2(); // Free power port
  // pmu.disableDC3();     // MODEM
  pmu.disableDC4();   // Free power port
  pmu.disableDC5();   // Free power port
  pmu.disableALDO1(); // CAMERA DVDD
  pmu.disableALDO2(); // CAMERA DVDD
  pmu.disableALDO3(); // SDCARD
  pmu.disableALDO4(); // CAMERA AVDD
  // pmu.disableBLDO1();   // Level conversion
  pmu.disableBLDO2();   // GPS
  pmu.disableCPUSLDO(); // ???
  pmu.disableDLDO1();   // Switch Function
  pmu.disableDLDO2();   // Switch Function

  //! Do not turn off BLDO1, which controls the 3.3V power supply for level conversion.
  //! If it is turned off, it will not be able to communicate with the modem normally
  pmu.setBLDO1Voltage(3300); // Set the power supply for level conversion to 3300mV
  pmu.enableBLDO1();

  // Set the working voltage of the modem, please do not modify the parameters
  pmu.setDC3Voltage(3000); // SIM7080 Modem main power channel 2700~ 3400V
  pmu.enableDC3();

  // Modem GPS Power channel
  pmu.setBLDO2Voltage(3300);
  pmu.enableBLDO2(); // The antenna power must be turned on to use the GPS function

  // TS Pin detection must be disable, otherwise it cannot be charged
  pmu.disableTSPinMeasure();
}

#else

void Beelance::BeelanceClass::_initPMU() {}

#endif
