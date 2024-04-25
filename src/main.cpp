// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>
#include <BeelanceWebsite.h>

#ifdef BEELANCE_DISABLE_BROWNOUT_DETECTOR
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#endif

#define TAG "BEELANCE"

AsyncWebServer webServer(80);
ESPDash dashboard = ESPDash(&webServer, "/dashboard", false);

Mycila::TaskManager hx711TaskManager("hx711", 3);
Mycila::TaskManager loopTaskManager("loopTask", 12);
Mycila::TaskManager modemTaskManager("modemTask", 5);

Mycila::HX711 hx711;
Mycila::TemperatureSensor temperatureSensor;

float calibrationWeight = 0;

// setup
void setup() {
#ifdef BEELANCE_DISABLE_BROWNOUT_DETECTOR
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif

  Serial.begin(BEELANCE_SERIAL_BAUDRATE);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);
  delay(100);
#else
  while (!Serial)
    yield();
#endif

  // logger
  Mycila::Logger.getOutputs().reserve(2);
  Mycila::Logger.forwardTo(&Serial);
  Mycila::Logger.info(TAG, "Booting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());

  // system
  Mycila::System.begin();

  // load config and initialize
  Beelance::Beelance.begin();

  Mycila::Logger.info(TAG, "Starting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());

  // PMU
  Mycila::Logger.info(TAG, "Configure PMU...");
  Mycila::PMU.begin();
  Mycila::PMU.enableDCPins();
  Mycila::PMU.setChargingLedMode(XPOWERS_CHG_LED_ON);
  Mycila::PMU.setChargingCurrent(Mycila::Config.get(KEY_PMU_CHARGING_CURRENT).toInt());
  Mycila::Logger.info(TAG, "Powering Modem...");
  Mycila::PMU.enableModem();
  Mycila::Logger.info(TAG, "Powering GPS...");
  Mycila::PMU.enableGPS();

  // Temperature
  Mycila::Logger.info(TAG, "Configure Temperature Sensor...");
  temperatureSensor.begin(Mycila::Config.get(KEY_TEMPERATURE_PIN).toInt(), 30);
  if (!temperatureSensor.isEnabled()) {
    Beelance::Website.disableTemperature();
  }

  // HX711
  Mycila::Logger.info(TAG, "Configure HX711...");
  hx711.setOffset(Mycila::Config.get(KEY_HX711_OFFSET).toInt());
  hx711.setScale(Mycila::Config.get(KEY_HX711_SCALE).toFloat());
  hx711.setExpirationDelay(10);
  hx711.begin(Mycila::Config.get(KEY_HX711_DATA_PIN).toInt(), Mycila::Config.get(KEY_HX711_CLOCK_PIN).toInt());

  // stack monitor
  Mycila::Logger.info(TAG, "Configure Task Stack Monitor...");
  Mycila::TaskMonitor.begin(5);
  Mycila::TaskMonitor.addTask("async_tcp"); // ESPAsyncTCP
  Mycila::TaskMonitor.addTask("loopTask");  // Arduino
  Mycila::TaskMonitor.addTask("modemTask"); // Modem
  Mycila::TaskMonitor.addTask("hx711");     // HX711

  // network
  Mycila::Logger.info(TAG, "Configure Network...");
  webServer.end();
  mdns_service_remove("_http", "_tcp");
  ESPConnect.end();
  Mycila::Logger.info(TAG, "Hostname: %s", Mycila::Config.get(KEY_HOSTNAME).c_str());
  ESPConnect.setAutoRestart(false);
  ESPConnect.setBlocking(false);
  ESPConnect.setCaptivePortalTimeout(Mycila::Config.get(KEY_CAPTURE_PORTAL_TIMEOUT).toInt());
  ESPConnect.setConnectTimeout(Mycila::Config.get(KEY_WIFI_CONNECTION_TIMEOUT).toInt());
  ESPConnect.begin(&webServer, Mycila::Config.get(KEY_HOSTNAME), Mycila::AppInfo.name + "-" + Mycila::AppInfo.id, Mycila::Config.get(KEY_ADMIN_PASSWORD), {Mycila::Config.get(KEY_WIFI_SSID), Mycila::Config.get(KEY_WIFI_PASSWORD), Mycila::Config.getBool(KEY_AP_MODE_ENABLE)});

  assert(modemTaskManager.asyncStart(BEELANCE_MODEM_TASK_STACK_SIZE, uxTaskPriorityGet(NULL), xPortGetCoreID()));
  assert(hx711TaskManager.asyncStart(BEELANCE_HX711_TASK_STACK_SIZE, uxTaskPriorityGet(NULL), xPortGetCoreID()));

  // STARTUP READY!
  Mycila::Logger.info(TAG, "Started %s %s %s", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());
}

void loop() {
  loopTaskManager.loop();
}
