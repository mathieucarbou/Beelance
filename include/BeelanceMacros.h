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

#define KEY_HOSTNAME       "hostname"
#define KEY_ADMIN_PASSWORD "admin_pwd"
#define KEY_DEBUG_ENABLE   "debug_enable"

#define KEY_AP_MODE_ENABLE "ap_mode_enable"

#define KEY_WIFI_SSID               "wifi_ssid"
#define KEY_WIFI_PASSWORD           "wifi_pwd"
#define KEY_WIFI_CONNECTION_TIMEOUT "wifi_timeout"

#define KEY_CAPTURE_PORTAL_TIMEOUT "portal_timeout"

#define KEY_TEMPERATURE_ENABLE "sys_tmp_enable"
#define KEY_TEMPERATURE_PIN    "sys_tmp_pin"

#define KEY_SLEEP_ENABLE  "sleep_enable"

#define KEY_SEND_INTERVAL "send_delay"

#define KEY_BEEHIVE_NAME "bh_name"

// default settings

#ifndef BEELANCE_SERIAL_BAUDRATE
#define BEELANCE_SERIAL_BAUDRATE 115200
#endif

#ifndef BEELANCE_ADMIN_USERNAME
#define BEELANCE_ADMIN_USERNAME "admin"
#endif

#ifndef BEELANCE_DEBUG_ENABLE
#define BEELANCE_DEBUG_ENABLE false
#endif

#ifndef BEELANCE_AP_MODE_ENABLE
#define BEELANCE_AP_MODE_ENABLE false
#endif

// system

#ifndef BEELANCE_TEMPERATURE_ENABLE
#define BEELANCE_TEMPERATURE_ENABLE false
#endif

#ifndef BEELANCE_TEMPERATURE_PIN
#define BEELANCE_TEMPERATURE_PIN 21
#endif

#ifndef BEELANCE_SEND_INTERVAL
#define BEELANCE_SEND_INTERVAL 3600
#endif

#ifndef BEELANCE_TEMPERATURE_READ_INTERVAL
#define BEELANCE_TEMPERATURE_READ_INTERVAL 10
#endif

#ifndef BEELANCE_SLEEP_ENABLE
#define BEELANCE_SLEEP_ENABLE false
#endif

// lang

#ifndef BEELANCE_LANG
#define BEELANCE_LANG "en"
#endif
