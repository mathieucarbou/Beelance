// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <Beelance.h>
#include <BeelanceWebsite.h>

#define TAG "BEELANCE"

AsyncWebServer webServer(80);
AsyncAuthenticationMiddleware authMiddleware;
Mycila::ESPConnect espConnect(webServer);
ESPDash dashboard(webServer, "/dashboard", false);
WebSerial webSerial;

Mycila::Logger logger;

Mycila::ConfigStorageNVS storage;
Mycila::Config config(storage);

Mycila::TaskManager hx711TaskManager("hx711");
Mycila::TaskManager loopTaskManager("beelance");
Mycila::TaskManager modemTaskManager("modem");

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
  Mycila::PMU.setChargingCurrent(config.getLong(KEY_PMU_CHARGING_CURRENT));
  logger.info(TAG, "Powering Modem...");
  Mycila::PMU.enableModem();
  logger.info(TAG, "Powering GPS...");
  Mycila::PMU.enableGPS();

  // Temperature
  temperatureSensor.begin(static_cast<int8_t>(config.getLong(KEY_TEMPERATURE_PIN)));
  if (!temperatureSensor.isEnabled()) {
    Beelance::Website.disableTemperature();
  }

  // HX711
  logger.info(TAG, "Configure HX711...");
  hx711.setOffset(config.getLong(KEY_HX711_OFFSET));
  hx711.setScale(config.getFloat(KEY_HX711_SCALE));
  hx711.setExpirationDelay(10);
  hx711.begin(config.getLong(KEY_HX711_DATA_PIN), config.getLong(KEY_HX711_CLOCK_PIN));

  // stack monitor
  Mycila::TaskMonitor.addTask("async_tcp"); // ESPAsyncTCP
  Mycila::TaskMonitor.addTask("beelance");  // Beelance
  Mycila::TaskMonitor.addTask("modem");     // Modem
  Mycila::TaskMonitor.addTask("hx711");     // HX711

  // network
  webServer.end();
  espConnect.end();
  espConnect.setAutoRestart(true);
  espConnect.setBlocking(false);
  Mycila::ESPConnect::Config espConnectConfig;
  espConnectConfig.hostname = Mycila::AppInfo.defaultHostname;
  espConnectConfig.apMode = config.getBool(KEY_AP_MODE_ENABLE);
  espConnectConfig.wifiSSID = config.getString(KEY_WIFI_SSID);
  espConnectConfig.wifiPassword = config.getString(KEY_WIFI_PASSWORD);
  espConnect.begin(Mycila::AppInfo.defaultHostname.c_str(), config.getString(KEY_ADMIN_PASSWORD), espConnectConfig);

  assert(loopTaskManager.asyncStart(512 * 19, uxTaskPriorityGet(NULL), xPortGetCoreID()));
  assert(modemTaskManager.asyncStart(512 * 11, uxTaskPriorityGet(NULL), xPortGetCoreID()));
  assert(hx711TaskManager.asyncStart(512 * 6, uxTaskPriorityGet(NULL), xPortGetCoreID()));

  // STARTUP READY!
  logger.info(TAG, "Started %s", Mycila::AppInfo.nameModelVersion.c_str());
}

void loop() {
  vTaskDelete(NULL);
}
