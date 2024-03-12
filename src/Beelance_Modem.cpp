// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <Beelance.h>

#define TAG "BEELANCE"

String readBuffer;
String writeBuffer;

void Beelance::BeelanceClass::_initModem() {
  Mycila::Logger.info(TAG, "Initializing Modem...");

  readBuffer.reserve(512);
  writeBuffer.reserve(512);

  serialAT.onRead([](const uint8_t* buffer, size_t size) {
    if (Mycila::Logger.isDebugEnabled()) {
      if (readBuffer.isEmpty())
        readBuffer += "<< ";
      readBuffer += String((const char*)buffer, size);
      if (readBuffer.endsWith("\n")) {
        size = readBuffer.endsWith("\r\n") ? readBuffer.length() - 2 : readBuffer.length() - 1;
        Mycila::Logger.debug(TAG, "%.*s", size, readBuffer.c_str());
        readBuffer.clear();
      }
    } else {
      readBuffer.clear();
    }
  });

  serialAT.onWrite([](const uint8_t* buffer, size_t size) {
    if (Mycila::Logger.isDebugEnabled()) {
      if (writeBuffer.isEmpty())
        writeBuffer += ">> ";
      writeBuffer += String((const char*)buffer, size);
      if (writeBuffer.endsWith("\n")) {
        size = writeBuffer.endsWith("\r\n") ? writeBuffer.length() - 2 : writeBuffer.length() - 1;
        Mycila::Logger.debug(TAG, "%.*s", size, writeBuffer.c_str());
        writeBuffer.clear();
      }
    } else {
      writeBuffer.clear();
    }
  });

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  pinMode(MODEM_PWR_PIN, OUTPUT);

  while (true) {
    Mycila::Logger.debug(TAG, "Waiting for Modem...");

    if (modem.testAT(1000))
      break;

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWR_PIN, LOW);
    delay(100);
    digitalWrite(MODEM_PWR_PIN, HIGH);
    delay(1000);
    digitalWrite(MODEM_PWR_PIN, LOW);
  }
}
