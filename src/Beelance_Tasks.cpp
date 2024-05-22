// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#define TAG "BEELANCE"

static const Mycila::TaskPredicate DEBUG_ENABLED = []() {
  return logger.isDebugEnabled();
};
static const Mycila::TaskDoneCallback LOG_EXEC_TIME = [](const Mycila::Task& me, const uint32_t elapsed) {
  logger.debug(TAG, "%s in %u us", me.getName(), elapsed);
};

Mycila::Task espConnectTask("ESPConnect.loop()", [](void* params) { ESPConnect.loop(); });
Mycila::Task stackMonitorTask("TaskMonitor.log()", [](void* params) { Mycila::TaskMonitor.log(); });
Mycila::Task websiteTask("Beelance.updateWebsite()", [](void* params) { Beelance::Beelance.updateWebsite(); });
Mycila::Task modemLoopTask("Modem.loop()", [](void* params) { Mycila::Modem.loop(); });

Mycila::Task serialDebugATTask("serialDebugAT", [](void* params) {
  if (Serial.available()) {
    String msg;
    msg.reserve(128);
    while (Serial.available())
      msg += static_cast<char>(Serial.read());
    if (msg.startsWith("AT+")) {
      Mycila::Modem.enqueueAT(msg);
    }
  }
});

Mycila::Task temperatureTask("temperatureSensor.read()", [](void* params) { temperatureSensor.read(); });

Mycila::Task pmuTask("Mycila::PMU.read()", [](void* params) { Mycila::PMU.read(); });

Mycila::Task hx711Task("hx711.read()", [](void* params) { hx711.read(); });

Mycila::Task hx711TareTask("hx711.tare()", [](void* params) {
  hx711.tare();
  config.set(KEY_HX711_OFFSET, String(hx711.getOffset()));
  config.set(KEY_HX711_SCALE, String(hx711.getScale()));
});

Mycila::Task hx711ScaleTask("hx711.calibrate()", [](void* params) {
  config.set(KEY_HX711_SCALE, String(hx711.calibrate(calibrationWeight), 6));
  calibrationWeight = 0;
});

Mycila::Task sendTask("Beelance.sendMeasurements()", [](void* params) {
  if (Mycila::Modem.activateData() && Beelance::Beelance.sendMeasurements()) {
    Mycila::Modem.activateGPS();
  } else {
    logger.error(TAG, "Failed to send measurements. Restarting...");
    restartTask.resume();
  }
});

Mycila::Task restartTask("restartTask", [](void* params) {
  logger.warn(TAG, "Restarting %s...", Mycila::AppInfo.nameModelVersion.c_str());
  Mycila::System.restart(500);
});

Mycila::Task resetTask("resetTask", [](void* params) {
  logger.warn(TAG, "Resetting %s...", Mycila::AppInfo.nameModelVersion.c_str());
  Beelance::Beelance.clearHistory();
  config.clear();
  Mycila::PMU.reset();
  Mycila::System.restart(500);
});

Mycila::Task startModemTask("startModemTask", [](void* params) {
  Mycila::Modem.setPIN(config.get(KEY_MODEM_PIN));
  Mycila::Modem.setAPN(config.get(KEY_MODEM_APN));
  Mycila::Modem.setTimeZoneInfo(config.get(KEY_TIMEZONE_INFO));
  Mycila::Modem.setGpsSyncTimeout(config.get(KEY_MODEM_GPS_SYNC_TIMEOUT).toInt());
  // mode
  String tech = config.get(KEY_MODEM_MODE);
  if (tech == "LTE-M") {
    Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_LTE_M);
  } else if (tech == "NB-IoT") {
    Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_NB_IOT);
  } else {
    Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_AUTO);
  }
  // bands
  Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_LTE_M, config.get(KEY_MODEM_BANDS_LTE_M));
  Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_NB_IOT, config.get(KEY_MODEM_BANDS_NB_IOT));
  // start modem
  if (Mycila::Modem.getState() == Mycila::ModemState::MODEM_OFF) {
    logger.info(TAG, "Enable Modem...");
    Mycila::Modem.begin();
  }
});

Mycila::Task startNetworkServicesTask("startNetworkServicesTask", [](void* params) {
  logger.info(TAG, "Enable Web Server...");
  webServer.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404);
  });
  webServer.begin();
  MDNS.addService("http", "tcp", 80);
});

Mycila::Task stopNetworkServicesTask("stopNetworkServicesTask", [](void* params) {
  logger.info(TAG, "Disable Web Server...");
  webServer.end();
  mdns_service_remove("_http", "_tcp");
});

