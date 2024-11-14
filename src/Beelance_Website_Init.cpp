// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <BeelanceWebsite.h>

#include <MycilaWebSerial.h>

#include <string>

#define TAG "WEBSITE"

extern const uint8_t logo_jpeg_gz_start[] asm("_binary__pio_data_logo_jpeg_gz_start");
extern const uint8_t logo_jpeg_gz_end[] asm("_binary__pio_data_logo_jpeg_gz_end");
extern const uint8_t config_html_gz_start[] asm("_binary__pio_data_config_html_gz_start");
extern const uint8_t config_html_gz_end[] asm("_binary__pio_data_config_html_gz_end");

void Beelance::WebsiteClass::init() {
  authMiddleware.setAuthType(AsyncAuthType::AUTH_DIGEST);
  authMiddleware.setRealm("YaSolR");
  authMiddleware.setUsername(BEELANCE_ADMIN_USERNAME);
  authMiddleware.setPassword(config.get(KEY_ADMIN_PASSWORD));
  authMiddleware.generateHash();

  webServer.addMiddleware(&authMiddleware);

  webServer.on("/logo", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "image/jpeg", logo_jpeg_gz_start, logo_jpeg_gz_end - logo_jpeg_gz_start);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "public, max-age=900");
    request->send(response);
  });

  // ping

  webServer.on("/api/ping", HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "pong");
    request->send(response);
  });

  // dashboard

  webServer.rewrite("/", "/dashboard").setFilter([](AsyncWebServerRequest* request) { return espConnect.getState() != Mycila::ESPConnect::State::PORTAL_STARTED; });

  // config

  webServer
    .on("/config", HTTP_GET, [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/html", config_html_gz_start, config_html_gz_end - config_html_gz_start);
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    });

  // web console

  WebSerial.begin(&webServer, "/console");
  WebSerial.onMessage([](const std::string& msg) {
    if (Mycila::string::startsWith(msg, "AT+")) {
      logger.info(TAG, "Enqueue AT Command: %s...", msg.c_str());
      Mycila::Modem.enqueueAT(msg.c_str());
    }
  });
  logger.forwardTo(&WebSerial);

  // app stats

  _firmName.set(Mycila::AppInfo.name);
  _firmVersionStat.set(Mycila::AppInfo.version);
  _firmManufacturerStat.set(Mycila::AppInfo.manufacturer);

  _firmFilenameStat.set(Mycila::AppInfo.firmware);
  _firmHashStat.set(Mycila::AppInfo.buildHash);
  _firmTimeStat.set(Mycila::AppInfo.buildDate);

  _deviceIdStat.set(Mycila::System::getChipIDStr());
  _cpuModelStat.set(ESP.getChipModel());
  _cpuCoresStat.set(std::to_string(ESP.getChipCores()));
  _bootCountStat.set(std::to_string(Mycila::System::getBootCount()));
  _bootReasonStat.set(Mycila::System::getLastRebootReason());
  _heapMemoryTotalStat.set(std::to_string(ESP.getHeapSize()) + " bytes");

  _hostnameStat.set(Mycila::AppInfo.defaultHostname);

  // home callbacks

  _restart.attachCallback([this](uint32_t value) {
    restartTask.resume();
    _restart.update(!restartTask.isPaused());
    dashboard.refreshCard(&_restart);
  });

  _safeBoot.attachCallback([this](uint32_t value) {
    Mycila::System::restartFactory("safeboot");
    _safeBoot.update(true);
    dashboard.refreshCard(&_safeBoot);
  });

  _sendNow.attachCallback([this](uint32_t value) {
    if (sendTask.isEnabled()) {
      sendTask.resume();
      sendTask.requestEarlyRun();
    }
    _sendNow.update(sendTask.isRunning() || (sendTask.isEnabled() && !sendTask.isPaused() && sendTask.isEarlyRunRequested()));
    dashboard.refreshCard(&_sendNow);
  });

  _tare.attachCallback([this](uint32_t value) {
    hx711TareTask.resume();
    _tare.update(hx711TareTask.isRunning() || (hx711TareTask.isEnabled() && !hx711TareTask.isPaused()));
    dashboard.refreshCard(&_tare);
  });

  _weight.attachCallback([this](int expectedWeight) {
    calibrationWeight = expectedWeight;
    hx711ScaleTask.resume(2 * Mycila::TaskDuration::SECONDS);
    _weight.update(expectedWeight);
    dashboard.refreshCard(&_weight);
  });

  _resetHistory.attachCallback([this](uint32_t value) {
    Beelance::Beelance.clearHistory();
    _resetHistory.update(false);
    dashboard.refreshCard(&_resetHistory);
  });

  _boolConfig(&_noSleepMode, KEY_PREVENT_SLEEP_ENABLE);

  _update(true);
}

void Beelance::WebsiteClass::disableTemperature() {
  dashboard.remove(&_chartLatestTemp);
  dashboard.remove(&_chartHourlyTemp);
  dashboard.remove(&_chartDailyTemp);
}

void Beelance::WebsiteClass::_boolConfig(Card* card, const char* key) {
  card->attachCallback([key, card, this](int value) {
    card->update(value);
    dashboard.refreshCard(card);
    config.setBool(key, value);
  });
}

namespace Beelance {
  WebsiteClass Website;
} // namespace Beelance
