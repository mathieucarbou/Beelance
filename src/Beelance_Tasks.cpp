// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#define TAG "BEELANCE"

static const Mycila::TaskPredicate DEBUG_ENABLED = []() {
  return Mycila::Logger.isDebugEnabled();
};
static const Mycila::TaskDoneCallback LOG_EXEC_TIME = [](const Mycila::Task& me, const uint32_t elapsed) {
  Mycila::Logger.debug(TAG, "%s in %u us", me.getName(), elapsed);
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

Mycila::Task hx711Task("hx711.read()", [](void* params) { hx711.read(); });

Mycila::Task hx711TareTask("hx711.tare()", [](void* params) {
  hx711.tare();
  Mycila::Config.set(KEY_HX711_OFFSET, String(hx711.getOffset()));
  Mycila::Config.set(KEY_HX711_SCALE, String(hx711.getScale()));
});

Mycila::Task hx711ScaleTask("hx711.calibrate()", [](void* params) {
  Mycila::Config.set(KEY_HX711_SCALE, String(hx711.calibrate(calibrationWeight), 6));
  calibrationWeight = 0;
});

Mycila::Task sendTask("Beelance.sendMeasurements()", [](void* params) {
  if (Mycila::Modem.activateData() && Beelance::Beelance.sendMeasurements()) {
    Mycila::Modem.activateGPS();
  } else {
    Mycila::Logger.error(TAG, "Failed to send measurements. Restarting...");
    restartTask.resume();
  }
});

Mycila::Task restartTask("restartTask", [](void* params) {
  Mycila::Logger.warn(TAG, "Restarting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());
  Mycila::System.restart(500);
});

Mycila::Task resetTask("resetTask", [](void* params) {
  Mycila::Logger.warn(TAG, "Resetting %s %s %s...", Mycila::AppInfo.name.c_str(), Mycila::AppInfo.model.c_str(), Mycila::AppInfo.version.c_str());
  Mycila::Config.clear();
  Mycila::System.restart(500);
});

Mycila::Task startModemTask("startModemTask", [](void* params) {
  Mycila::Modem.setPIN(Mycila::Config.get(KEY_MODEM_PIN));
  Mycila::Modem.setAPN(Mycila::Config.get(KEY_MODEM_APN));
  Mycila::Modem.setTimeZoneInfo(Mycila::Config.get(KEY_TIMEZONE_INFO));
  Mycila::Modem.setGpsSyncTimeout(Mycila::Config.get(KEY_MODEM_GPS_SYNC_TIMEOUT).toInt());
  // mode
  String tech = Mycila::Config.get(KEY_MODEM_MODE);
  if (tech == "LTE-M") {
    Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_LTE_M);
  } else if (tech == "NB-IoT") {
    Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_NB_IOT);
  } else {
    Mycila::Modem.setPreferredMode(Mycila::ModemMode::MODEM_MODE_AUTO);
  }
  // bands
  Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_LTE_M, Mycila::Config.get(KEY_MODEM_BANDS_LTE_M));
  Mycila::Modem.setBands(Mycila::ModemMode::MODEM_MODE_NB_IOT, Mycila::Config.get(KEY_MODEM_BANDS_NB_IOT));
  // start modem
  if (Mycila::Modem.getState() == Mycila::ModemState::MODEM_OFF) {
    Mycila::Logger.info(TAG, "Enable Modem...");
    Mycila::Modem.begin();
  }
});

Mycila::Task startNetworkServicesTask("startNetworkServicesTask", [](void* params) {
  Mycila::Logger.info(TAG, "Enable Web Server...");
  webServer.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404);
  });
  webServer.begin();
  MDNS.addService("http", "tcp", 80);
});

Mycila::Task stopNetworkServicesTask("stopNetworkServicesTask", [](void* params) {
  Mycila::Logger.info(TAG, "Disable Web Server...");
  webServer.end();
  mdns_service_remove("_http", "_tcp");
});

Mycila::Task otaPrepareTask("otaPrepareTask", [](void* params) {
  Mycila::Logger.info(TAG, "Preparing OTA update...");
  watchdogTask.pause();
});

Mycila::Task watchdogTask("watchdogTask", [](void* params) {
  if (!Mycila::Modem.isReady()) {
    Mycila::Logger.error(TAG, "Watchdog triggered: restarting...");
    restartTask.resume();
  }
});

Mycila::Task configureDebugTask("configureDebugTask", [](void* params) {
  Mycila::Logger.info(TAG, "Configure Debug Level...");
  Mycila::Logger.setLevel(Mycila::Config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO);
  esp_log_level_set("*", static_cast<esp_log_level_t>(Mycila::Logger.getLevel()));
  Mycila::Modem.setDebug(Mycila::Config.getBool(KEY_DEBUG_ENABLE));
});

