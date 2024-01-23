// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <ESPmDNS.h>

#define TAG "BEELANCE"

static const Mycila::TaskPredicate DEBUG_ENABLED = []() {
  return Mycila::Logger.isDebugEnabled();
};
static const Mycila::TaskDoneCallback LOG_EXEC_TIME = [](const Mycila::Task& me, const uint32_t elapsed) {
  Mycila::Logger.debug(TAG, "%s in %u ms", me.getName(), elapsed / Mycila::TaskDuration::MILLISECONDS);
};

// Forever tasks

Mycila::Task espConnectTask("ESPConnect.loop()", [](void* params) { ESPConnect.loop(); });
Mycila::Task stackMonitorTask("TaskMonitor.log()", [](void* params) { Mycila::TaskMonitor.log(); });
Mycila::Task profilerTask("TaskManager.log()", [](void* params) { loopTaskManager.log(); });
Mycila::Task websiteTask("Beelance.updateWebsite()", [](void* params) { Beelance::Beelance.updateWebsite(); });
Mycila::Task modemTask("Modem.loop()", [](void* params) { Mycila::Modem.loop(); });
Mycila::Task modemSyncTimeTask("Modem.syncTime()", [](void* params) { Mycila::Modem.syncTime(); });
Mycila::Task modemSyncGPSTask("Modem.syncGPS()", [](void* params) { Mycila::Modem.syncGPS(); });
Mycila::Task sendTask("Beelance.sendMeasurements()", [](void* params) { Beelance::Beelance.sendMeasurements(); });

Mycila::Task serialDebugATTask("serialDebugAT", [](void* params) {
  if (Serial.available()) {
    String msg;
    msg.reserve(128);
    while (Serial.available())
      msg += static_cast<char>(Serial.read());
    if (msg.startsWith("AT+")) {
      Mycila::Modem.sendAT(msg.substring(2));
    }
  }
});

Mycila::Task systemTemperatureTask("systemTemperatureSensor.read()", [](void* params) {
  float t = systemTemperatureSensor.read();
  Mycila::Logger.debug(TAG, "Temperature Sensor: %.02f °C", t);
});

// On Demand tasks

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
  // apn
  Mycila::Modem.setAPN(Mycila::Config.get(KEY_MODEM_APN));
  // pin
  Mycila::Modem.setPIN(Mycila::Config.get(KEY_MODEM_PIN));
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
});

// reconfigure tasks

Mycila::Task configureNetworkTask("configureNetworkTask", [](void* params) {
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
});

Mycila::Task configurePMUTask("configurePMUTask", [](void* params) {
  Mycila::Logger.info(TAG, "Configure PMU...");
  Mycila::PMU.begin();
  if (Mycila::PMU.isEnabled()) {
    Mycila::Logger.info(TAG, "PMU is enabled");
    Mycila::Logger.info(TAG, "Powering Modem...");
    Mycila::PMU.enableModem();
    Mycila::Logger.info(TAG, "Powering GPS...");
    Mycila::PMU.enableGPS();
  } else {
    Mycila::Logger.error(TAG, "Failed to enable PMU...");
  }
});

Mycila::Task configureSystemTemperatureSensorTask("configureSystemTemperatureSensorTask", [](void* params) {
  Mycila::Logger.info(TAG, "Configure Temperature Sensor...");
  systemTemperatureSensor.end();
  if (Mycila::Config.getBool(KEY_TEMPERATURE_ENABLE)) {
    systemTemperatureSensor.begin(Mycila::Config.get(KEY_TEMPERATURE_PIN).toInt(), 6 * BEELANCE_TEMPERATURE_READ_INTERVAL);
  } else {
    Mycila::Logger.warn(TAG, "Temperature Sensor not enabled");
  }
});

Mycila::Task configureTaskMonitorTask("configureTaskMonitorTask", [](void* params) {
  Mycila::Logger.info(TAG, "Configure Task Stack Monitor...");
  Mycila::TaskMonitor.begin(5);
  Mycila::TaskMonitor.addTask("async_tcp"); // ESPAsyncTCP
  Mycila::TaskMonitor.addTask("loopTask");  // Arduino
  Mycila::TaskMonitor.addTask("modemTask"); // Modem
});

