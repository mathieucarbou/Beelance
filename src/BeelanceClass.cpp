// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#include <BeelanceWebsite.h>

#define TAG "BEELANCE"

void Beelance::BeelanceClass::_initHTTPd() {
  Mycila::HTTPd.init(&webServer, BEELANCE_ADMIN_USERNAME, Mycila::Config.get(KEY_ADMIN_PASSWORD));
}

void Beelance::BeelanceClass::_initWebsite() {
  Beelance::Website.init();
}

void Beelance::BeelanceClass::updateWebsite() {
  Beelance::Website.update();
}

void Beelance::BeelanceClass::sendMeasurements() {
  // TODO: implement
  Mycila::Logger.info(TAG, "Sending measurements...");
}

namespace Beelance {
  BeelanceClass Beelance;
} // namespace Beelance
