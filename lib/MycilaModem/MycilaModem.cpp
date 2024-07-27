// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <MycilaModem.h>

#include <ArduinoHttpClient.h>
#include <MycilaLogger.h>
#include <MycilaTime.h>

#if defined(TINY_GSM_MODEM_A7670) && defined(MYCILA_GPS_RX_PIN) && defined(MYCILA_GPS_TX_PIN)
#define TINY_GSM_MODEM_A7670G 1
#include <TinyGPS++.h>
TinyGPSPlus tinyGPS;
#endif

#define TAG "MODEM"

extern Mycila::Logger logger;

Mycila::ModemClass::ModemClass() : _spy(MYCILA_MODEM_SERIAL), _modem(_spy) {}

void Mycila::ModemClass::begin() {
  if (_state != MODEM_OFF)
    return;

  logger.info(TAG, "Starting modem...");

#ifdef TINY_GSM_MODEM_A7670
  // Set modem reset pin ,reset modem
  pinMode(MYCILA_MODEM_RST_PIN, OUTPUT);
  digitalWrite(MYCILA_MODEM_RST_PIN, LOW);
  delay(100);
  digitalWrite(MYCILA_MODEM_RST_PIN, HIGH);
  delay(2600);
  digitalWrite(MYCILA_MODEM_RST_PIN, LOW);
#endif

  _powerModem();

  // Set modem baud
  MYCILA_MODEM_SERIAL.begin(115200, SERIAL_8N1, MYCILA_MODEM_RX_PIN, MYCILA_MODEM_TX_PIN);

#ifdef TINY_GSM_MODEM_A7670G
  logger.info(TAG, "Starting GPS...");
  Serial2.setRxBufferSize(1024);
  Serial2.begin(9600, SERIAL_8N1, MYCILA_GPS_RX_PIN, MYCILA_GPS_TX_PIN);
#endif

  _setState(MODEM_STARTING);
}