Mycila::Task configureDebugTask("configureDebugTask", [](void* params) {
  Mycila::Logger.info(TAG, "Configure Debug Level...");
  Mycila::Logger.setLevel(Mycila::Config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO);
  esp_log_level_set("*", static_cast<esp_log_level_t>(Mycila::Logger.getLevel()));
  if (Mycila::Config.getBool(KEY_DEBUG_ENABLE)) {
    Mycila::Modem.setDebug(true);
    // Mycila::Logger.info(TAG, "Enabling profiling for some tasks");
    // websiteTask.enableProfiling(12, Mycila::TaskTimeUnit::MILLISECONDS);
    // sendTask.enableProfiling(12, Mycila::TaskTimeUnit::MILLISECONDS);
    // modemTask.enableProfiling(12, Mycila::TaskTimeUnit::MILLISECONDS);
  } else {
    Mycila::Modem.setDebug(false);
    Mycila::Logger.info(TAG, "Disable profiling for all tasks");
    loopTaskManager.disableProfiling();
  }
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

  profilerTask.setType(Mycila::TaskType::FOREVER);
  profilerTask.setManager(&loopTaskManager);
  profilerTask.setEnabledWhen(DEBUG_ENABLED);
  profilerTask.setInterval(10 * Mycila::TaskDuration::SECONDS);

  systemTemperatureTask.setType(Mycila::TaskType::FOREVER);
  systemTemperatureTask.setManager(&loopTaskManager);
  systemTemperatureTask.setEnabledWhen([]() { return systemTemperatureSensor.isEnabled(); });
  systemTemperatureTask.setInterval(BEELANCE_TEMPERATURE_READ_INTERVAL * Mycila::TaskDuration::SECONDS);

  sendTask.setType(Mycila::TaskType::FOREVER);
  sendTask.setManager(&modemTaskManager);
  sendTask.setInterval(Mycila::Config.get(KEY_SEND_INTERVAL).toInt() * Mycila::TaskDuration::SECONDS);
  sendTask.setCallback(LOG_EXEC_TIME);
  sendTask.setCallback([](const Mycila::Task& me, const uint32_t elapsed) {
    if (Mycila::Config.getBool(KEY_NO_SLEEP_ENABLE)) {
      // TODO: sleep after sending
    }
  });

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

  configureDebugTask.setType(Mycila::TaskType::ONCE);
  configureDebugTask.setManager(&loopTaskManager);

  configureNetworkTask.setType(Mycila::TaskType::ONCE);
  configureNetworkTask.setManager(&loopTaskManager);

  configurePMUTask.setType(Mycila::TaskType::ONCE);
  configurePMUTask.setManager(&loopTaskManager);

  configureSystemTemperatureSensorTask.setType(Mycila::TaskType::ONCE);
  configureSystemTemperatureSensorTask.setManager(&loopTaskManager);

  configureTaskMonitorTask.setType(Mycila::TaskType::ONCE);
  configureTaskMonitorTask.setManager(&loopTaskManager);

  // modem

  startModemTask.setType(Mycila::TaskType::ONCE);
  startModemTask.setManager(&modemTaskManager);

  serialDebugATTask.setType(Mycila::TaskType::FOREVER);
  serialDebugATTask.setManager(&modemTaskManager);

  modemTask.setType(Mycila::TaskType::FOREVER);
  modemTask.setManager(&modemTaskManager);

  modemSyncTimeTask.setType(Mycila::TaskType::FOREVER);
  modemSyncTimeTask.setManager(&modemTaskManager);
  modemSyncTimeTask.setInterval(5 * Mycila::TaskDuration::SECONDS);
  modemSyncTimeTask.setEnabledWhen([]() { return !Mycila::Modem.isTimeSynced(); });

  modemSyncGPSTask.setType(Mycila::TaskType::FOREVER);
  modemSyncGPSTask.setManager(&modemTaskManager);
  modemSyncGPSTask.setInterval(10 * Mycila::TaskDuration::SECONDS);

#ifdef MYCILA_TASK_MANAGER_DEBUG
  configureDebugTask.setDebugWhen(DEBUG_ENABLED);
  configureNetworkTask.setDebugWhen(DEBUG_ENABLED);
  configurePMUTask.setDebugWhen(DEBUG_ENABLED);
  configureSystemTemperatureSensorTask.setDebugWhen(DEBUG_ENABLED);
  configureTaskMonitorTask.setDebugWhen(DEBUG_ENABLED);
  otaPrepareTask.setDebugWhen(DEBUG_ENABLED);
  // profilerTask.setDebugWhen(DEBUG_ENABLED);
  resetTask.setDebugWhen(DEBUG_ENABLED);
  restartTask.setDebugWhen(DEBUG_ENABLED);
  sendTask.setDebugWhen(DEBUG_ENABLED);
  // stackMonitorTask.setDebugWhen(DEBUG_ENABLED);
  startNetworkServicesTask.setDebugWhen(DEBUG_ENABLED);
  stopNetworkServicesTask.setDebugWhen(DEBUG_ENABLED);
  systemTemperatureTask.setDebugWhen(DEBUG_ENABLED);
  // websiteTask.setDebugWhen(DEBUG_ENABLED);
  // modem
  startModemTask.setDebugWhen(DEBUG_ENABLED);
#endif
}
