// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#pragma once

#define BEELANCE_TRUE     "true"
#define BEELANCE_FALSE    "false"
#define BEELANCE_ON       "on"
#define BEELANCE_OFF      "off"
#define BEELANCE_BOOL(a)  ((a) ? BEELANCE_TRUE : BEELANCE_FALSE)
#define BEELANCE_STATE(a) ((a) ? BEELANCE_ON : BEELANCE_OFF)

// configuration keys

#define KEY_ADMIN_PASSWORD          "admin_pwd"
#define KEY_AP_MODE_ENABLE          "ap_mode_enable"
#define KEY_BEEHIVE_NAME            "bh_name"
#define KEY_CAPTURE_PORTAL_TIMEOUT  "portal_timeout"
#define KEY_DEBUG_ENABLE            "debug_enable"
#define KEY_HOSTNAME                "hostname"
#define KEY_HX711_CLOCK_PIN         "hx711_clk_pin"
#define KEY_HX711_DATA_PIN          "hx711_dt_pin"
#define KEY_HX711_OFFSET            "hx711_offset"
#define KEY_HX711_SCALE             "hx711_scale"
#define KEY_MODEM_APN               "modem_apn"
#define KEY_MODEM_BANDS_LTE_M       "bands_ltem"
#define KEY_MODEM_BANDS_NB_IOT      "bands_nbiot"
#define KEY_MODEM_GPS_SYNC_TIMEOUT  "gps_timeout"
#define KEY_MODEM_MODE              "modem_mode"
#define KEY_MODEM_PIN               "modem_pin"
#define KEY_NIGHT_START_TIME        "night_start"
#define KEY_NIGHT_STOP_TIME         "night_end"
#define KEY_NO_SLEEP_ENABLE         "no_sleep_enable"
#define KEY_PMU_CHARGING_CURRENT    "pmu_chg_current"
#define KEY_SEND_INTERVAL           "send_delay"
#define KEY_SEND_URL                "send_url"
#define KEY_TEMPERATURE_PIN         "temp_pin"
#define KEY_TIMEZONE_INFO           "tz_info"
#define KEY_WIFI_CONNECTION_TIMEOUT "wifi_timeout"
#define KEY_WIFI_PASSWORD           "wifi_pwd"
#define KEY_WIFI_SSID               "wifi_ssid"

// default settings

#ifndef BEELANCE_SERIAL_BAUDRATE
#define BEELANCE_SERIAL_BAUDRATE 115200
#endif

#ifndef BEELANCE_ADMIN_USERNAME
#define BEELANCE_ADMIN_USERNAME "admin"
#endif

#ifndef BEELANCE_TEMPERATURE_PIN
#define BEELANCE_TEMPERATURE_PIN 12
#endif

#ifndef BEELANCE_MODEM_TASK_STACK_SIZE
#define BEELANCE_MODEM_TASK_STACK_SIZE 256 * 21
#endif

#ifndef BEELANCE_HX711_TASK_STACK_SIZE
#define BEELANCE_HX711_TASK_STACK_SIZE 256 * 12
#endif

#ifndef BEELANCE_MIN_SEND_DELAY
#define BEELANCE_MIN_SEND_DELAY 1800
#endif

#ifndef BEELANCE_HX711_CLOCK_PIN
#define BEELANCE_HX711_CLOCK_PIN 13
#endif

#ifndef BEELANCE_HX711_DATA_PIN
#define BEELANCE_HX711_DATA_PIN 14
#endif

#ifndef BEELANCE_MAX_HISTORY_SIZE
#define BEELANCE_MAX_HISTORY_SIZE 10
#endif

#define FILE_HISTORY "/history.json"
