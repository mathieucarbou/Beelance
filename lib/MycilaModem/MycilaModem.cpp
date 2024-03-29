// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
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

Mycila::ModemClass::ModemClass() : _spy(MYCILA_MODEM_SERIAL), _modem(_spy) {}

void Mycila::ModemClass::begin() {
  if (_state != MODEM_OFF)
    return;

  Mycila::Logger.info(TAG, "Starting modem...");

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

#ifdef TINY_GSM_MODEM_A7670G
  Mycila::Logger.info(TAG, "Starting GPS...");
  Serial2.setRxBufferSize(1024);
  Serial2.begin(9600, SERIAL_8N1, MYCILA_GPS_RX_PIN, MYCILA_GPS_TX_PIN);
#endif

  _state = MODEM_STARTING;
}

void Mycila::ModemClass::loop() {
  bool refreshInfo = false;

  if (_state == MODEM_STARTING) {
    Mycila::Logger.info(TAG, "Init SIM...");

    if (_modem.init(_pin.c_str())) {
      Mycila::Logger.info(TAG, "SIM Ready!");
      _error = emptyString;

#ifdef TINY_GSM_MODEM_SIM7080
      // 2 Automatic
      _modem.setNetworkMode(2);

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
      _state = MODEM_WAIT_REGISTRATION;

    } else {
      switch (_modem.getSimStatus()) {
        case SIM_ERROR:
          _error = "SIM not detected";
          break;
        case SIM_READY:
          _error = "SIM Ready";
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
      Mycila::Logger.error(TAG, "Init SIM Error: %s", _error.c_str());
      _state = MODEM_ERROR;
    }

    refreshInfo = true;
  }

  if (_state == MODEM_WAIT_REGISTRATION && millis() - _registrationCheckLastTime >= 2000) {
    Mycila::Logger.info(TAG, "Check registration...");
    _registrationCheckCount--;

    if (_modem.isNetworkConnected()) {
      Mycila::Logger.info(TAG, "Registered!");

      Mycila::Logger.info(TAG, "Sync GPS...");
#ifdef TINY_GSM_MODEM_SIM7080
      _modem.enableGPS(); // GPS is incompatible with networking
#endif
      _gpsSyncStartTime = millis();
      _state = MODEM_WAIT_GPS;
      if (_gpsState != MODEM_GPS_SYNCED)
        _gpsState = MODEM_GPS_SYNCING;

    } else if (_registrationCheckCount <= 0) {
      if (!_candidate) {
        Mycila::Logger.warn(TAG, "Timeout registering with any operator");
        _state = MODEM_SEARCHING;

      } else {
        Mycila::Logger.warn(TAG, "Timeout registering with %s (%d)", _candidate->name.c_str(), _candidate->mode);

        while (true) {
          _candidateIndex++;
          _candidate = _candidateIndex < _operators.size() ? &_operators[_candidateIndex] : nullptr;

          if (!_candidate) {
            Mycila::Logger.warn(TAG, "No more operator to try");
            _state = MODEM_SEARCHING;
            break;
          }

          Mycila::Logger.info(TAG, "Try associate with %s (%d)...", _candidate->name.c_str(), _candidate->mode);
          _setMode(_candidate->mode);
          _modem.sendAT("+COPS=0,0,\"", _candidate->name.c_str(), "\",", _candidate->mode);

          if (_modem.waitResponse(60000) == 1) {
            Mycila::Logger.info(TAG, "Associated with %s (%d)", _candidate->name.c_str(), _candidate->mode);
            _registrationCheckCount = 7;
            _state = MODEM_WAIT_REGISTRATION;
            break;

          } else {
            Mycila::Logger.warn(TAG, "Failed to associate with %s (%d)", _candidate->name.c_str(), _candidate->mode);
          }
        }
      }
    } else {
      Mycila::Logger.info(TAG, "Not registered yet.");
    }

    _registrationCheckLastTime = millis();
    refreshInfo = true;
  }

  if (_state == MODEM_SEARCHING) {
    Mycila::Logger.info(TAG, "Searching for operators...");

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
            Mycila::Logger.warn(TAG, "Skipping forbidden operator %s (%d)", op.name.c_str(), op.mode);
          } else {
            Mycila::Logger.info(TAG, "Found operator %s (%d) code=%s, state=%d", op.name.c_str(), op.mode, op.code.c_str(), op.state);
            _operators.push_back(op);
          }
        }
      }
      _readDrop();

      for (_candidateIndex = 0; _candidateIndex < _operators.size(); _candidateIndex++) {
        _candidate = &_operators[_candidateIndex];

        Mycila::Logger.info(TAG, "Try associate with %s (%d)...", _candidate->name.c_str(), _candidate->mode);

        _setMode(_candidate->mode);
        _modem.sendAT("+COPS=0,0,\"", _candidate->name.c_str(), "\",", _candidate->mode);

        if (_modem.waitResponse(60000) == 1) {
          Mycila::Logger.info(TAG, "Associated with %s (%d)", _candidate->name.c_str(), _candidate->mode);
          _registrationCheckCount = 7;
          _state = MODEM_WAIT_REGISTRATION;
          break;

        } else {
          Mycila::Logger.warn(TAG, "Failed to associate with %s (%d)", _candidate->name.c_str(), _candidate->mode);
        }
      }
    } else {
      // search again after
    }

    refreshInfo = true;
  }

  if (_state == MODEM_WAIT_GPS && (refreshInfo || millis() - _lastRefreshTime >= 2000)) {
    Mycila::Logger.info(TAG, "Check for GPS Sync...");
#ifdef TINY_GSM_MODEM_A7670G
    if (Serial2.available()) {
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
            _gpsState = MODEM_GPS_SYNCED;
          }
        }
      }
    }