void Mycila::ModemClass::loop() {
  if (_state == MODEM_STARTING) {
    logger.info(TAG, "Init SIM...");

    if (_modem.init(_pin.c_str())) {
      logger.info(TAG, "SIM Ready!");
      _error = emptyString;

#ifdef TINY_GSM_MODEM_SIM7080
      // 2 Automatic
      _modem.setNetworkMode(2);

      // NB-IoT bands
      String nbiotBands = "+CBANDCFG=\"NB-IOT\",";
      nbiotBands.concat(_bands[MODEM_MODE_NB_IOT]);
      _modem.sendAT(nbiotBands.c_str());
      _modem.waitResponse();

      // LTE-M bands
      String ltemBands = "+CBANDCFG=\"CAT-M\",";
      ltemBands.concat(_bands[MODEM_MODE_LTE_M]);
      _modem.sendAT(ltemBands.c_str());
      _modem.waitResponse();

      // APN
      _modem.sendAT("+CNCFG=0,1,\"", _apn, "\"");
      _modem.waitResponse();
#endif

      // APN
      _modem.sendAT("+CGDCONT=1,\"IP\",\"", _apn, "\"");
      _modem.waitResponse();

      // go to registration
      _setMode(_mode);
      _timeState = ModemTimeState::MODEM_TIME_SYNCING;
      _setState(MODEM_WAIT_REGISTRATION);

    } else {
      SimStatus simStatus = _modem.getSimStatus();
      if (simStatus != SIM_READY) {
        switch (simStatus) {
          case SIM_ERROR:
            _error = "SIM not detected";
            break;
          case SIM_LOCKED:
            _error = "SIM PIN required";
            break;
          case SIM_ANTITHEFT_LOCKED:
            _error = "SIM Locked";
            break;
          default:
            _error = "SIM Error";
            break;
        }
        logger.error(TAG, "Init SIM Error: %s", _error.c_str());
        _powerModem();
      }
    }
  }

  if (_state == MODEM_WAIT_REGISTRATION && millis() - _registrationCheckLastTime >= 2000) {
    logger.info(TAG, "Check registration...");
    _registrationCheckCount--;

    if (_modem.isNetworkConnected()) {
      logger.info(TAG, "Registered!");

      activateGPS();
      _gpsSyncStartTime = millis();
      _lastRefreshTime = 0;
      _setState(MODEM_GPS);

    } else if (_registrationCheckCount <= 0) {
      if (!_candidate) {
        logger.warn(TAG, "Timeout registering with any operator");
        _setState(MODEM_SEARCHING);

      } else {
        logger.warn(TAG, "Timeout registering with %s (%d)", _candidate->name.c_str(), _candidate->mode);

        while (true) {
          _candidateIndex++;
          _candidate = _candidateIndex < _operators.size() ? &_operators[_candidateIndex] : nullptr;

          if (!_candidate) {
            logger.warn(TAG, "No more operator to try");
            _setState(MODEM_SEARCHING);
            break;
          }

          logger.info(TAG, "Try associate with %s (%d)...", _candidate->name.c_str(), _candidate->mode);
          _setMode(_candidate->mode);
          _modem.sendAT("+COPS=0,0,\"", _candidate->name.c_str(), "\",", _candidate->mode);

          if (_modem.waitResponse(60000) == 1) {
            logger.info(TAG, "Associated with %s (%d)", _candidate->name.c_str(), _candidate->mode);
            _registrationCheckCount = 7;
            _setState(MODEM_WAIT_REGISTRATION);
            break;

          } else {
            logger.warn(TAG, "Failed to associate with %s (%d)", _candidate->name.c_str(), _candidate->mode);
          }
        }
      }
    } else {
      logger.info(TAG, "Not registered yet.");
    }

    _sync();
    _registrationCheckLastTime = millis();
  }

  if (_state == MODEM_SEARCHING) {
    logger.info(TAG, "Searching for operators...");

    // de-register
    _modem.sendAT("+COPS=2");
    _modem.waitResponse();
    _operator = emptyString;

    // https://help.onomondo.com/en/how-to-clear-the-fplmn-list
    // Query state: AT+CRSM=176,28539,0,0,12AT+CRSM=176,28539,0,0,12
    _modem.sendAT("+CRSM=214,28539,0,0,12,\"FFFFFFFFFFFFFFFFFFFFFFFF\"");
    _modem.waitResponse(2000L, "+CRSM:");

    _setMode(_mode);
    _modem.sendAT("+COPS=?");

    _operators.clear();
    _candidateIndex = -1;
    _candidate = nullptr;

    _spy.setTimeout(70000);
    if (_spy.readStringUntil(':').endsWith("+COPS")) {
      _spy.setTimeout(1000);
      while (!_spy.readStringUntil('(').isEmpty()) {
        ModemOperatorSearchResult op;
        op.state = static_cast<ModemOperatorState>(_spy.parseInt());
        _spy.readStringUntil('"');
        op.name = _spy.readStringUntil('"');
        op.name.trim();
        _spy.readStringUntil('"');
        _spy.readStringUntil('"');
        _spy.readStringUntil('"');
        op.code = _spy.readStringUntil('"');
        op.code.trim();
        _spy.readStringUntil(',');
        op.mode = _spy.parseInt();

        if (!op.name.isEmpty()) {
          if (op.state == MODEM_OPERATOR_FORBIDDEN) {
            logger.warn(TAG, "Skipping forbidden operator %s (%d)", op.name.c_str(), op.mode);
          } else {
            logger.info(TAG, "Found operator %s (%d) code=%s, state=%d", op.name.c_str(), op.mode, op.code.c_str(), op.state);
            _operators.push_back(op);
          }
        }
      }
      _readDrop();

      for (_candidateIndex = 0; _candidateIndex < _operators.size(); _candidateIndex++) {
        _candidate = &_operators[_candidateIndex];

        logger.info(TAG, "Try associate with %s (%d)...", _candidate->name.c_str(), _candidate->mode);

        _setMode(_candidate->mode);
        _modem.sendAT("+COPS=0,0,\"", _candidate->name.c_str(), "\",", _candidate->mode);

        if (_modem.waitResponse(60000) == 1) {
          logger.info(TAG, "Associated with %s (%d)", _candidate->name.c_str(), _candidate->mode);
          _registrationCheckCount = 7;
          _setState(MODEM_WAIT_REGISTRATION);
          break;

        } else {
          logger.warn(TAG, "Failed to associate with %s (%d)", _candidate->name.c_str(), _candidate->mode);
        }
      }
    } else {
      // search again
    }

    _sync();
  }

  if (_state == MODEM_GPS && millis() - _lastRefreshTime >= 5000) {
    _sync();

    if (_gpsState == MODEM_GPS_SYNCED) {
      _setState(MODEM_CONNECTING);

    } else if (millis() - _gpsSyncStartTime >= _gpsSyncTimeout * 1000) {
      logger.error(TAG, "GPS Sync timeout!");
      _gpsState = MODEM_GPS_TIMEOUT;
      _setState(MODEM_CONNECTING);

    } else {
      _lastRefreshTime = millis();
    }
  }

  if (_state == MODEM_CONNECTING) {
    if (activateData()) {
      _sync();
      activateGPS();
      _setState(MODEM_READY);
    } else {
      logger.error(TAG, "Failed to activate data!");
      _setState(MODEM_STARTING);
    }
  }

  // refresh modem info
  if (_state > MODEM_OFF && millis() - _lastRefreshTime >= 30000) {
    _sync();
    _lastRefreshTime = millis();
  }

  _dequeueATCommands();
}

