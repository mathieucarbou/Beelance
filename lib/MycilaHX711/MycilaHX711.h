// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#pragma once

#include "HX711.h"
#include <ArduinoJson.h>

#ifndef MYCILA_WEIGHT_EXPIRATION_DELAY
#define MYCILA_WEIGHT_EXPIRATION_DELAY 60
#endif

namespace Mycila {
  class HX711 {
    public:
      ~HX711() { end(); }

      void setOffset(int32_t offset) { _offset = offset; }
      void setScale(float scale) { _scale = scale; }
      void setExpirationDelay(uint32_t expirationDelay) { _expirationDelay = expirationDelay; }

      int32_t getOffset() const { return _offset; }
      float getScale() const { return _scale; }
      float getTare() const { return -_offset * _scale; }
      uint32_t getExpirationDelay() const { return _expirationDelay; }

      void begin(const uint8_t dataPin, const uint8_t clockPin);
      void end();

      float read();
      int32_t tare();
      float calibrate(float expectedWeight);

      float getWeight() const { return _weight; }
      bool isEnabled() const { return _enabled; }
      bool isValid() const { return _expirationDelay == 0 ? _enabled : (millis() - _lastUpdate < _expirationDelay * 1000); }
      uint32_t getLastUpdate() const { return _lastUpdate; }

      gpio_num_t getDataPin() const { return _dataPin; };
      gpio_num_t getClockPin() const { return _clockPin; };

      void toJson(const JsonObject& root) const;

    private:
      bool _enabled = false;
      gpio_num_t _dataPin = GPIO_NUM_NC;
      gpio_num_t _clockPin = GPIO_NUM_NC;
      float _weight = 0;
      uint32_t _lastUpdate = 0;
      uint32_t _expirationDelay = 0;
      int32_t _offset = 0;
      float _scale = 1.0;
      ::HX711 _hx711;
  };
} // namespace Mycila
