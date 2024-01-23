// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <MycilaPMU.h>

#include <MycilaLogger.h>
#include <esp_adc_cal.h>

#include <algorithm>

#define TAG "PMU"

void Mycila::PMUClass::begin() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  if (MYCILA_PMU_I2C_SDA < 0 || MYCILA_PMU_I2C_SCL < 0) {
    ESP_LOGW(TAG, "PMU not enabled: I2C pins not defined");
    return;
  }

  if (!_pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, MYCILA_PMU_I2C_SDA, MYCILA_PMU_I2C_SCL)) {
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
}

float Mycila::PMUClass::getVoltage() {
  if (millis() - _lastVoltageRefreshTime >= 2000) {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
    _voltage = _pmu.getBattVoltage() / 1000.0;
#endif
#ifdef MYCILA_PMU_BATTERY_ADC_PIN
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    _voltage = esp_adc_cal_raw_to_voltage(analogRead(MYCILA_PMU_BATTERY_ADC_PIN), &adc_chars) * 2 / 1000.0;
#endif
    _lastVoltageRefreshTime = millis();
  }
  return _voltage;
}

float Mycila::PMUClass::getBatteryVoltage() {
  return isBatteryPowered() ? _voltage : 0;
}

float Mycila::PMUClass::getBatteryLevel() {
  float voltage = getBatteryVoltage();
  // map() equivalent with float
  constexpr float run = MYCILA_PMU_BATTERY_VOLTAGE_MAX - MYCILA_PMU_BATTERY_VOLTAGE_MIN;
  return run == 0 || voltage < MYCILA_PMU_BATTERY_VOLTAGE_MIN ? 0 : min(100.0, ((voltage - MYCILA_PMU_BATTERY_VOLTAGE_MIN) * 100.0) / run);
}

bool Mycila::PMUClass::isExternallyPowered() {
  return getVoltage() > MYCILA_PMU_BATTERY_VOLTAGE_MAX;
}

bool Mycila::PMUClass::isBatteryPowered() {
  float voltage = getVoltage();
  // ADC does not work when USB is connected and switch is off (no battery to measure)
  // The measured voltage is between 0 and 1.
  // When charging (usb-c connected and battery present), the voltage is greater than the maximum voltage of the battery.
  return voltage <= MYCILA_PMU_BATTERY_VOLTAGE_MAX && (voltage <= 0 || voltage >= 1);
}

void Mycila::PMUClass::enableModem() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.setBLDO1Voltage(3300); // Set the power supply for level conversion to 3300mV
  _pmu.enableBLDO1();

  _pmu.setDC3Voltage(3000); // SIM7080 Modem main power channel 2700~ 3400V
  _pmu.enableDC3();
#endif
}

void Mycila::PMUClass::enableGPS() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.setBLDO2Voltage(3300);
  _pmu.enableBLDO2(); // The antenna power must be turned on to use the GPS function
#endif
}

void Mycila::PMUClass::enableDCPins() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.setDC2Voltage(3300);
  _pmu.enableDC2(); // Free power port
  _pmu.setDC4Voltage(3300);
  _pmu.enableDC4(); // Free power port
  _pmu.setDC5Voltage(3300);
  _pmu.enableDC5(); // Free power port
#endif
}

void Mycila::PMUClass::enableSDCard() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.setALDO3Voltage(3300);
  _pmu.enableALDO3(); // SDCARD
#endif
}

void Mycila::PMUClass::enableCamera() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.setALDO1Voltage(1800);
  _pmu.enableALDO1(); // CAMERA DVDD
  _pmu.setALDO2Voltage(2800);
  _pmu.enableALDO2(); // CAMERA DVDD
  _pmu.setALDO4Voltage(3000);
  _pmu.enableALDO4(); // CAMERA AVDD
#endif
}

void Mycila::PMUClass::setChargingLedMode(xpowers_chg_led_mode_t mode) {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.setChargingLedMode(mode);
#endif

#ifdef MYCILA_BOARD_LED_PIN
  pinMode(MYCILA_BOARD_LED_PIN, OUTPUT);
  switch (mode) {
    case XPOWERS_CHG_LED_ON:
    case XPOWERS_CHG_LED_BLINK_1HZ:
    case XPOWERS_CHG_LED_BLINK_4HZ:
      Mycila::Logger.info(TAG, "LED: ON");
      digitalWrite(MYCILA_BOARD_LED_PIN, HIGH);
      break;
    default:
      Mycila::Logger.info(TAG, "LED: OFF");
      digitalWrite(MYCILA_BOARD_LED_PIN, LOW);
      break;
  }
#endif
}

void Mycila::PMUClass::powerOff() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  // Turn off ADC data monitoring to save power
  _pmu.disableBattDetection();
  _pmu.disableTemperatureMeasure();
  _pmu.disableVbusVoltageMeasure();
  _pmu.disableBattVoltageMeasure();
  _pmu.disableSystemVoltageMeasure();
  // Turn off the power output of other channels
  _pmu.disableDC2();
  _pmu.disableDC3();
  _pmu.disableDC4();
  _pmu.disableDC5();
  _pmu.disableALDO1();
  _pmu.disableALDO2();
  _pmu.disableALDO3();
  _pmu.disableALDO4();
  _pmu.disableBLDO1();
  _pmu.disableBLDO2();
  _pmu.disableCPUSLDO();
  _pmu.disableDLDO1();
  _pmu.disableDLDO2();
  // Close other IRQs
  _pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Clear PMU Interrupt Status Register // I applied a short press interrupt on PMU
  _pmu.clearIrqStatus();
  // Turn off the charging indicator to save power
  _pmu.setChargingLedMode(XPOWERS_CHG_LED_OFF);
#endif
#ifdef MYCILA_BOARD_LED_PIN
  digitalWrite(MYCILA_BOARD_LED_PIN, LOW);
#endif
}

namespace Mycila {
  PMUClass PMU;
} // namespace Mycila
