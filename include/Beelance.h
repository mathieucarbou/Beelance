// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <ESPmDNS.h>
#include <HardwareSerial.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <WebSerialLite.h>

#include <MycilaAppInfo.h>
#include <MycilaConfig.h>
#include <MycilaESPConnect.h>
#include <MycilaHTTPd.h>
#include <MycilaHX711.h>
#include <MycilaLogger.h>
#include <MycilaModem.h>
#include <MycilaPMU.h>
#include <MycilaSystem.h>
#include <MycilaTaskManager.h>
#include <MycilaTaskMonitor.h>
#include <MycilaTemperatureSensor.h>
#include <MycilaTime.h>

#include <BeelanceClass.h>
#include <BeelanceMacros.h>

extern AsyncWebServer webServer;
extern ESPDash dashboard;

extern Mycila::HX711 hx711;
extern Mycila::TemperatureSensor temperatureSensor;

extern Mycila::TaskManager hx711TaskManager;
extern Mycila::TaskManager loopTaskManager;
extern Mycila::TaskManager modemTaskManager;

extern Mycila::Task espConnectTask;
extern Mycila::Task otaPrepareTask;
extern Mycila::Task resetTask;
extern Mycila::Task restartTask;
extern Mycila::Task sendTask;
extern Mycila::Task stackMonitorTask;
extern Mycila::Task startNetworkServicesTask;
extern Mycila::Task stopNetworkServicesTask;
extern Mycila::Task temperatureTask;
extern Mycila::Task hx711Task;
extern Mycila::Task hx711ScaleTask;
extern Mycila::Task hx711TareTask;
extern Mycila::Task watchdogTask;
extern Mycila::Task websiteTask;

extern Mycila::Task configureDebugTask;
extern Mycila::Task configureTaskMonitorTask;

extern Mycila::Task startModemTask;
extern Mycila::Task serialDebugATTask;
extern Mycila::Task modemLoopTask;
extern Mycila::Task modemConnectivityCheckTask;

extern float calibrationWeight;
