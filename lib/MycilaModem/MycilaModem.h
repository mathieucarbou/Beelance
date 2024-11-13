// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
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

#ifndef MYCILA_MODEM_GPS_SYNC_TIMEOUT
#define MYCILA_MODEM_GPS_SYNC_TIMEOUT 90
#endif

#ifndef MYCILA_MODEM_CONNECT_TIMEOUT
#define MYCILA_MODEM_CONNECT_TIMEOUT 20
#endif

#ifndef MYCILA_MODEM_PWR_PIN
#error "MYCILA_MODEM_PWR_PIN not defined"
#endif

namespace Mycila {

  typedef enum {
    MODEM_ERROR = 0,
    MODEM_OFF,
    MODEM_STARTING,
    MODEM_WAIT_REGISTRATION,
    MODEM_SEARCHING,
    MODEM_GPS,
    MODEM_CONNECTING,
    MODEM_READY,
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
  } ModemOperatorState;

  typedef enum {
    MODEM_TIME_OFF = 1,
    MODEM_TIME_SYNCING = 2,
    MODEM_TIME_SYNCED = 3,
  } ModemTimeState;

  typedef enum {
    MODEM_GPS_OFF = 0,
    MODEM_GPS_SYNCING = 1,
    MODEM_GPS_SYNCED = 2,
    MODEM_GPS_TIMEOUT = 3,
  } ModemGPSState;

  typedef struct {
      ModemOperatorState state;
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

  typedef std::function<void(ModemState state)> ModemStateChangeCallback;

  class ModemClass {
    public:
      ModemClass();

      void begin();
      void loop();

      bool isReady() const { return _state == MODEM_READY; }
      bool isGPSSynced() const { return _gpsState == ModemGPSState::MODEM_GPS_SYNCED; }
      bool isTimeSynced() const { return _timeState == ModemTimeState::MODEM_TIME_SYNCED; }

      const ModemGPSData& getGPSData() const { return _gpsData; }
      const ModemOperatorSearchResult* getCandidate() const { return _candidate; }
      TinyGsm* getModem() { return &_modem; }
      ModemMode getMode() const { return _mode; }
      ModemState getState() const { return _state; }
      ModemTimeState getTimeState() const { return _timeState; }
      ModemGPSState getGPSState() const { return _gpsState; }
      std::vector<ModemOperatorSearchResult> getDiscoveredOperators() const { return _operators; }

      String getAPN() const { return _apn; }
      String getTimeZoneInfo() const { return _timeZoneInfo; }
      String getBands(ModemMode mode) { return mode == MODEM_MODE_AUTO ? "" : _bands[mode]; }
      String getError() const { return _error; }
      String getICCID() const { return _iccid; }
      String getIMEI() const { return _imei; }
      String getIMSI() const { return _imsi; }
      String getLocalIP() const { return _localIP; }
      String getModel() const { return _model; }
      String getOperator() const { return _operator; }
      String getPIN() const { return _pin; }
      // 0-100%
      uint8_t getSignalQuality() const { return _signal; }

      void setAPN(const String& apn) { _apn = apn; }
      void setTimeZoneInfo(const String& timeZoneInfo) { _timeZoneInfo = timeZoneInfo; }
      void setBands(ModemMode mode, const String& bands) { _bands[mode] = bands; }
      void setDebug(bool debug);
      void setPIN(const String& pin) { _pin = pin; }
      void setPreferredMode(ModemMode mode) { _mode = mode; }
      void setGpsSyncTimeout(uint32_t timeoutSec) { _gpsSyncTimeout = timeoutSec; }
      void setCallback(ModemStateChangeCallback callback) { _callback = callback; }

      void enqueueAT(const char* cmd) { _commands.push_back(cmd); }
      void scanForOperators() { _state = MODEM_SEARCHING; }
      void powerOff();
      bool activateData();
      void activateGPS();

      // Returns ESP_OK or ESP_ERR_TIMEOUT if connection times out
      int sendTCP(const String& host, uint16_t port, const String& payload, const uint16_t connectTimeoutSec = MYCILA_MODEM_CONNECT_TIMEOUT);

      // Returns ESP_OK or ESP_ERR_TIMEOUT if connection times out
      int httpPOST(const String& url, const String& payload, const uint16_t connectTimeoutSec = MYCILA_MODEM_CONNECT_TIMEOUT);

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
      ModemTimeState _timeState = MODEM_TIME_OFF;
      ModemGPSState _gpsState = MODEM_GPS_OFF;
      uint32_t _gpsSyncStartTime = 0;
      ModemGPSData _gpsData;
      String _timeZoneInfo = "UTC0";

    private:
      // Modem info
      String _iccid;
      String _imei;
      String _imsi;
      String _localIP;
      String _model;
      String _operator;
      uint8_t _signal;

    private:
      // Modem settings
      ModemMode _mode = MODEM_MODE_AUTO;
      String _apn;
      String _pin;
      uint32_t _gpsSyncTimeout = MYCILA_MODEM_GPS_SYNC_TIMEOUT;
      std::map<ModemMode, String> _bands = {
        {MODEM_MODE_LTE_M, "1,2,3,4,5,8,12,13,14,18,19,20,25,26,2 7,28,66,85"},
        {MODEM_MODE_NB_IOT, "1,2,3,4,5,8,12,13,18,19,20,25,26,28,6 6,71,85"},
      };

    private:
      // Operator search and registration
      int _candidateIndex = -1;
      int _registrationCheckCount = 7;
      uint32_t _registrationCheckLastTime = 0;
      ModemOperatorSearchResult* _candidate = nullptr;
      std::vector<ModemOperatorSearchResult> _operators;

    private:
      // state machine
      ModemState _state = MODEM_OFF;
      ModemStateChangeCallback _callback = nullptr;
      String _error;
      uint32_t _lastRefreshTime = 0;
      std::vector<String> _commands;

    private:
      // utilities
      void _onRead(const uint8_t* buffer, size_t size);
      void _onWrite(const uint8_t* buffer, size_t size);
      void _readDrop();
      void _setMode(uint8_t mode);
      void _setState(ModemState state);
      bool _syncGPS();
      bool _syncTime();
      void _syncInfo();
      void _sync();
      void _dequeueATCommands();
      void _powerModem();
  };

  extern ModemClass Modem;
} // namespace Mycila