void Mycila::ModemClass::powerOff() {
  // Turn off modem
  _modem.poweroff();
  // Wait until the modem does not respond to the command, and then proceed to the next step
  while (_modem.testAT())
    delay(500);

  MYCILA_MODEM_SERIAL.end();
  digitalWrite(MYCILA_MODEM_PWR_PIN, LOW);

#ifdef TINY_GSM_MODEM_A7670
  digitalWrite(MYCILA_MODEM_RST_PIN, LOW);
#endif

#ifdef TINY_GSM_MODEM_A7670G
  Serial2.end();
#endif
}

void Mycila::ModemClass::setDebug(bool debug) {
  if (debug) {
    _readBuffer.reserve(512);
    _writeBuffer.reserve(512);
    _readBuffer.concat("<< ");
    _writeBuffer.concat(">> ");
    _spy.onRead(std::bind(&Mycila::ModemClass::_onRead, this, std::placeholders::_1, std::placeholders::_2));
    _spy.onWrite(std::bind(&Mycila::ModemClass::_onWrite, this, std::placeholders::_1, std::placeholders::_2));

  } else {
    _spy.onRead(nullptr);
    _spy.onWrite(nullptr);
    _readBuffer = emptyString;
    _writeBuffer = emptyString;
  }
}

int Mycila::ModemClass::sendTCP(const String& host, uint16_t port, const String& payload, const uint16_t connectTimeoutSec) {
  TinyGsmClient client(_modem);
  client.setTimeout(connectTimeoutSec * 1000);
  int code = ESP_ERR_TIMEOUT;
  if (client.connect(host.c_str(), port, connectTimeoutSec)) {
    client.print(payload);
    client.flush();
    client.stop();
    code = ESP_OK;
  }
  return code;
}

#ifdef TINY_GSM_MODEM_A7670
int Mycila::ModemClass::httpPOST(const String& url, const String& payload, const uint16_t connectTimeoutSec) {
  if (url.isEmpty() || payload.isEmpty())
    return ESP_ERR_INVALID_ARG;

  if (!_modem.https_begin())
    return ESP_ERR_INVALID_STATE;

  if (!_modem.https_set_url(url.c_str()))
    return ESP_ERR_INVALID_ARG;

  _modem.https_set_timeout(connectTimeoutSec);
  _modem.https_set_user_agent(_model);
  _modem.https_set_content_type("application/json");
  if (_modem.https_post(payload.c_str()) == -1)
    return ESP_ERR_INVALID_STATE;

  return ESP_OK;
}
#endif

