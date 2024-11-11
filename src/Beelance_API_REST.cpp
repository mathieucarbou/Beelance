// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <Beelance.h>

#include <AsyncJson.h>
#include <LittleFS.h>
#include <StreamString.h>

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
    });

  // config

  webServer
    .on("/api/config/backup", HTTP_GET, [](AsyncWebServerRequest* request) {
      StreamString body;
      body.reserve(4096);
      config.backup(body);
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", body);
      response->addHeader("Content-Disposition", "attachment; filename=\"config.txt\"");
      request->send(response);
    });

  webServer
    .on(
      "/api/config/restore",
      HTTP_POST,
      [](AsyncWebServerRequest* request) {
        if (!request->_tempObject) {
          return request->send(400, "text/plain", "No config file uploaded");
        }
        StreamString* buffer = reinterpret_cast<StreamString*>(request->_tempObject);
        config.restore((*buffer).c_str());
        delete buffer;
        request->_tempObject = nullptr;
        request->send(200, "text/plain", "OK");
      },
      [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
        if (!index) {
          if (request->_tempObject) {
            delete reinterpret_cast<StreamString*>(request->_tempObject);
          }
          StreamString* buffer = new StreamString();
          buffer->reserve(4096);
          request->_tempObject = buffer;
        }
        if (len) {
          reinterpret_cast<StreamString*>(request->_tempObject)->write(data, len);
        }
      });

  webServer
    .on("/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {
      std::map<const char*, String> settings;
      for (size_t i = 0, max = request->params(); i < max; i++) {
        const AsyncWebParameter* p = request->getParam(i);
        if (p->isPost() && !p->isFile()) {
          const char* keyRef = config.keyRef(p->name().c_str());
          settings[keyRef] = p->value();
        }
      }
      request->send(200);
      config.set(settings);
    });

  webServer
    .on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse(false);
      config.toJson(response->getRoot());
      response->setLength();
      request->send(response);
    });

  // network

  webServer
    .on("/api/network", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      espConnect.toJson(response->getRoot());
      response->setLength();
      request->send(response);
    });

  // system

  webServer
    .on("/api/system/restart", HTTP_ANY, [=](AsyncWebServerRequest* request) {
      restartTask.resume();
      request->send(200);
    });

  webServer
    .on("/api/system/reset", HTTP_ANY, [=](AsyncWebServerRequest* request) {
      resetTask.resume();
      request->send(200);
    });

  webServer
    .on("/api/system", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      JsonObject root = response->getRoot();
      Mycila::System::toJson(root);
      hx711.toJson(root["hx711"].to<JsonObject>());
      Mycila::PMU.toJson(root["pmu"].to<JsonObject>());
      Mycila::TaskMonitor.toJson(root["stack"].to<JsonObject>());
      loopTaskManager.toJson(root["task_managers"][0].to<JsonObject>());
      temperatureSensor.toJson(root["temp_sensor"].to<JsonObject>());
      response->setLength();
      request->send(response);
    });

  // beelance

  webServer
    .on("/api/beelance/history.json", HTTP_GET, [this](AsyncWebServerRequest* request) {
      if (LittleFS.exists(FILE_HISTORY)) {
        AsyncWebServerResponse* response = request->beginResponse(LittleFS, FILE_HISTORY, "application/json");
        request->send(response);
      } else {
        request->send(404);
      }
    });

  webServer
    .on("/api/beelance/history/reset", HTTP_ANY, [this](AsyncWebServerRequest* request) {
      Beelance::Beelance.clearHistory();
      request->send(200);
    });

  webServer
    .on("/api/beelance/history", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      Beelance::Beelance.historyToJson(response->getRoot());
      response->setLength();
      request->send(response);
    });

  webServer
    .on("/api/beelance", HTTP_GET, [this](AsyncWebServerRequest* request) {
      AsyncJsonResponse* response = new AsyncJsonResponse();
      JsonObject root = response->getRoot();
      Beelance::Beelance.toJson(root);
      Beelance::Beelance.historyToJson(root["history"].to<JsonObject>());
      response->setLength();
      request->send(response);
    });

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
      espConnect.toJson(root["network"].to<JsonObject>());
      // system
      Mycila::System::toJson(root["system"].to<JsonObject>());
      hx711.toJson(root["system"]["hx711"].to<JsonObject>());
      Mycila::PMU.toJson(root["system"]["pmu"].to<JsonObject>());
      Mycila::TaskMonitor.toJson(root["system"]["stack"].to<JsonObject>());
      loopTaskManager.toJson(root["system"]["task_managers"][0].to<JsonObject>());
      temperatureSensor.toJson(root["system"]["temp_sensor"].to<JsonObject>());
      response->setLength();
      request->send(response);
    });
}
