// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <MycilaHX711.h>
#include <MycilaLogger.h>

#define TAG "HX711"

#ifndef GPIO_IS_VALID_GPIO
#define GPIO_IS_VALID_GPIO(gpio_num) ((gpio_num >= 0) && \
                                      (((1ULL << (gpio_num)) & SOC_GPIO_VALID_GPIO_MASK) != 0))
#endif

extern Mycila::Logger logger;

void Mycila::HX711::begin(const uint8_t dataPin, const uint8_t clockPin) {
  if (_enabled)
    return;

  if (GPIO_IS_VALID_GPIO(dataPin)) {
    _dataPin = (gpio_num_t)dataPin;
  } else {
    logger.error(TAG, "Disable HX711: Invalid DATA pin: %u", dataPin);
    _dataPin = GPIO_NUM_NC;
    return;
  }

  if (GPIO_IS_VALID_GPIO(clockPin)) {
    _clockPin = (gpio_num_t)clockPin;
  } else {
    logger.error(TAG, "Disable HX711: Invalid CLOCK pin: %u", clockPin);
    _clockPin = GPIO_NUM_NC;
    return;
  }

  _hx711.begin(_dataPin, _clockPin, true);

  logger.info(TAG, "Enable HX711...");

  _enabled = true;
}

void Mycila::HX711::end() {
  if (_enabled) {
    logger.info(TAG, "Disable HX711...");
    _enabled = false;
    _weight = 0;
    _lastUpdate = 0;
    _dataPin = GPIO_NUM_NC;
    _clockPin = GPIO_NUM_NC;
  }
}

float Mycila::HX711::read() {
  if (!_enabled)
    return 0;
#ifdef MYCILA_SIMULATION
  _weight = random(15, 30) * 1000;
#else
  float f = (_hx711.read_average(30) - _offset) * _scale;
  _weight = f < 0 ? 0 : f;
#endif
  _lastUpdate = millis();
  return _weight;
}

int32_t Mycila::HX711::tare() {
  if (!_enabled)
    return _offset;
  _scale = 1;
  _offset = _hx711.read_average(30);
  return _offset;
}

float Mycila::HX711::calibrate(float expectedWeight) {
  if (!_enabled)
    return _scale;
  _scale = expectedWeight / (_hx711.read_average(30) - _offset);
  return _scale;
}

void Mycila::HX711::toJson(const JsonObject& root) const {
  root["enabled"] = _enabled;
  root["offset"] = _offset;
  root["scale"] = _scale;
  root["tare"] = getTare();
  root["valid"] = isValid();
  root["weight"] = _weight;
}