void Beelance::BeelanceClass::_initTasks() {
  espConnectTask.setType(Mycila::TaskType::FOREVER);
  espConnectTask.setManager(&loopTaskManager);

  websiteTask.setType(Mycila::TaskType::FOREVER);
  websiteTask.setManager(&loopTaskManager);
  websiteTask.setEnabledWhen([]() { return ESPConnect.isConnected() && !dashboard.isAsyncAccessInProgress(); });
  websiteTask.setInterval(1 * Mycila::TaskDuration::SECONDS);

  stackMonitorTask.setType(Mycila::TaskType::FOREVER);
  stackMonitorTask.setManager(&loopTaskManager);
  stackMonitorTask.setEnabledWhen(DEBUG_ENABLED);
  stackMonitorTask.setInterval(10 * Mycila::TaskDuration::SECONDS);

  temperatureTask.setType(Mycila::TaskType::FOREVER);
  temperatureTask.setManager(&loopTaskManager);
  temperatureTask.setEnabledWhen([]() { return temperatureSensor.isEnabled(); });
  temperatureTask.setInterval(5 * Mycila::TaskDuration::SECONDS);

  restartTask.setType(Mycila::TaskType::ONCE);
  restartTask.setManager(&loopTaskManager);

  resetTask.setType(Mycila::TaskType::ONCE);
  resetTask.setManager(&loopTaskManager);

  startNetworkServicesTask.setType(Mycila::TaskType::ONCE);
  startNetworkServicesTask.setManager(&loopTaskManager);

  stopNetworkServicesTask.setType(Mycila::TaskType::ONCE);
  stopNetworkServicesTask.setManager(&loopTaskManager);

  otaPrepareTask.setType(Mycila::TaskType::ONCE);
  otaPrepareTask.setManager(&loopTaskManager);

  watchdogTask.setType(Mycila::TaskType::ONCE);
  watchdogTask.setManager(&loopTaskManager);

  configureDebugTask.setType(Mycila::TaskType::ONCE);
  configureDebugTask.setManager(&loopTaskManager);

  // hx711

  hx711Task.setType(Mycila::TaskType::FOREVER);
  hx711Task.setManager(&hx711TaskManager);
  hx711Task.setEnabledWhen([]() { return hx711.isEnabled(); });
  hx711Task.setInterval(500 * Mycila::TaskDuration::MILLISECONDS);

  hx711TareTask.setType(Mycila::TaskType::ONCE);
  hx711TareTask.setManager(&hx711TaskManager);

  hx711ScaleTask.setType(Mycila::TaskType::ONCE);
  hx711ScaleTask.setManager(&hx711TaskManager);

  // modem

  startModemTask.setType(Mycila::TaskType::ONCE);
  startModemTask.setManager(&modemTaskManager);

  serialDebugATTask.setType(Mycila::TaskType::FOREVER);
  serialDebugATTask.setManager(&modemTaskManager);

  modemLoopTask.setType(Mycila::TaskType::FOREVER);
  modemLoopTask.setManager(&modemTaskManager);

  sendTask.setType(Mycila::TaskType::ONCE);
  sendTask.setManager(&modemTaskManager);
  sendTask.setEnabled(false);
  sendTask.setCallback([](const Mycila::Task& me, const uint32_t elapsed) {
    Mycila::Logger.debug(TAG, "%s in %u us", me.getName(), elapsed);
    const uint32_t delay = Beelance::Beelance.getDelayUntilNextSend();
    if (Mycila::Config.getBool(KEY_NO_SLEEP_ENABLE)) {
      Mycila::Logger.info(TAG, "Sending next measurements in %u seconds...", delay);
      sendTask.setInterval(delay * Mycila::TaskDuration::SECONDS);
      sendTask.resume();
    } else {
      Mycila::Logger.info(TAG, "Going to sleep for %u seconds...", delay);
      Beelance::Beelance.sleep(delay); // after sleep, ESP will be restarted
    }
  });

#ifdef MYCILA_TASK_MANAGER_DEBUG
  configureDebugTask.setDebugWhen(DEBUG_ENABLED);
  hx711TareTask.setDebugWhen(DEBUG_ENABLED);
  hx711ScaleTask.setDebugWhen(DEBUG_ENABLED);
  // hx711Task.setDebugWhen(DEBUG_ENABLED);
  otaPrepareTask.setDebugWhen(DEBUG_ENABLED);
  resetTask.setDebugWhen(DEBUG_ENABLED);
  restartTask.setDebugWhen(DEBUG_ENABLED);
  sendTask.setDebugWhen(DEBUG_ENABLED);
  startModemTask.setDebugWhen(DEBUG_ENABLED);
  startNetworkServicesTask.setDebugWhen(DEBUG_ENABLED);
  stopNetworkServicesTask.setDebugWhen(DEBUG_ENABLED);
  temperatureTask.setDebugWhen(DEBUG_ENABLED);
#endif
}
