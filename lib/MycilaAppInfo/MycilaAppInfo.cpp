// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <Esp.h>
#include <MycilaAppInfo.h>
#include <MycilaString.h>
#include <MycilaSystem.h>

#include <string>

#ifndef APP_NAME
  #define APP_NAME "Beelance"
#endif

#ifndef APP_MANUFACTURER
  #define APP_MANUFACTURER "Mathieu Carbou"
#endif

#ifdef APP_MODEL_PRO
  #ifdef APP_MODEL_TRIAL
    #define APP_MODEL "Trial"
  #else
    #define APP_MODEL "Pro"
  #endif
#endif

#ifdef APP_MODEL_OSS
  #define APP_MODEL "OSS"
#endif

extern const char* __COMPILED_APP_VERSION__;
extern const char* __COMPILED_BUILD_BRANCH__;
extern const char* __COMPILED_BUILD_HASH__;
extern const char* __COMPILED_BUILD_NAME__;
extern const char* __COMPILED_BUILD_TIMESTAMP__;

Mycila::AppInfoClass::AppInfoClass() : id(Mycila::System::getChipIDStr()),
                                       name(APP_NAME),
                                       model(APP_MODEL),
                                       version(__COMPILED_APP_VERSION__),
                                       nameModel(name + " " + model),
                                       nameModelVersion(name + " " + model + " " + version),
                                       manufacturer(APP_MANUFACTURER),
                                       firmware(std::string(APP_NAME) + (std::isdigit(version[0]) ? "-v" : "-") + version.substr(0, version.find("_")) + "-" + __COMPILED_BUILD_NAME__ + ".bin"),
                                       buildBranch(__COMPILED_BUILD_BRANCH__),
                                       buildHash(__COMPILED_BUILD_HASH__),
                                       buildDate(__COMPILED_BUILD_TIMESTAMP__),
                                       defaultHostname(Mycila::string::toLowerCase(name + "-" + id)),
                                       defaultMqttClientId(Mycila::string::toLowerCase(name + "_" + id)),
                                       debug(firmware.find("debug") != std::string::npos),
                                       trial(firmware.find("trial") != std::string::npos) {}

void Mycila::AppInfoClass::toJson(const JsonObject& root) const {
  root["build_date"] = buildDate;
  root["build_hash"] = buildHash;
  root["debug"] = debug;
  root["firmware"] = firmware;
  root["id"] = id;
  root["manufacturer"] = manufacturer;
  root["model"] = model;
  root["name"] = name;
  root["trial"] = trial;
  root["version"] = version;
}

namespace Mycila {
  AppInfoClass AppInfo;
} // namespace Mycila
