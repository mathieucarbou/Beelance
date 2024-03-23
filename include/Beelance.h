// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <HardwareSerial.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <WebSerialLite.h>

#include <MycilaAppInfo.h>
#include <MycilaConfig.h>
#include <MycilaESPConnect.h>
#include <MycilaHTTPd.h>
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

extern Mycila::TemperatureSensor systemTemperatureSensor;
extern Mycila::TaskManager loopTaskManager;
extern Mycila::TaskManager modemTaskManager;

extern Mycila::Task espConnectTask;
extern Mycila::Task otaPrepareTask;
extern Mycila::Task profileTask;
extern Mycila::Task resetTask;
extern Mycila::Task restartTask;
extern Mycila::Task sendTask;
extern Mycila::Task stackMonitorTask;
extern Mycila::Task startNetworkServicesTask;
extern Mycila::Task stopNetworkServicesTask;
extern Mycila::Task systemTemperatureTask;
extern Mycila::Task websiteTask;

extern Mycila::Task configureDebugTask;
extern Mycila::Task configureNetworkTask;
extern Mycila::Task configurePMUTask;
extern Mycila::Task configureSystemTemperatureSensorTask;
extern Mycila::Task configureTaskMonitorTask;

extern Mycila::Task startModemTask;
extern Mycila::Task serialDebugATTask;
extern Mycila::Task modemLoopTask;
extern Mycila::Task modemSyncGPSTask;
