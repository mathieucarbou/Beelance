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

// Core tasks

Mycila::Task espConnectTask("ESPConnect.loop()", [](void* params) { ESPConnect.loop(); });
Mycila::Task stackMonitorTask("TaskMonitor.log()", [](void* params) { Mycila::TaskMonitor.log(); });
Mycila::Task profilerTask("TaskManager.log()", [](void* params) { loopTaskManager.log(); });
Mycila::Task websiteTask("Beelance.updateWebsite()", [](void* params) { Beelance::Beelance.updateWebsite(); });
Mycila::Task forwardSerialATTask("forwardSerialAT", [](void* params) { while (Serial.available()) serialAT.write(Serial.read()); });

// Beelance tasks

Mycila::Task systemTemperatureTask("systemTemperatureSensor.read()", [](void* params) {
  float t = systemTemperatureSensor.read();
  Mycila::Logger.debug(TAG, "Temperature Sensor: %.02f °C", t);
});

Mycila::Task sendTask("Beelance.sendMeasurements()", [](void* params) { Beelance::Beelance.sendMeasurements(); });

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

Mycila::Task startNetworkServicesTask("startNetworkServicesTask", [](void* params) {
  Mycila::Logger.info(TAG, "Starting network services...");

  Mycila::Logger.info(TAG, "Enable Web Server...");
  webServer.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404);
  });
  webServer.begin();
  MDNS.addService("http", "tcp", 80);
});

Mycila::Task stopNetworkServicesTask("stopNetworkServicesTask", [](void* params) {
  Mycila::Logger.info(TAG, "Stopping network services...");
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
});

Mycila::Task configureDebugTask("configureDebugTask", [](void* params) {
  Mycila::Logger.info(TAG, "Configure Debug Level...");
  Mycila::Logger.setLevel(Mycila::Config.getBool(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO);
  esp_log_level_set("*", static_cast<esp_log_level_t>(Mycila::Logger.getLevel()));
  if (Mycila::Config.getBool(KEY_DEBUG_ENABLE)) {
    Mycila::Logger.info(TAG, "Enabling profiling for some tasks");
    websiteTask.enableProfiling(12, Mycila::TaskTimeUnit::MILLISECONDS);
    sendTask.enableProfiling(12, Mycila::TaskTimeUnit::MILLISECONDS);
  } else {
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

  forwardSerialATTask.setType(Mycila::TaskType::FOREVER);
  forwardSerialATTask.setManager(&loopTaskManager);

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
  sendTask.setManager(&loopTaskManager);
  // TODO: check connectivity sendTask.setEnabledWhen([]() { return ESPConnect.isConnected() && !dashboard.isAsyncAccessInProgress(); });
  sendTask.setInterval(Mycila::Config.get(KEY_SEND_INTERVAL).toInt() * Mycila::TaskDuration::SECONDS);
  sendTask.setCallback(LOG_EXEC_TIME);

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

  configureSystemTemperatureSensorTask.setType(Mycila::TaskType::ONCE);
  configureSystemTemperatureSensorTask.setManager(&loopTaskManager);

  configureTaskMonitorTask.setType(Mycila::TaskType::ONCE);
  configureTaskMonitorTask.setManager(&loopTaskManager);

#ifdef MYCILA_TASK_MANAGER_DEBUG
  configureDebugTask.setDebugWhen(DEBUG_ENABLED);
  configureNetworkTask.setDebugWhen(DEBUG_ENABLED);
  configureSystemTemperatureSensorTask.setDebugWhen(DEBUG_ENABLED);
  configureTaskMonitorTask.setDebugWhen(DEBUG_ENABLED);
  otaPrepareTask.setDebugWhen(DEBUG_ENABLED);
  profilerTask.setDebugWhen(DEBUG_ENABLED);
  resetTask.setDebugWhen(DEBUG_ENABLED);
  restartTask.setDebugWhen(DEBUG_ENABLED);
  sendTask.setDebugWhen(DEBUG_ENABLED);
  stackMonitorTask.setDebugWhen(DEBUG_ENABLED);
  startNetworkServicesTask.setDebugWhen(DEBUG_ENABLED);
  stopNetworkServicesTask.setDebugWhen(DEBUG_ENABLED);
  systemTemperatureTask.setDebugWhen(DEBUG_ENABLED);
  websiteTask.setDebugWhen(DEBUG_ENABLED);
#endif
}
