// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <MycilaWebSerial.h>

#include <MycilaAppInfo.h>
#include <MycilaConfig.h>
#include <MycilaDS18.h>
#include <MycilaESPConnect.h>
#include <MycilaHX711.h>
#include <MycilaLogger.h>
#include <MycilaModem.h>
#include <MycilaPMU.h>
#include <MycilaSystem.h>
#include <MycilaTaskManager.h>
#include <MycilaTaskMonitor.h>
#include <MycilaTime.h>

#include <BeelanceClass.h>
#include <BeelanceMacros.h>

extern AsyncWebServer webServer;
extern ESPDash dashboard;

extern Mycila::Logger logger;
extern Mycila::Config config;
extern Mycila::ESPConnect espConnect;

extern Mycila::HX711 hx711;
extern Mycila::DS18 temperatureSensor;

extern Mycila::TaskManager hx711TaskManager;
extern Mycila::TaskManager loopTaskManager;
extern Mycila::TaskManager modemTaskManager;

extern Mycila::Task espConnectTask;
extern Mycila::Task hx711ScaleTask;
extern Mycila::Task hx711TareTask;
extern Mycila::Task hx711Task;
extern Mycila::Task otaPrepareTask;
extern Mycila::Task pmuTask;
extern Mycila::Task resetTask;
extern Mycila::Task restartTask;
extern Mycila::Task sendTask;
extern Mycila::Task stackMonitorTask;
extern Mycila::Task startNetworkServicesTask;
extern Mycila::Task stopNetworkServicesTask;
extern Mycila::Task temperatureTask;
extern Mycila::Task watchdogTask;
extern Mycila::Task websiteTask;

extern Mycila::Task configureDebugTask;
extern Mycila::Task configureTaskMonitorTask;

extern Mycila::Task startModemTask;
extern Mycila::Task serialDebugATTask;
extern Mycila::Task modemLoopTask;

extern float calibrationWeight;
