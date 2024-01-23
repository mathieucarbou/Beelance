// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#ifdef BEELANCE_DISABLE_BROWNOUT_DETECTOR
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#endif

#define TAG "BEELANCE"

Mycila::TaskManager loopTaskManager("loopTask", 16);

AsyncWebServer webServer(80);

ESPDash dashboard = ESPDash(&webServer, "/dashboard", false);

Mycila::TemperatureSensor systemTemperatureSensor;

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

  // start logging
  configureDebugTask.forceRun();
  WebSerial.begin(&webServer, "/console", BEELANCE_ADMIN_USERNAME, Mycila::Config.get(KEY_ADMIN_PASSWORD));
  Mycila::Logger.forwardTo(&WebSerial);

  Mycila::Logger.info(TAG, "Starting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());

  configureSystemTemperatureSensorTask.forceRun();
  configureTaskMonitorTask.forceRun();
  configureNetworkTask.forceRun();

  // STARTUP READY!
  Mycila::Logger.info(TAG, "Started %s %s %s", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());
}

void loop() {
  loopTaskManager.loop();
}