#endif
#ifdef TINY_GSM_MODEM_SIM7080
    uint8_t status = 0;
    if (_modem.getGPS(&status, &_gpsData.latitude, &_gpsData.longitude, nullptr, &_gpsData.altitude, nullptr, nullptr, &_gpsData.accuracy, &_gpsData.time.tm_year, &_gpsData.time.tm_mon, &_gpsData.time.tm_mday, &_gpsData.time.tm_hour, &_gpsData.time.tm_min, &_gpsData.time.tm_sec)) {
      _gpsData.time.tm_year -= 1900;
      _gpsData.time.tm_mon -= 1;
      _gpsState = MODEM_GPS_SYNCED;
    }
#endif
    if (_gpsState == MODEM_GPS_SYNCED) {
      Mycila::Logger.info(TAG, "GPS Synced!");
      if (_state < MODEM_CONNECTING)
        _state = MODEM_CONNECTING;
    } else if (millis() - _gpsSyncStartTime >= _gpsSyncTimeout * 1000) {
      Mycila::Logger.error(TAG, "GPS Sync timeout!");
      _gpsState = MODEM_GPS_TIMEOUT;
      if (_state < MODEM_CONNECTING)
        _state = MODEM_CONNECTING;
    } else {
      _lastRefreshTime = millis();
    }
    refreshInfo = true;
  }

  if (_state == MODEM_CONNECTING) {
    Mycila::Logger.info(TAG, "Activate DATA...");
#ifdef TINY_GSM_MODEM_A7670
    _modem.gprsConnect(_apn.c_str());
#endif
#ifdef TINY_GSM_MODEM_SIM7080
    _modem.disableGPS(); // GPS is incompatible with networking
    _modem.setNetworkActive();
#endif
    _modem.waitForNetwork();
    _state = MODEM_READY;
    refreshInfo = true;
  }

  // refresh modem info
  if (refreshInfo || millis() - _lastRefreshTime >= 60000) {
    // time
    struct tm t = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (_timeState == ModemTimeState::MODEM_TIME_SYNCING && !_modem.getGSMDateTime(TinyGSMDateTimeFormat::DATE_FULL).startsWith("80/01/06") && _modem.getNetworkTime(&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec, nullptr)) {
      t.tm_year -= 1900;
      t.tm_mon -= 1;
      struct timeval now = {mktime(&t), 0};
      settimeofday(&now, nullptr);
      tzset();
      if (!Mycila::Time::getLocalStr().isEmpty())
        _timeState = ModemTimeState::MODEM_TIME_SYNCED;
    }

    // signal quality
    int16_t sq = _modem.getSignalQuality();
    _signal = sq >= 0 && sq <= 31 ? map(sq, 0, 31, 0, 100) : 0;

    _iccid = _modem.getSimCCID();
    _imei = _modem.getIMEI();
    _imsi = _modem.getIMSI();
    _localIP = _modem.getLocalIP();
    _model = _modem.getModemName();
    _operator = _modem.getOperator();

    _lastRefreshTime = millis();
  }

  // execute queued AT commands
  if (_commands.size()) {
    for (size_t i = 0; i < _commands.size(); i++) {
      Mycila::Logger.info(TAG, "Execute command: %s", _commands[i].c_str());
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

int Mycila::ModemClass::sendTCP(const String& host, uint16_t port, const String& payload, const uint16_t timeoutSec) {
  TinyGsmClient client(_modem);
  client.setTimeout(timeoutSec * 1000);
  int code = ESP_ERR_TIMEOUT;
  if (client.connect(host.c_str(), port, timeoutSec)) {
    client.print(payload);
    client.flush();
    client.stop();
    code = ESP_OK;
  }
  return code;
}

#ifdef TINY_GSM_MODEM_A7670
int Mycila::ModemClass::httpPOST(const String& url, const String& payload, const uint16_t timeoutSec) {
  if (url.isEmpty() || payload.isEmpty())
    return ESP_ERR_INVALID_ARG;

  if (!_modem.https_begin())
    return ESP_ERR_INVALID_STATE;

  if (!_modem.https_set_url(url.c_str()))
    return ESP_ERR_INVALID_ARG;

  _modem.https_set_timeout(timeoutSec, timeoutSec, timeoutSec);
  _modem.https_set_content_type("application/json");
  _modem.https_post(payload.c_str());

  return ESP_OK;
}
#endif

#ifdef TINY_GSM_MODEM_SIM7080
int Mycila::ModemClass::httpPOST(const String& url, const String& payload, const uint16_t timeoutSec) {
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
          protocol += url[i];
        break;
      case URLParseState::SEPERATOR:
        if (url[i] != '/') {
          state = HOST;
          host += url[i];
        }
        break;
      case URLParseState::HOST:
        if (url[i] == ':')
          state = PORT;
        else if (url[i] == '/')
          state = PATH;
        else
          host += url[i];
        break;
      case URLParseState::PORT:
        if (url[i] == '/')
          state = PATH;
        else
          port += url[i];
        break;
      case URLParseState::PATH:
        path += url[i];
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
    client.setTimeout(timeoutSec * 1000);
    if (client.connect(host.c_str(), httpsPort, timeoutSec)) {
      HttpClient http(client, host, httpsPort);
      http.setTimeout(timeoutSec * 1000);
      ret = http.post(path, "application/json", payload);
      http.stop();
    } else {
      ret = HTTP_ERROR_CONNECTION_FAILED;
    }

  } else if (protocol == "http") {
    const uint16_t httpPort = port.isEmpty() ? 80 : port.toInt();
    TinyGsmClient client(_modem);
    client.setTimeout(timeoutSec * 1000);
    if (client.connect(host.c_str(), httpPort, timeoutSec)) {
      HttpClient http(client, host, httpPort);
      http.setTimeout(timeoutSec * 1000);
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

namespace Mycila {
  ModemClass Modem;
} // namespace Mycila
