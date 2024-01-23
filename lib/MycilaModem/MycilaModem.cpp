// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <MycilaModem.h>

#include <MycilaLogger.h>

#if defined(TINY_GSM_MODEM_A7670) && defined(MYCILA_GPS_RX_PIN) && defined(MYCILA_GPS_TX_PIN)
#define MYCILA_GPS_SHIELD 1
#endif

#ifdef MYCILA_GPS_SHIELD
#include <TinyGPS++.h>
TinyGPSPlus tinyGPS;
#endif

#define TAG "MODEM"

Mycila::ModemClass::ModemClass() : _spy(MYCILA_MODEM_SERIAL), _modem(_spy) {}

void Mycila::ModemClass::begin() {
  if (_state != MODEM_OFF)
    return;
  _start();
}

void Mycila::ModemClass::loop() {
  _readDrop();
  switch (_state) {
    case MODEM_OFF:
      return;
    case MODEM_STARTING:
      _loopStarting();
      break;
    case MODEM_REGISTERING:
      _loopRegistering();
      break;
    case MODEM_SEARCHING:
      _loopSearching();
      break;
    case MODEM_CONNECTING:
      _loopConnecting();
      break;
    case MODEM_CONNECTED: {
      break;
    }
    default:
      break;
  }
}

void Mycila::ModemClass::setDebug(bool debug) {
  if (debug) {
    _readBuffer.reserve(512);
    _writeBuffer.reserve(512);
    _readBuffer += "<< ";
    _writeBuffer += ">> ";
    _spy.onRead(std::bind(&Mycila::ModemClass::_onRead, this, std::placeholders::_1, std::placeholders::_2));
    _spy.onWrite(std::bind(&Mycila::ModemClass::_onWrite, this, std::placeholders::_1, std::placeholders::_2));

  } else {
    _spy.onRead(nullptr);
    _spy.onWrite(nullptr);
    _readBuffer = emptyString;
    _writeBuffer = emptyString;
  }
}

bool Mycila::ModemClass::clearFPLMN() {
  // https://help.onomondo.com/en/how-to-clear-the-fplmn-list
  // Query status: AT+CRSM=176,28539,0,0,12AT+CRSM=176,28539,0,0,12
  _modem.sendAT("AT+CRSM=214,28539,0,0,12,\"FFFFFFFFFFFFFFFFFFFFFFFF\"");
  return _modem.waitResponse(1000L, "+CRSM:") == 1;
}

bool Mycila::ModemClass::syncTime() {
  if (_state < MODEM_CONNECTING)
    return false;
  if (_modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL).startsWith("80/01/06"))
    return false;
  Mycila::Logger.debug(TAG, "Sync Time...");
  struct tm t = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  if (_modem.getNetworkTime(&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, nullptr)) {
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    struct timeval now = {mktime(&t), 0};
    settimeofday(&now, nullptr);
    tzset();
    _timeSynced = true;
    return true;
  }
  return false;
}

#ifdef MYCILA_GPS_SHIELD
bool Mycila::ModemClass::syncGPS() {
  if (_state < MODEM_CONNECTING)
    return false;
  if (!Serial2.available())
    return false;
  Mycila::Logger.debug(TAG, "Sync GPS...");
  while (Serial2.available()) {
    int c = Serial2.read();
    if (tinyGPS.encode(c)) {
      bool valid = tinyGPS.location.isValid() && tinyGPS.date.isValid() && tinyGPS.time.isValid() && tinyGPS.altitude.isValid() && tinyGPS.hdop.isValid();
      if (valid) {
        _gpsData.latitude = tinyGPS.location.lat();
        _gpsData.longitude = tinyGPS.location.lng();
        _gpsData.altitude = tinyGPS.altitude.meters();
        _gpsData.accuracy = tinyGPS.hdop.hdop();
        _gpsData.time.tm_year = tinyGPS.date.year() - 1900;
        _gpsData.time.tm_mon = tinyGPS.date.month() - 1;
        _gpsData.time.tm_mday = tinyGPS.date.day();
        _gpsData.time.tm_hour = tinyGPS.time.hour();
        _gpsData.time.tm_min = tinyGPS.time.minute();
        _gpsData.time.tm_sec = tinyGPS.time.second();
        _gpsSynced = true;
        return true;
      }
    }
  }
  return false;
}
#else
bool Mycila::ModemClass::syncGPS() {
  if (_state < MODEM_CONNECTING)
    return false;
  Mycila::Logger.debug(TAG, "Sync GPS...");
  uint8_t status = 0;
  if (_modem.getGPS(&status,
                    &_gpsData.latitude,
                    &_gpsData.longitude,
                    nullptr,
                    &_gpsData.altitude,
                    nullptr,
                    nullptr,
                    &_gpsData.accuracy,
                    &_gpsData.time.tm_year,
                    &_gpsData.time.tm_mon,
                    &_gpsData.time.tm_mday,
                    &_gpsData.time.tm_hour,
                    &_gpsData.time.tm_min,
                    &_gpsData.time.tm_sec)) {
    _gpsData.time.tm_year -= 1900;
    _gpsData.time.tm_mon -= 1;
    _gpsSynced = true;
    return true;
  }
  return false;
}
#endif // MYCILA_GPS_SHIELD