Mycila::Task otaPrepareTask("otaPrepareTask", [](void* params) {
  logger.info(TAG, "Preparing OTA update...");
  watchdogTask.pause();
});

Mycila::Task watchdogTask("watchdogTask", [](void* params) {
  if (!Mycila::Modem.isReady()) {
    logger.error(TAG, "Watchdog triggered: restarting...");
    restartTask.resume();
  }
});

Mycila::Task configureDebugTask("configureDebugTask", [](void* params) {
  logger.info(TAG, "Configure logging...");
  logger.setLevel(config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO);
  esp_log_level_set("*", static_cast<esp_log_level_t>(logger.getLevel()));
  Mycila::Modem.setDebug(config.getBool(KEY_DEBUG_ENABLE));
});

void Beelance::BeelanceClass::_initTasks() {
  espConnectTask.setType(Mycila::TaskType::FOREVER);
  espConnectTask.setManager(loopTaskManager);

  websiteTask.setType(Mycila::TaskType::FOREVER);
  websiteTask.setManager(loopTaskManager);
  websiteTask.setEnabledWhen([]() { return ESPConnect.isConnected() && !dashboard.isAsyncAccessInProgress(); });
  websiteTask.setInterval(1 * Mycila::TaskDuration::SECONDS);

  stackMonitorTask.setType(Mycila::TaskType::FOREVER);
  stackMonitorTask.setManager(loopTaskManager);
  stackMonitorTask.setEnabledWhen(DEBUG_ENABLED);
  stackMonitorTask.setInterval(10 * Mycila::TaskDuration::SECONDS);

  temperatureTask.setType(Mycila::TaskType::FOREVER);
  temperatureTask.setManager(loopTaskManager);

  restartTask.setType(Mycila::TaskType::ONCE);
  restartTask.setManager(loopTaskManager);

  resetTask.setType(Mycila::TaskType::ONCE);
  resetTask.setManager(loopTaskManager);

  startNetworkServicesTask.setType(Mycila::TaskType::ONCE);
  startNetworkServicesTask.setManager(loopTaskManager);

  stopNetworkServicesTask.setType(Mycila::TaskType::ONCE);
  stopNetworkServicesTask.setManager(loopTaskManager);

  otaPrepareTask.setType(Mycila::TaskType::ONCE);
  otaPrepareTask.setManager(loopTaskManager);

  watchdogTask.setType(Mycila::TaskType::ONCE);
  watchdogTask.setManager(loopTaskManager);

  configureDebugTask.setType(Mycila::TaskType::ONCE);
  configureDebugTask.setManager(loopTaskManager);

  pmuTask.setType(Mycila::TaskType::FOREVER);
  pmuTask.setManager(loopTaskManager);
  pmuTask.setInterval(500 * Mycila::TaskDuration::MILLISECONDS);

  // hx711

  hx711Task.setType(Mycila::TaskType::FOREVER);
  hx711Task.setManager(hx711TaskManager);
  hx711Task.setEnabledWhen([]() { return hx711.isEnabled(); });
  hx711Task.setInterval(500 * Mycila::TaskDuration::MILLISECONDS);

  hx711TareTask.setType(Mycila::TaskType::ONCE);
  hx711TareTask.setManager(hx711TaskManager);

  hx711ScaleTask.setType(Mycila::TaskType::ONCE);
  hx711ScaleTask.setManager(hx711TaskManager);

  // modem

  startModemTask.setType(Mycila::TaskType::ONCE);
  startModemTask.setManager(modemTaskManager);

  serialDebugATTask.setType(Mycila::TaskType::FOREVER);
  serialDebugATTask.setManager(modemTaskManager);

  modemLoopTask.setType(Mycila::TaskType::FOREVER);
  modemLoopTask.setManager(modemTaskManager);

  sendTask.setType(Mycila::TaskType::ONCE);
  sendTask.setManager(modemTaskManager);
  sendTask.setEnabled(false);
  sendTask.setCallback([](const Mycila::Task& me, const uint32_t elapsed) {
    logger.debug(TAG, "%s in %u us", me.getName(), elapsed);
    const uint32_t delay = Beelance::Beelance.getDelayUntilNextSend();
    if (Beelance::Beelance.mustSleep()) {
      logger.info(TAG, "Going to sleep for %u seconds...", delay);
      Beelance::Beelance.sleep(delay); // after sleep, ESP will be restarted
    } else {
      logger.info(TAG, "Sending next measurements in %u seconds...", delay);
      sendTask.resume(delay * Mycila::TaskDuration::SECONDS);
    }
  });
}
