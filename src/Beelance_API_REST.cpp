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

  Mycila::HTTPd.apiGET("/app", [](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    Mycila::AppInfo.toJson(response->getRoot());
    response->setLength();
    request->send(response);
  });

  // config

  Mycila::HTTPd.apiGET("/config/backup", [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", Mycila::Config.backup());
    response->addHeader("Content-Disposition", "attachment; filename=\"config.txt\"");
    request->send(response);
  });

  Mycila::HTTPd.apiUPLOAD(
    "/config/restore",
    [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
      if (!LittleFS.exists("/config.txt")) {
        return request->send(400, "text/plain", "No config.txt file uploaded");
      }
      File cfg = LittleFS.open("/config.txt", "r");
      const String data = cfg.readString();
      cfg.close();
      Mycila::Config.restore(data);
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
    });

  Mycila::HTTPd.apiPOST("/config", [](AsyncWebServerRequest* request) {
    std::map<const char*, String> settings;
    for (size_t i = 0, max = request->params(); i < max; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost() && !p->isFile()) {
        const char* keyRef = Mycila::Config.keyRef(p->name().c_str());
        settings[keyRef] = p->value();
      }
    }
    request->send(200);
    Mycila::Config.set(settings);
  });

  Mycila::HTTPd.apiGET("/config", [](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse(false);
    Mycila::Config.toJson(response->getRoot());
    response->setLength();
    request->send(response);
  });

  // network

  Mycila::HTTPd.apiGET("/network", [](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    ESPConnect.toJson(response->getRoot());
    response->setLength();
    request->send(response);
  });

  // system

  Mycila::HTTPd.apiANY("/system/restart", [=](AsyncWebServerRequest* request) {
    restartTask.resume();
    request->send(200);
  });

  Mycila::HTTPd.apiANY("/system/reset", [=](AsyncWebServerRequest* request) {
    resetTask.resume();
    request->send(200);
  });

  Mycila::HTTPd.apiGET("/system", [this](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    Mycila::System.toJson(root);
    hx711.toJson(root["hx711"].to<JsonObject>());
    Mycila::TaskMonitor.toJson(root["stack"].to<JsonObject>());
    loopTaskManager.toJson(root["task_managers"][0].to<JsonObject>());
    temperatureSensor.toJson(root["temp_sensor"].to<JsonObject>());
    response->setLength();
    request->send(response);
  });

  // beelance

  Mycila::HTTPd.apiGET("/beelance/history.json", [this](AsyncWebServerRequest* request) {
    if (LittleFS.exists(FILE_HISTORY)) {
      AsyncWebServerResponse* response = request->beginResponse(LittleFS, FILE_HISTORY, "application/json");
      request->send(response);
    } else {
      request->send(404);
    }
  });

  Mycila::HTTPd.apiANY("/beelance/history/reset", [this](AsyncWebServerRequest* request) {
    Beelance::Beelance.clearHistory();
    request->send(200);
  });

  Mycila::HTTPd.apiGET("/beelance/history", [this](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    Beelance::Beelance.historyToJson(response->getRoot());
    response->setLength();
    request->send(response);
  });

  Mycila::HTTPd.apiGET("/beelance", [this](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    Beelance::Beelance.toJson(root);
    Beelance::Beelance.historyToJson(root["history"].to<JsonObject>());
    response->setLength();
    request->send(response);
  });

  // root

  Mycila::HTTPd.apiGET("/", [this](AsyncWebServerRequest* request) {
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    // app
    Mycila::AppInfo.toJson(root["app"].to<JsonObject>());
    // beelance
    Beelance::Beelance.toJson(root["beelance"].to<JsonObject>());
    Beelance::Beelance.historyToJson(root["beelance"]["history"].to<JsonObject>());
    // config
    Mycila::Config.toJson(root["config"].to<JsonObject>());
    // network
    ESPConnect.toJson(root["network"].to<JsonObject>());
    // system
    Mycila::System.toJson(root["system"].to<JsonObject>());
    hx711.toJson(root["system"]["hx711"].to<JsonObject>());
    Mycila::TaskMonitor.toJson(root["system"]["stack"].to<JsonObject>());
    loopTaskManager.toJson(root["system"]["task_managers"][0].to<JsonObject>());
    temperatureSensor.toJson(root["system"]["temp_sensor"].to<JsonObject>());
    response->setLength();
    request->send(response);
  });
}
