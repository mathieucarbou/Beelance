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

Mycila::Logger logger;
Mycila::Config config;

Mycila::TaskManager hx711TaskManager("hx711", 3);
Mycila::TaskManager loopTaskManager("loopTask", 12);
Mycila::TaskManager modemTaskManager("modemTask", 5);

Mycila::HX711 hx711;
Mycila::DS18 temperatureSensor;

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
  logger.getOutputs().reserve(2);
  logger.forwardTo(&Serial);
  logger.info(TAG, "Booting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());

  // system
  Mycila::System.begin();

  // load config and initialize
  Beelance::Beelance.begin();

  logger.info(TAG, "Starting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());

  // PMU
  logger.info(TAG, "Configure PMU...");
  Mycila::PMU.begin();
  Mycila::PMU.enableDCPins();
  Mycila::PMU.setChargingLedMode(XPOWERS_CHG_LED_ON);
  Mycila::PMU.setChargingCurrent(config.get(KEY_PMU_CHARGING_CURRENT).toInt());
  logger.info(TAG, "Powering Modem...");
  Mycila::PMU.enableModem();
  logger.info(TAG, "Powering GPS...");
  Mycila::PMU.enableGPS();

  // Temperature
  logger.info(TAG, "Configure Temperature Sensor...");
  temperatureSensor.begin(config.get(KEY_TEMPERATURE_PIN).toInt());
  if (!temperatureSensor.isEnabled()) {
    Beelance::Website.disableTemperature();
  }

  // HX711
  logger.info(TAG, "Configure HX711...");
  hx711.setOffset(config.get(KEY_HX711_OFFSET).toInt());
  hx711.setScale(config.get(KEY_HX711_SCALE).toFloat());
  hx711.setExpirationDelay(10);
  hx711.begin(config.get(KEY_HX711_DATA_PIN).toInt(), config.get(KEY_HX711_CLOCK_PIN).toInt());

  // stack monitor
  logger.info(TAG, "Configure Task Stack Monitor...");
  Mycila::TaskMonitor.begin(5);
  Mycila::TaskMonitor.addTask("async_tcp"); // ESPAsyncTCP
  Mycila::TaskMonitor.addTask("loopTask");  // Arduino
  Mycila::TaskMonitor.addTask("modemTask"); // Modem
  Mycila::TaskMonitor.addTask("hx711");     // HX711

  // network
  logger.info(TAG, "Configure Network...");
  webServer.end();
  mdns_service_remove("_http", "_tcp");
  ESPConnect.end();
  logger.info(TAG, "Hostname: %s", config.get(KEY_HOSTNAME).c_str());
  ESPConnect.setAutoRestart(false);
  ESPConnect.setBlocking(false);
  ESPConnect.setCaptivePortalTimeout(config.get(KEY_CAPTURE_PORTAL_TIMEOUT).toInt());
  ESPConnect.setConnectTimeout(config.get(KEY_WIFI_CONNECTION_TIMEOUT).toInt());
  ESPConnect.begin(webServer, config.get(KEY_HOSTNAME), Mycila::AppInfo.name + "-" + Mycila::AppInfo.id, config.get(KEY_ADMIN_PASSWORD), {config.get(KEY_WIFI_SSID), config.get(KEY_WIFI_PASSWORD), config.getBool(KEY_AP_MODE_ENABLE)});

  assert(modemTaskManager.asyncStart(BEELANCE_MODEM_TASK_STACK_SIZE, uxTaskPriorityGet(NULL), xPortGetCoreID()));
  assert(hx711TaskManager.asyncStart(BEELANCE_HX711_TASK_STACK_SIZE, uxTaskPriorityGet(NULL), xPortGetCoreID()));

  // STARTUP READY!
  logger.info(TAG, "Started %s %s %s", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());
}

void loop() {
  loopTaskManager.loop();
}
