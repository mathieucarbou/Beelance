// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <Beelance.h>
#include <BeelanceWebsite.h>

#define TAG "BEELANCE"

AsyncWebServer webServer(80);
Mycila::ESPConnect espConnect(webServer);
ESPDash dashboard = ESPDash(&webServer, "/dashboard", false);

Mycila::Logger logger;
Mycila::Config config;

Mycila::TaskManager hx711TaskManager("hx711");
Mycila::TaskManager loopTaskManager("loopTask");
Mycila::TaskManager modemTaskManager("modemTask");

Mycila::HX711 hx711;
Mycila::DS18 temperatureSensor;

float calibrationWeight = 0;

// setup
void setup() {
  Serial.begin(BEELANCE_SERIAL_BAUDRATE);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.setTxTimeoutMs(0);
  delay(100);
#else
  while (!Serial)
    yield();
#endif

  // logger
  logger.forwardTo(&Serial);
  logger.info(TAG, "Booting %s...", Mycila::AppInfo.nameModelVersion.c_str());

  // system
  Mycila::System::init();

  // load config and initialize
  Beelance::Beelance.begin();

  logger.info(TAG, "Starting %s...", Mycila::AppInfo.nameModelVersion.c_str());

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
  Mycila::TaskMonitor.begin(5);
  Mycila::TaskMonitor.addTask("async_tcp"); // ESPAsyncTCP
  Mycila::TaskMonitor.addTask("loopTask");  // Arduino
  Mycila::TaskMonitor.addTask("modemTask"); // Modem
  Mycila::TaskMonitor.addTask("hx711");     // HX711

  // network
  webServer.end();
  espConnect.end();
  espConnect.setAutoRestart(true);
  espConnect.setBlocking(false);
  espConnect.begin(Mycila::AppInfo.defaultHostname, Mycila::AppInfo.defaultSSID, config.get(KEY_ADMIN_PASSWORD), {config.get(KEY_WIFI_SSID), config.get(KEY_WIFI_PASSWORD), config.getBool(KEY_AP_MODE_ENABLE)});

  assert(modemTaskManager.asyncStart(BEELANCE_MODEM_TASK_STACK_SIZE, uxTaskPriorityGet(NULL), xPortGetCoreID()));
  assert(hx711TaskManager.asyncStart(BEELANCE_HX711_TASK_STACK_SIZE, uxTaskPriorityGet(NULL), xPortGetCoreID()));

  // STARTUP READY!
  logger.info(TAG, "Started %s", Mycila::AppInfo.nameModelVersion.c_str());
}

void loop() {
  loopTaskManager.loop();
}
