// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#include <StreamDebugger.h>
#include <TinyGsmClient.h>

#include <map>
#include <vector>

#ifndef MYCILA_MODEM_SERIAL
#define MYCILA_MODEM_SERIAL Serial1
#endif

#ifndef MYCILA_MODEM_RX_PIN
#define MYCILA_MODEM_RX_PIN RX1
#endif

#ifndef MYCILA_MODEM_TX_PIN
#define MYCILA_MODEM_TX_PIN TX1
#endif

#ifndef MYCILA_MODEM_PWR_PIN
#error "MYCILA_MODEM_PWR_PIN not defined"
#endif

#ifndef MYCILA_MODEM_REGISTER_TIMEOUT
#define MYCILA_MODEM_REGISTER_TIMEOUT 30000U
#endif

#ifndef MYCILA_MODEM_SEARCH_TIMEOUT
#define MYCILA_MODEM_SEARCH_TIMEOUT 60000U
#endif

namespace Mycila {

  typedef enum {
    MODEM_ERROR = 0,
    MODEM_OFF,
    MODEM_STARTING,
    MODEM_REGISTERING,
    MODEM_SEARCHING,
    MODEM_CONNECTING,
    MODEM_CONNECTED,
  } ModemState;

  typedef enum {
    MODEM_MODE_AUTO = 0,
    MODEM_MODE_LTE_M = 7,
    MODEM_MODE_NB_IOT = 9
  } ModemMode;

  typedef enum {
    MODEM_OPERATOR_UNKNOWN = 0,
    MODEM_OPERATOR_AVAILABLE = 1,
    MODEM_OPERATOR_CURRENT = 2,
    MODEM_OPERATOR_FORBIDDEN = 3,
  } ModemOperatorStatus;

  typedef struct {
      ModemOperatorStatus status;
      String name;  // operator name
      String code;  // operator code
      uint8_t mode; // technology access mode
  } ModemOperatorSearchResult;

  typedef struct {
      float latitude = 0;
      float longitude = 0;
      float altitude = 0;
      struct tm time = {0, 0, 0, 0, 0, 0, 0, 0, 0};
      float accuracy = 0;
  } ModemGPSData;

  class ModemClass {
    public:
      ModemClass();

      void begin();
      void loop();

      bool isConnected() const { return _state == MODEM_CONNECTED && !_localIP.isEmpty(); }
      bool isGPSSynced() const { return _gpsSynced; }
      bool isTimeSynced() const { return _timeSynced; }

      const ModemGPSData& getGPSData() const { return _gpsData; }
      const ModemOperatorSearchResult* getCandidate() const { return _candidate; }
      TinyGsm* getModem() { return &_modem; }
      ModemMode getMode() const { return _mode; }
      ModemState getState() const { return _state; }
      std::vector<ModemOperatorSearchResult> getDiscoveredOperators() const { return _operators; }

      String getAPN() const { return _apn; }
      String getBands(ModemMode mode) { return mode == MODEM_MODE_AUTO ? "" : _bands[mode]; }
      String getError() const { return _error; }
      String getICCID() const { return _iccid; }
      String getIMEI() const { return _imei; }
      String getIMSI() const { return _imsi; }
      String getLocalIP() const { return _localIP; }
      String getModel() const { return _model; }
      String getOperator() const { return _operator; }
      String getPIN() const { return _pin; }

      void setAPN(const String& apn) { _apn = apn; }
      void setBands(ModemMode mode, const String& bands) { _bands[mode] = bands; }
      void setDebug(bool debug);
      void setPIN(const String& pin) { _pin = pin; }
      void setPreferredMode(ModemMode mode) { _mode = mode; }

      template <typename... Args>
      void sendAT(Args... cmd) { _modem.sendAT(cmd...); }

      bool clearFPLMN();
      bool syncTime();
      bool syncGPS();
      bool registerWithOperatorCode(const String& code, uint8_t mode);
      bool deregisterFromOperator();
      void scanForOperators();

    private:
      // model and streams
      StreamDebugger _spy;
      TinyGsm _modem;

    private:
      // debug
      String _readBuffer;
      String _writeBuffer;

    private:
      // GPS and Time
      bool _gpsSynced = false;
      bool _timeSynced = false;
      ModemGPSData _gpsData;

    private:
      // Modem info
      String _iccid;
      String _imei;
      String _imsi;
      String _localIP;
      String _model;
      String _operator;

    private:
      // Modem settings
      ModemMode _mode = MODEM_MODE_AUTO;
      String _apn;
      String _pin;
      std::map<ModemMode, String> _bands = {
        {MODEM_MODE_LTE_M, "1,2,3,4,5,8,12,13,14,18,19,20,25,26,2 7,28,66,85"},
        {MODEM_MODE_NB_IOT, "1,2,3,4,5,8,12,13,18,19,20,25,26,28,6 6,71,85"},
      };

    private:
      // Operator search and registration
      int _candidateIndex = 0;
      ModemOperatorSearchResult* _candidate = nullptr;
      std::vector<ModemOperatorSearchResult> _operators;
      uint32_t _since = 0;

    private:
      // state machine
      ModemState _state = MODEM_OFF;
      String _error;

    private:
      // utilities
      void _onRead(const uint8_t* buffer, size_t size);
      void _onWrite(const uint8_t* buffer, size_t size);
      void _readDrop();
      void _setMode(uint8_t mode);

    private:
      // handle modem state machine
      void _start();
      void _register();
      void _search();
      void _connect();
      void _loopStarting();
      void _loopRegistering();
      void _loopSearching();
      void _loopConnecting();
  };

  extern ModemClass Modem;
} // namespace Mycila
