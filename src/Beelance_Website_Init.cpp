// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <BeelanceWebsite.h>

#include <ElegantOTA.h>
#include <WebSerialLite.h>

#define TAG "WEBSITE"

extern const uint8_t logo_png_gz_start[] asm("_binary__pio_data_logo_png_gz_start");
extern const uint8_t logo_png_gz_end[] asm("_binary__pio_data_logo_png_gz_end");
extern const uint8_t config_html_gz_start[] asm("_binary__pio_data_config_html_gz_start");
extern const uint8_t config_html_gz_end[] asm("_binary__pio_data_config_html_gz_end");

void Beelance::WebsiteClass::init() {
  webServer.rewrite("/dash/assets/logo/large", "/logo");
  webServer.rewrite("/dash/assets/logo", "/logo");
  webServer.rewrite("/ota/logo/dark", "/logo");
  webServer.rewrite("/ota/logo/light", "/logo");

  Mycila::HTTPd.get(
    "/logo",
    [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", logo_png_gz_start, logo_png_gz_end - logo_png_gz_start);
      response->addHeader("Content-Encoding", "gzip");
      response->addHeader("Cache-Control", "public, max-age=900");
      request->send(response);
    },
    true);

  // ping

  Mycila::HTTPd.get(
    "/ping",
    [](AsyncWebServerRequest* request) {
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "pong");
      request->send(response);
    },
    true);

  // dashboard

  dashboard.setAuthentication(BEELANCE_ADMIN_USERNAME, Mycila::Config.get(KEY_ADMIN_PASSWORD).c_str());
  webServer.rewrite("/", "/dashboard").setFilter([](AsyncWebServerRequest* request) { return ESPConnect.getState() != ESPConnectState::PORTAL_STARTED; });

  // config

  Mycila::HTTPd.get("/config", [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", config_html_gz_start, config_html_gz_end - config_html_gz_start);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // ota

  ElegantOTA.setAutoReboot(false);
  ElegantOTA.onStart([this]() { otaPrepareTask.resume(); });
  ElegantOTA.onEnd([this](bool success) {
    if (success) {
      Mycila::Logger.info(TAG, "OTA Update Success! Restarting...");
    } else {
      Mycila::Logger.error(TAG, "OTA Failed! Restarting...");
    }
    restartTask.resume();
  });
  ElegantOTA.begin(&webServer, BEELANCE_ADMIN_USERNAME, Mycila::Config.get(KEY_ADMIN_PASSWORD).c_str());

  // web console

  WebSerial.begin(&webServer, "/console", BEELANCE_ADMIN_USERNAME, Mycila::Config.get(KEY_ADMIN_PASSWORD));
  WebSerial.onMessage([](AsyncWebSocketClient*, const String& msg) {
    if (msg.startsWith("AT+"))
      Mycila::Modem.enqueueAT(msg);
  });
  Mycila::Logger.forwardTo(&WebSerial);

  // app stats

  _firmName.set((Mycila::AppInfo.name.c_str()));
  _firmVersionStat.set(Mycila::AppInfo.version.c_str());
  _firmManufacturerStat.set(Mycila::AppInfo.manufacturer.c_str());

  _firmFilenameStat.set(Mycila::AppInfo.firmware.c_str());
  _firmHashStat.set(Mycila::AppInfo.buildHash.c_str());
  _firmTimeStat.set(Mycila::AppInfo.buildDate.c_str());

  _deviceIdStat.set(Mycila::AppInfo.id.c_str());
  _cpuModelStat.set(ESP.getChipModel());
  _cpuCoresStat.set(String(ESP.getChipCores()).c_str());
  _bootCountStat.set(String(Mycila::System.getBootCount()).c_str());
  _heapMemoryTotalStat.set((String(ESP.getHeapSize()) + " bytes").c_str());

  // home callbacks

  _restart.attachCallback([=](uint32_t value) {
    restartTask.resume();
    _restart.update(!restartTask.isPaused());
    dashboard.refreshCard(&_restart);
  });

  _sendNow.attachCallback([=](uint32_t value) {
    sendTask.requestEarlyRun();
    _sendNow.update(sendTask.isEarlyRunRequested() || sendTask.isRunning());
    dashboard.refreshCard(&_sendNow);
  });

  _scanOps.attachCallback([=](uint32_t value) {
    if (value && Mycila::Modem.isReady()) {
      Mycila::Modem.scanForOperators();
    }
    _scanOps.update(Mycila::Modem.getState() == Mycila::ModemState::MODEM_SEARCHING);
    dashboard.refreshCard(&_scanOps);
  });

  _boolConfig(&_debugMode, KEY_DEBUG_ENABLE);
  _boolConfig(&_noSleepMode, KEY_NO_SLEEP_ENABLE);

  _update(true);
}

void Beelance::WebsiteClass::_boolConfig(Card* card, const char* key) {
  card->attachCallback([key, card, this](int value) {
    card->update(value);
    dashboard.refreshCard(card);
    Mycila::Config.setBool(key, value);
  });
}

namespace Beelance {
  WebsiteClass Website;
} // namespace Beelance