bool Mycila::ModemClass::registerWithOperatorCode(const String& code, uint8_t mode) {
  Mycila::Logger.debug(TAG, "Manually register to %s,%d", code.c_str(), mode);
  _setMode(mode);
  _modem.sendAT("+COPS=1,2,\"", code.c_str(), "\",", mode);
  return _modem.waitResponse(MYCILA_MODEM_REGISTER_TIMEOUT) == 1;
}

bool Mycila::ModemClass::deregisterFromOperator() {
  Mycila::Logger.debug(TAG, "Deregister from operator...");
  _modem.sendAT("+COPS=2");
  _modem.waitResponse();
  _operator = _modem.getOperator();
  return _operator.isEmpty();
}

void Mycila::ModemClass::scanForOperators() {
  Mycila::Logger.debug(TAG, "Scan for operators...");
  clearFPLMN();
  deregisterFromOperator();
  _search();
}

void Mycila::ModemClass::_onRead(const uint8_t* buffer, size_t size) {
  if (size) {
    _readBuffer += String((const char*)buffer, size);
    if (_readBuffer.endsWith("\n")) {
      size = _readBuffer.endsWith("\r\n") ? _readBuffer.length() - 2 : _readBuffer.length() - 1;
      Mycila::Logger.debug(TAG, "%.*s", size, _readBuffer.c_str());
      _readBuffer.clear();
      _readBuffer += "<< ";
    }
  }
}

void Mycila::ModemClass::_onWrite(const uint8_t* buffer, size_t size) {
  if (size) {
    _writeBuffer += String((const char*)buffer, size);
    if (_writeBuffer.endsWith("\n")) {
      size = _writeBuffer.endsWith("\r\n") ? _writeBuffer.length() - 2 : _writeBuffer.length() - 1;
      Mycila::Logger.debug(TAG, "%.*s", size, _writeBuffer.c_str());
      _writeBuffer.clear();
      _writeBuffer += ">> ";
    }
  }
}

void Mycila::ModemClass::_readDrop() {
  while (_spy.available())
    _spy.read();
}

void Mycila::ModemClass::_setMode(uint8_t mode) {
#ifdef TINY_GSM_MODEM_SIM7080
  switch (mode) {
    case 7:
      _modem.setPreferredMode(1);
      break;
    case 9:
      _modem.setPreferredMode(2);
      break;
    default:
      _modem.setPreferredMode(3);
      break;
  }
#endif
}

void Mycila::ModemClass::_start() {
  Mycila::Logger.debug(TAG, "Starting modem...");

  _gpsSynced = false;
  _timeSynced = false;

#ifdef TINY_GSM_MODEM_A7670
  // Set modem reset pin ,reset modem
  pinMode(MYCILA_MODEM_RST_PIN, OUTPUT);
  digitalWrite(MYCILA_MODEM_RST_PIN, !MYCILA_MODEM_RST_PIN);
  delay(100);
  digitalWrite(MYCILA_MODEM_RST_PIN, MYCILA_MODEM_RST_PIN);
  delay(2600);
  digitalWrite(MYCILA_MODEM_RST_PIN, !MYCILA_MODEM_RST_PIN);
#endif

  // Turn on modem
  pinMode(MYCILA_MODEM_PWR_PIN, OUTPUT);
  digitalWrite(MYCILA_MODEM_PWR_PIN, LOW);
  delay(100);
  digitalWrite(MYCILA_MODEM_PWR_PIN, HIGH);
  delay(100);
  digitalWrite(MYCILA_MODEM_PWR_PIN, LOW);

  // Set modem baud
  MYCILA_MODEM_SERIAL.begin(115200, SERIAL_8N1, MYCILA_MODEM_RX_PIN, MYCILA_MODEM_TX_PIN);

  _state = MODEM_STARTING;
}

void Mycila::ModemClass::_register() {
  Mycila::Logger.debug(TAG, "Register to network...");

// optimizations for SIM7080
#ifdef TINY_GSM_MODEM_SIM7080
  _modem.setNetworkMode(2);

  _setMode(_mode);

  // NB-IoT bands
  String nbiotBands = "+CBANDCFG=\"NB-IOT\",";
  nbiotBands += _bands[MODEM_MODE_NB_IOT];
  _modem.sendAT(nbiotBands.c_str());
  _modem.waitResponse();

  // LTE-M bands
  String ltemBands = "+CBANDCFG=\"CAT-M\",";
  ltemBands += _bands[MODEM_MODE_LTE_M];
  _modem.sendAT(ltemBands.c_str());
  _modem.waitResponse();

  // band scan
  _modem.sendAT("+CNBS=3");
  _modem.waitResponse();
#endif

  // APN
  _modem.sendAT("+CGDCONT=1,\"IP\",\"", _apn, "\"");
  _modem.waitResponse();
  _modem.sendAT("+CNCFG=0,1,\"", _apn, "\"");
  _modem.waitResponse();

  // grab modem info
  _imei = _modem.getIMEI();
  _iccid = _modem.getSimCCID();
  _model = _modem.getModemName();

  _since = millis();
  _state = MODEM_REGISTERING;
}

