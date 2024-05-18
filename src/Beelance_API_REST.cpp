// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <AsyncJson.h>
#include <LittleFS.h>
#include <map>

#define TAG "BEELANCE"

void Beelance::BeelanceClass::_initREST() {
  // app

  webServer
    .on("/api/app", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      Mycila::AppInfo.toJson(response->getRoot());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  // config

  webServer
    .on("/api/config/backup", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", config.backup());
      response->addHeader("Content-Disposition", "attachment; filename=\"config.txt\"");
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on(
      "/api/config/restore",
      HTTP_POST,
      [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
        if (!LittleFS.exists("/config.txt")) {
          return request->send(400, "text/plain", "No config.txt file uploaded");
        }
        File cfg = LittleFS.open("/config.txt", "r");
        const String data = cfg.readString();
        cfg.close();
        config.restore(data);
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
      },
      [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
        if (!index)
          request->_tempFile = LittleFS.open("/config.txt", "w");
        if (len)
          request->_tempFile.write(data, len);
        if (final)
          request->_tempFile.close();
      })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {
      std::map<const char*, String> settings;
      for (size_t i = 0, max = request->params(); i < max; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost() && !p->isFile()) {
          const char* keyRef = config.keyRef(p->name().c_str());
          settings[keyRef] = p->value();
        }
      }
      request->send(200);
      config.set(settings);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse(false);
      config.toJson(response->getRoot());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  // network

  webServer
    .on("/api/network", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      ESPConnect.toJson(response->getRoot());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  // system

  webServer
    .on("/api/system/restart", HTTP_ANY, [=](AsyncWebServerRequest* request) {
      restartTask.resume();
      request->send(200);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/system/reset", HTTP_ANY, [=](AsyncWebServerRequest* request) {
      resetTask.resume();
      request->send(200);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      JsonObject root = response->getRoot();
      Mycila::System.toJson(root);
      hx711.toJson(root["hx711"].to<JsonObject>());
      Mycila::PMU.toJson(root["pmu"].to<JsonObject>());
      Mycila::TaskMonitor.toJson(root["stack"].to<JsonObject>());
      loopTaskManager.toJson(root["task_managers"][0].to<JsonObject>());
      temperatureSensor.toJson(root["temp_sensor"].to<JsonObject>());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  // beelance

  webServer
    .on("/api/beelance/history.json", HTTP_GET, [this](AsyncWebServerRequest* request) {
      if (LittleFS.exists(FILE_HISTORY)) {
        AsyncWebServerResponse* response = request->beginResponse(LittleFS, FILE_HISTORY, "application/json");
        request->send(response);
      } else {
        request->send(404);
      }
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/beelance/history/reset", HTTP_ANY, [this](AsyncWebServerRequest* request) {
      Beelance::Beelance.clearHistory();
      request->send(200);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/beelance/history", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      Beelance::Beelance.historyToJson(response->getRoot());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  webServer
    .on("/api/beelance", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      JsonObject root = response->getRoot();
      Beelance::Beelance.toJson(root);
      Beelance::Beelance.historyToJson(root["history"].to<JsonObject>());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));

  // root

  webServer
    .on("/api/", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      JsonObject root = response->getRoot();
      // app
      Mycila::AppInfo.toJson(root["app"].to<JsonObject>());
      // beelance
      Beelance::Beelance.toJson(root["beelance"].to<JsonObject>());
      Beelance::Beelance.historyToJson(root["beelance"]["history"].to<JsonObject>());
      // config
      config.toJson(root["config"].to<JsonObject>());
      // network
      ESPConnect.toJson(root["network"].to<JsonObject>());
      // system
      Mycila::System.toJson(root["system"].to<JsonObject>());
      hx711.toJson(root["system"]["hx711"].to<JsonObject>());
      Mycila::PMU.toJson(root["system"]["pmu"].to<JsonObject>());
      Mycila::TaskMonitor.toJson(root["system"]["stack"].to<JsonObject>());
      loopTaskManager.toJson(root["system"]["task_managers"][0].to<JsonObject>());
      temperatureSensor.toJson(root["system"]["temp_sensor"].to<JsonObject>());
      response->setLength();
      request->send(response);
    })
    .setAuthentication(BEELANCE_ADMIN_USERNAME, config.get(KEY_ADMIN_PASSWORD));
}