#ifdef TINY_GSM_MODEM_SIM7080
int Mycila::ModemClass::httpPOST(const String& url, const String& payload, const uint16_t connectTimeoutSec) {
  if (url.isEmpty() || payload.isEmpty())
    return ESP_ERR_INVALID_ARG;

  if (!_modem.isNetworkConnected())
    return ESP_ERR_INVALID_STATE;

  enum URLParseState { PROTOCOL,
                       SEPERATOR,
                       HOST,
                       PORT,
                       PATH } state = PROTOCOL;
  String protocol;
  String host;
  String port;
  String path = "/";

  for (int i = 0; i < url.length(); i++) {
    switch (state) {
      case URLParseState::PROTOCOL:
        if (url[i] == ':')
          state = URLParseState::SEPERATOR;
        else
          protocol.concat(url[i]);
        break;
      case URLParseState::SEPERATOR:
        if (url[i] != '/') {
          state = HOST;
          host.concat(url[i]);
        }
        break;
      case URLParseState::HOST:
        if (url[i] == ':')
          state = PORT;
        else if (url[i] == '/')
          state = PATH;
        else
          host.concat(url[i]);
        break;
      case URLParseState::PORT:
        if (url[i] == '/')
          state = PATH;
        else
          port.concat(url[i]);
        break;
      case URLParseState::PATH:
        path.concat(url[i]);
        break;
      default:
        assert(false);
        break;
    }
  }

  protocol.toLowerCase();

  if (host.isEmpty() || (protocol != "http" && protocol != "https"))
    return ESP_ERR_INVALID_ARG;

  int ret = HTTP_SUCCESS;

  if (protocol == "https") {
    const uint16_t httpsPort = port.isEmpty() ? 443 : port.toInt();
    TinyGsmClientSecure client(_modem);
    client.setTimeout(connectTimeoutSec * 1000);
    if (client.connect(host.c_str(), httpsPort, connectTimeoutSec)) {
      HttpClient http(client, host, httpsPort);
      http.setTimeout(connectTimeoutSec * 1000);
      ret = http.post(path, "application/json", payload);
      http.stop();
    } else {
      ret = HTTP_ERROR_CONNECTION_FAILED;
    }

  } else if (protocol == "http") {
    const uint16_t httpPort = port.isEmpty() ? 80 : port.toInt();
    TinyGsmClient client(_modem);
    client.setTimeout(connectTimeoutSec * 1000);
    if (client.connect(host.c_str(), httpPort, connectTimeoutSec)) {
      HttpClient http(client, host, httpPort);
      http.setTimeout(connectTimeoutSec * 1000);
      ret = http.post(path, "application/json", payload);
      http.stop();
    } else {
      ret = HTTP_ERROR_CONNECTION_FAILED;
    }

  } else {
    assert(false);
  }

  switch (ret) {
    case HTTP_SUCCESS:
      return ESP_OK;
    case HTTP_ERROR_TIMED_OUT:
      return ESP_ERR_TIMEOUT;
    case HTTP_ERROR_CONNECTION_FAILED:
      return ESP_ERR_INVALID_STATE;
    case HTTP_ERROR_INVALID_RESPONSE:
      return ESP_ERR_INVALID_RESPONSE;
    default:
      // HTTP_ERROR_API
      return ESP_FAIL;
  }
}
#endif

void Mycila::ModemClass::_onRead(const uint8_t* buffer, size_t size) {
  if (size) {
    _readBuffer.concat((const char*)buffer, size);
    if (_readBuffer.endsWith("\n")) {
      size = _readBuffer.endsWith("\r\n") ? _readBuffer.length() - 2 : _readBuffer.length() - 1;
      logger.debug(TAG, "%.*s", size, _readBuffer.c_str());
      _readBuffer.clear();
      _readBuffer.concat("<< ");
    }
  }
}

