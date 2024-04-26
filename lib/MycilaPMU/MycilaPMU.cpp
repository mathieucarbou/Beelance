// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <MycilaPMU.h>

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
  // Set the minimum common working voltage of the PMU VBUS input,
  // below this value will turn off the PMU
  // _pmu.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);

  // Set the maximum current of the PMU VBUS input,
  // higher than this value will turn off the PMU
  // _pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_2000MA);

  // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
  // _pmu.setSysPowerDownVoltage(2600);

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

  _pmu.enableBattDetection();
  _pmu.enableBattVoltageMeasure();
  _pmu.enableVbusVoltageMeasure();
  _pmu.enableSystemVoltageMeasure();
  _pmu.enableTemperatureMeasure();

  // TS Pin detection must be disable, otherwise it cannot be charged
  _pmu.disableTSPinMeasure();

  // Disable all interrupts
  _pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Clear all interrupt flags
  _pmu.clearIrqStatus();

  // Enable the required interrupt function
  _pmu.enableIRQ(
    XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |      // BATTERY
    XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |    // VBUS
    XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |       // POWER KEY
    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ); // CHARGE

  // Set the precharge charging current
  _pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
  // Set constant current charge current limit
  _pmu.setChargerConstantCurr(_chargingCurrent);
  // Set stop charging termination current
  _pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
  // Set charge cut-off voltage
  _pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  // Set the time of pressing the button to turn off
  _pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
#endif
}

float Mycila::PMUClass::getBatteryLevel() const {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  if (!_pmuBatteryConnected)
    return 0;
#endif
  if (_batteryVoltage < MYCILA_PMU_BATTERY_VOLTAGE_MIN)
    return 0;
  if (_batteryVoltage > MYCILA_PMU_BATTERY_VOLTAGE_MAX)
    return 0;
  // map() equivalent with float
  constexpr float run = MYCILA_PMU_BATTERY_VOLTAGE_MAX - MYCILA_PMU_BATTERY_VOLTAGE_MIN;
  return run <= 0 ? 0 : min(100.0, ((_batteryVoltage - MYCILA_PMU_BATTERY_VOLTAGE_MIN) * 100.0) / run);
}

bool Mycila::PMUClass::isBatteryCharging() const {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  return _pmuBatteryCharging;
#else
  // ADC does not work when USB is connected and switch is off (no battery to measure)
  // The measured voltage is between 0 and 1.
  // When charging (usb-c connected and battery present), the voltage is greater than the maximum voltage of the battery.
  return _batteryVoltage > MYCILA_PMU_BATTERY_VOLTAGE_MAX || (_batteryVoltage > 0 && _batteryVoltage < 1);
#endif
}

bool Mycila::PMUClass::isBatteryDischarging() const {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  return _pmuBatteryDischarging;
#else
  // ADC does not work when USB is connected and switch is off (no battery to measure)
  // The measured voltage is between 0 and 1.
  // When charging (usb-c connected and battery present), the voltage is greater than the maximum voltage of the battery.
  return _batteryVoltage <= MYCILA_PMU_BATTERY_VOLTAGE_MAX && _batteryVoltage >= 1;
#endif
}

bool Mycila::PMUClass::isBatteryConnected() const {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  return _pmuBatteryConnected;
#else
  // ADC does not work when USB is connected and switch is off (no battery to measure)
  // The measured voltage is between 0 and 1.
  // When charging (usb-c connected and battery present), the voltage is greater than the maximum voltage of the battery.
  return _batteryVoltage <= MYCILA_PMU_BATTERY_VOLTAGE_MAX || (_batteryVoltage > 0 && _batteryVoltage < 1);
#endif
}

float Mycila::PMUClass::read() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _batteryVoltage = _pmu.getBattVoltage() / 1000.0;
  //  getBatteryPercent() cannot be used because it requires a full charge and discharge cycle
  // _pmu.getBatteryPercent();
  _pmuBatteryConnected = _pmu.isBatteryConnect();
  _pmuBatteryCharging = _pmu.isCharging();
  _pmuBatteryDischarging = _pmu.isDischarge();
#endif
#ifdef MYCILA_PMU_BATTERY_ADC_PIN
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  _batteryVoltage = esp_adc_cal_raw_to_voltage(analogRead(MYCILA_PMU_BATTERY_ADC_PIN), &adc_chars) * 2 / 1000.0;
#endif
  return _batteryVoltage;
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
      digitalWrite(MYCILA_BOARD_LED_PIN, HIGH);
      break;
    default:
      digitalWrite(MYCILA_BOARD_LED_PIN, LOW);
      break;
  }
#endif
}

void Mycila::PMUClass::setChargingCurrent(int chargingCurrent) {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  if (chargingCurrent >= 500)
    _chargingCurrent = xpowers_axp2101_chg_curr_t::XPOWERS_AXP2101_CHG_CUR_500MA;
  else if (chargingCurrent >= 400)
    _chargingCurrent = xpowers_axp2101_chg_curr_t::XPOWERS_AXP2101_CHG_CUR_400MA;
  else if (chargingCurrent >= 300)
    _chargingCurrent = xpowers_axp2101_chg_curr_t::XPOWERS_AXP2101_CHG_CUR_300MA;
  else if (chargingCurrent >= 200)
    _chargingCurrent = xpowers_axp2101_chg_curr_t::XPOWERS_AXP2101_CHG_CUR_200MA;
  else
    _chargingCurrent = xpowers_axp2101_chg_curr_t::XPOWERS_AXP2101_CHG_CUR_100MA;
  _pmu.setChargerConstantCurr(_chargingCurrent);
#endif
}

void Mycila::PMUClass::powerOff() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
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
  _pmu.setChargingLedMode(XPOWERS_CHG_LED_OFF);
#endif
#ifdef MYCILA_BOARD_LED_PIN
  digitalWrite(MYCILA_BOARD_LED_PIN, LOW);
#endif
}

void Mycila::PMUClass::reset() {
#ifdef MYCILA_XPOWERS_PMU_ENABLED
  _pmu.reset();
#endif
}

void Mycila::PMUClass::toJson(const JsonObject& root) const {
  root["powered_by"] = isBatteryDischarging() ? "bat" : "ext";
  root["battery_level"] = getBatteryLevel();
  root["battery_voltage"] = _batteryVoltage;
  root["battery_charging"] = isBatteryCharging();
  root["battery_discharging"] = isBatteryDischarging();
  root["battery_connected"] = isBatteryConnected();
}

namespace Mycila {
  PMUClass PMU;
} // namespace Mycila