void Mycila::ModemClass::_search() {
  Mycila::Logger.debug(TAG, "Search for networks...");
  _candidate = nullptr;
  _candidateIndex = -1;
  _operators.clear();
  _state = MODEM_SEARCHING;
}

void Mycila::ModemClass::_connect() {
  Mycila::Logger.debug(TAG, "Connecting...");

  _imsi = _modem.getIMSI();
  _operator = _modem.getOperator();

#ifdef MYCILA_GPS_SHIELD
  Mycila::Logger.debug(TAG, "Starting GPS...");
  Serial2.begin(9600, SERIAL_8N1, MYCILA_GPS_RX_PIN, MYCILA_GPS_TX_PIN);
  Serial2.setRxBufferSize(1024);
#else
  _modem.enableGPS();
#endif

  _state = MODEM_CONNECTING;
}

void Mycila::ModemClass::_loopStarting() {
  Mycila::Logger.debug(TAG, "Init SIM...");
  if (_modem.init(_pin.c_str())) {
    Mycila::Logger.debug(TAG, "SIM Ready");
    _readDrop();
    _error = emptyString;
    _register();
  }
}

void Mycila::ModemClass::_loopRegistering() {
  // find if we have an operator to test
  if (!_candidate && _candidateIndex >= 0 && _candidateIndex < _operators.size()) {
    while (_candidateIndex >= 0 && _candidateIndex < _operators.size()) {
      if (_operators[_candidateIndex].status == MODEM_OPERATOR_FORBIDDEN) {
        Mycila::Logger.debug(TAG, "Skipping forbidden operator: %s", _operators[_candidateIndex].name.c_str());
        _candidateIndex++;
      } else {
        break;
      }
    }
    _candidate = _candidateIndex >= 0 && _candidateIndex < _operators.size() ? &_operators[_candidateIndex] : nullptr;

    // try to register to the candidate
    if (_candidate) {
      if (registerWithOperatorCode(_candidate->code, _candidate->mode)) {
        // reset registration timeout for this candidate
        _since = millis();

      } else {
        Mycila::Logger.debug(TAG, "Failed to register to %s", _candidate->name.c_str());
        _candidate = nullptr;
        _candidateIndex++; // prepare next candidate
        return;
      }
    }
  }

  // check for registration timeout
  if (millis() - _since >= MYCILA_MODEM_REGISTER_TIMEOUT) {
    Mycila::Logger.debug(TAG, "Timeout registering to any operator");
    if (_candidate) {
      Mycila::Logger.debug(TAG, "Try next operator");
      _candidate = nullptr;
      _candidateIndex++;
    } else {
      Mycila::Logger.debug(TAG, "Not registered to any operator: searching again...");
      _search();
    }
    return;
  }

  // test registration
  if (!_modem.isNetworkConnected()) {
    // check again in 1 second
    delay(1000);
    return;
  }

  _connect();
}

void Mycila::ModemClass::_loopSearching() {
  Mycila::Logger.debug(TAG, "Search for networks...");

  _setMode(_mode);
  _modem.sendAT("+COPS=?");

  if (_modem.waitResponse(MYCILA_MODEM_SEARCH_TIMEOUT, "+COPS:") != 1) {
    // search again
    return;
  }

  while (!_spy.readStringUntil('(').isEmpty()) {
    ModemOperatorSearchResult op;
    op.status = static_cast<ModemOperatorStatus>(_spy.parseInt());
    _spy.readStringUntil('"');
    op.name = _spy.readStringUntil('"');
    _spy.readStringUntil('"');
    _spy.readStringUntil('"');
    _spy.readStringUntil('"');
    op.code = _spy.readStringUntil('"');
    _spy.readStringUntil(',');
    op.mode = _spy.parseInt();
    if (!op.name.isEmpty())
      _operators.push_back(op);
  }

  _readDrop();

  for (const ModemOperatorSearchResult& op : _operators) {
    Mycila::Logger.debug(TAG, "Found operator: %s (%s) - stat=%d, mode=%d", op.name.c_str(), op.code.c_str(), op.status, op.mode);
  }

  _candidateIndex = _operators.size() > 0 ? 0 : -1;
  _register();
}

void Mycila::ModemClass::_loopConnecting() {
  Mycila::Logger.debug(TAG, "Activate Networking...");

#ifdef TINY_GSM_MODEM_SIM7080
  _modem.setNetworkActive();
#endif

#ifdef TINY_GSM_MODEM_A7670
  _modem.gprsConnect(_apn.c_str());
#endif

  _localIP = _modem.getLocalIP();

  if (_localIP.isEmpty())
    return;

  _state = MODEM_CONNECTED;
}

namespace Mycila {
  ModemClass Modem;
} // namespace Mycila