void Mycila::ModemClass::_onWrite(const uint8_t* buffer, size_t size) {
  if (size) {
    _writeBuffer.concat((const char*)buffer, size);
    if (_writeBuffer.endsWith("\n")) {
      size = _writeBuffer.endsWith("\r\n") ? _writeBuffer.length() - 2 : _writeBuffer.length() - 1;
      logger.debug(TAG, "%.*s", size, _writeBuffer.c_str());
      _writeBuffer.clear();
      _writeBuffer.concat(">> ");
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

void Mycila::ModemClass::_setState(ModemState state) {
  bool changed = _state != state;
  _state = state;
  if (changed && _callback)
    _callback(_state);
}

bool Mycila::ModemClass::_syncGPS() {
  if (_gpsState != MODEM_GPS_SYNCED)
    _gpsState = MODEM_GPS_SYNCING;

#ifdef TINY_GSM_MODEM_A7670G
  if (Serial2.available()) {
    while (Serial2.available()) {
      int c = Serial2.read();
      if (tinyGPS.encode(c)) {
        bool valid = tinyGPS.location.isValid() && tinyGPS.date.isValid() && tinyGPS.time.isValid() && tinyGPS.altitude.isValid() && tinyGPS.hdop.isValid();
        if (valid && tinyGPS.altitude.meters() >= 0) {
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
          logger.info(TAG, "GPS Synced!");
          return true;
        }
      }
    }
  }
#endif

#ifdef TINY_GSM_MODEM_SIM7080
  uint8_t status = 0;
  if (_modem.getGPS(&status, &_gpsData.latitude, &_gpsData.longitude, nullptr, &_gpsData.altitude, nullptr, nullptr, &_gpsData.accuracy, &_gpsData.time.tm_year, &_gpsData.time.tm_mon, &_gpsData.time.tm_mday, &_gpsData.time.tm_hour, &_gpsData.time.tm_min, &_gpsData.time.tm_sec) && _gpsData.altitude >= 0) {
    _gpsData.time.tm_year -= 1900;
    _gpsData.time.tm_mon -= 1;
    logger.info(TAG, "GPS Synced!");
    return true;
  }
#endif

  return false;
}

bool Mycila::ModemClass::_syncTime() {
  if (_timeState == MODEM_TIME_SYNCED)
    return true;

  _timeState = MODEM_TIME_SYNCING;

  // try GPS Time
  if (_gpsState == MODEM_GPS_SYNCED) {
    setenv("TZ", "UTC0", 1);
    tzset();

    struct timeval now = {mktime(&_gpsData.time), 0};
    settimeofday(&now, nullptr);

    setenv("TZ", _timeZoneInfo.c_str(), 1);
    tzset();

    String time = Mycila::Time::getLocalStr();
    if (!time.isEmpty()) {
      logger.info(TAG, "Time synced from GPS: %s", time.c_str());
      return true;
    }
  }

  // otherwise try time from cell network, which is sadly unreliable
  struct tm t = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  float timezone = 0;
  if (!_modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL).startsWith("80/01/06") && _modem.getNetworkTime(&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, &timezone)) {
    // +CCLK: "24/04/02,13:39:57+08 (dst, gmt+1, so 11:39 gmt)
    t.tm_year -= 1900;
    t.tm_mon -= 1;

    setenv("TZ", "UTC0", 1);
    tzset();

    struct timeval now = {mktime(&t) - static_cast<int32_t>(timezone * 3600.0), 0};
    settimeofday(&now, nullptr);

    setenv("TZ", _timeZoneInfo.c_str(), 1);
    tzset();

    String time = Mycila::Time::getLocalStr();
    if (!time.isEmpty()) {
      logger.info(TAG, "Time synced from cellular network: %s", time.c_str());
      return true;
    }
  }

  return false;
}

void Mycila::ModemClass::_syncInfo() {
  // signal quality
  int16_t sq = _modem.getSignalQuality();
  _signal = sq >= 0 && sq <= 31 ? map(sq, 0, 31, 0, 100) : 0;

  // misc info
  _iccid = _modem.getSimCCID();
  _imei = _modem.getIMEI();
  _imsi = _modem.getIMSI();
  _localIP = _modem.getLocalIP();
  _model = _modem.getModemName();
  _operator = _modem.getOperator();
}

void Mycila::ModemClass::_sync() {
  if (_state < MODEM_STARTING)
    return;

  _syncInfo();

  if (_syncGPS()) {
    _gpsState = MODEM_GPS_SYNCED;
  }

  if (_syncTime()) {
    _timeState = ModemTimeState::MODEM_TIME_SYNCED;
  }
}

void Mycila::ModemClass::_dequeueATCommands() {
  if (_commands.size()) {
    for (size_t i = 0; i < _commands.size(); i++) {
      logger.info(TAG, "Execute command: %s", _commands[i].c_str());
      if (_commands[i].startsWith("AT+"))
        _modem.sendAT(_commands[i].substring(2).c_str());
      else
        _modem.sendAT(_commands[i].c_str());
      delay(5000);
      while (!_spy.available()) {
        delay(5000);
      }
      _readDrop();
    }
    _commands.clear();
  }
}

bool Mycila::ModemClass::activateData() {
#ifdef TINY_GSM_MODEM_A7670
  logger.info(TAG, "Activate Data...");
  _modem.gprsConnect(_apn.c_str());
#endif
#ifdef TINY_GSM_MODEM_SIM7080
  logger.info(TAG, "Disable GPS...");
  _modem.disableGPS(); // GPS is incompatible with networking for SIM7080
  logger.info(TAG, "Activate Data...");
  _modem.sendAT("+CNACT=0,1");
#endif
  return _modem.waitForNetwork();
}

void Mycila::ModemClass::activateGPS() {
  logger.info(TAG, "Enable GPS...");
#ifdef TINY_GSM_MODEM_SIM7080
  _modem.enableGPS(); // GPS is incompatible with networking for SIM7080
#endif
}

void Mycila::ModemClass::_powerModem() {
  // Turn on modem
  pinMode(MYCILA_MODEM_PWR_PIN, OUTPUT);
  digitalWrite(MYCILA_MODEM_PWR_PIN, LOW);
  delay(100);
  digitalWrite(MYCILA_MODEM_PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(MYCILA_MODEM_PWR_PIN, LOW);
}

namespace Mycila {
  ModemClass Modem;
} // namespace Mycila
