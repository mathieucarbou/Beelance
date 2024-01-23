[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)
[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
[![Build](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml)
[![Download](https://img.shields.io/badge/Download-bin-green.svg)](https://github.com/mathieucarbou/Beelance/releases)
[![Doc](https://img.shields.io/badge/Doc-html-green.svg)](https://beelance.carbou.me/)

- [Beelance](#beelance)
- [Downloads](#downloads)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [How to build](#how-to-build)
  - [Connectivity provider](#connectivity-provider)
  - [Shopping list](#shopping-list)
  - [Wiring](#wiring)
- [Developer guide](#developer-guide)
  - [Project structure](#project-structure)
  - [Building and uploading the firmware](#building-and-uploading-the-firmware)
- [Related projects, guides and articles](#related-projects-guides-and-articles)
- [Sponsors and partners](#sponsors-and-partners)

# Beelance

Beelance comes from the French word "Balance" which means "weight scale".

Beelance is an autonomous and remotely connected weight scale for beehives 🐝.
It can monitor the weight of beehives and send the data to a remote server.
The communication can be done through LTE-M / NB-IoT / LTE / 4G.
LTE-M and NB-IOT are special communication frequencies for IoT devices which allows cellular coverage at more remote locations.

**Features:**

- Powered by Solar Panel
- Li-ion backup battery, recharged by solar panel
- CAT-M (LTE-M), NB-IOT, LTE, 4G connectivity
- Web interface with [ESP-DASH](https://docs.espdash.pro)
- OTA Web updates with [ElegantOTA](https://docs.elegantota.pro)
- Configuration backup and restore
- Work in AP Mode and WiFi mode
- Sleep mode to save battery
- DS18B20 Temperature sensor
- Supported boards:
  - [LILYGO® T-SIM7080G S3](https://www.lilygo.cc/products/t-sim7080-s3)
  - [LILYGO® T-A7670G R2 with GPS](https://www.lilygo.cc/products/t-sim-a7670e?variant=43043706077365)

# Downloads

Firmware downloads are available in the [Releases](https://github.com/mathieucarbou/Beelance/releases) page of the project.

Firmware files are named as follow:

- `Beelance-<VERSION>-<BOARD>.bin`: the firmware used to update through web interface
- `Beelance-<VERSION>-<BOARD>.factory.bin`: the firmware used to flash for the first time

Where:

- `VERSION`: version, or `main` for the latest development build
- `BOARD`: the board name

# Installation

First time, flash the entire firmware which includes the partition table and all partitions:

```bash
esptool.py --port /dev/ttyUSB0 \
  erase_flash

esptool.py --port /dev/ttyUSB0 \
  --chip esp32 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  --flash_mode dout \
  --flash_freq 40m \
  --flash_size detect \
  0x0 Beelance-VERSION-CHIP.factory.bin
```

Next time, just upload the firmware `Beelance-<VERSION>--<BOARD>.bin` through the web interface.

On Windows, you can use the official [ESP32 Flash Download Tool](https://www.espressif.com/en/support/download/other-tools).
Pay attention to set the flash address to 0.

- Install the SIM card in the board and power it.

# Configuration

- Connect to the WiFi access point `Beelance-XXXXXX` and open a web browser to `http://192.168.4.1`.
- The first tine, you need to choose between AP mode or connect to your Home WiFi (this can be changed later).
- Go to the configuration page (`http://192.168.4.1/config`) to setup your parameters and activate the components you want to use.
- Restart the device

**APN configuration**

- For Onomondo, the APN must be set to: `onomondo`
- For Things Mobile, the APN must be set to: `TM`

# Usage

- `http://192.168.4.1/`: main dashboard
- `http://192.168.4.1/update`: OTA Web Update page
- `http://192.168.4.1/config`: Configuration page (with backup and restore)
- `http://192.168.4.1/console`: Web Console to see logs
- `http://192.168.4.1/api/*`: API endpoints with more technical information about system, sensors and connectivity
  - `http://192.168.4.1/api/app`
  - `http://192.168.4.1/api/config`
  - `http://192.168.4.1/api/network`
  - `http://192.168.4.1/api/system`

# How to build

## Connectivity provider

You need to choose if you go with:

1. A IoT / M2M SIM card (with LILYGO® T-SIM7080G S3)

   - Pros: cheaper if you have several beehives
   - Cons: requires to get SIM card from the right IoT / M2M provider

2. A normal 4G SIM card from a traditional mobile operator (with LILYGO® T-A7670G R2 with GPS)

   - Pros: easier (i.e. in France a cheap Free Mobile SIM card could work)
   - Cons: won't scale with many beehives

**Which LTE-M or NB-IOT provider for France**

Here is a matrix of tested operators and boards:

| Provider                                      | T-SIM7080G-S3 | T-A7670G R2 |
| :-------------------------------------------- | :-----------: | :---------: |
| [Things Mobile](https://www.thingsmobile.com) |      ✅       |     ✅      |
| [Onomondo](https://onomondo.com)              |      ✅       |     ✅      |

Ref: https://github.com/Xinyuan-LilyGO/LilyGo-T-SIM7080G/issues/41

## Shopping list

**Hardware**

- LTE-M / NB-IOT: [LILYGO® T-SIM7080G S3](https://www.lilygo.cc/products/t-sim7080-s3) ([AliExpress](https://fr.aliexpress.com/item/1005005188988179.html)) - 42 euros
- LTE / 4G: [LILYGO® T-A7670G R2 with GPS](https://www.lilygo.cc/products/t-sim-a7670e?variant=43043706077365) ([AliExpress](https://fr.aliexpress.com/item/1005003036514769.html)) - 47 euros
- [DS18B20 Cable + DS18B20 Adapter](https://fr.aliexpress.com/item/4000143479592.html) - _SAMIORE Store_ - 3 euros
- [Solar Panels](https://fr.aliexpress.com/item/1005005509831452.html) - 33 euros for 2 items
- [4x Load Sensors](https://fr.aliexpress.com/item/1005005916651412.html) - _SAMIORE Store_ - 11 euros for 4 items
- [HX711](https://fr.aliexpress.com/item/33041823995.html) - _SAMIORE Store_ - 3 euros
- [Dupont Cable Kit](https://fr.aliexpress.com/item/1699285992.html) - _SAMIORE Store_ - 4 euros
- [Li-ion Battery 18650 3.7V 3200-3600mAh](https://www.amazon.fr/gp/product/B09DY1QVDW) - 15 euros for 2 items
- [Li-ion 18650 battery Charger](https://www.amazon.fr/gp/product/B08FDMGKMZ) - 16 euros

Alternatives:

- [Set of 4x Load Sensors + HX711](https://fr.aliexpress.com/item/1005004455387340.html) - _SAMIORE Store_ - 6 euros

**WARNINGS:**

1. Make sure to verify what you receive.
   The seller _SAMIORE Store_ is quite reliable: I often buy electronic stuff at this store.
   I already received broken items or unfinished items from some other sellers.

2. Verify the Solar Panel Voc: it should be within 4.4-6V.

## Wiring

- Connect the GPS antenna and LTE antenna to the T-SIM7080G S3
- Connect de DS18B20 adapter data to GPIO pin 21, and VCC (3.3V DC1) and GND

Please make sure to read the technical documentation regarding the Lilygo board at [https://github.com/Xinyuan-LilyGO/LilyGo-T-SIM7080G](https://github.com/Xinyuan-LilyGO/LilyGo-T-SIM7080G).

# Developer guide

Recommended IDE: [Visual Studio Code](https://code.visualstudio.com) with the [PlatformIO](https://platformio.org) extension.

## Project structure

- `.github`: CI/CD workflows
- `data`: Build components added to the firmware
- `include`: Firmware include code
- `lib`: Firmware libraries
- `pio`: pio scripts
- `src`: Firmware source code
- `platformio.ini`: PlatformIO configuration

## Building and uploading the firmware

```bash
pio run -t build
pio run -t upload
pio run -t monitor
```

# Related projects, guides and articles

- [The Internet of Bees: Adding Sensors to Monitor Hive Health](https://makezine.com/projects/bees-sensors-monitor-hive-health)
- [Balance Pour Surveiller Le Poids Des Ruches Pour Moins De 150€](https://miel-jura.com/balance-pour-surveiller-le-poids-des-ruches-pour-moi-de-150e/)
- [Beehive project issue](https://community.blynk.cc/t/beehive-project-issue/27245/48)
- [Balance de ruche à 30€ : épisode 1 - Fabrication du capteur](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-1-fabrication-du-capteur/)
- [Balance de ruche à 30€ : épisode 2 - L'électronique](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-2-lelectronique/)
- [Balance de ruche à 30€ : épisode 3 - L'étalonnage](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-3-letalonnage/)
- [Balance de ruche à 30€ : épisode 4 - Le programme](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-4-le-programme/)
- [Balance connectée](https://github.com/herve-tourrel/balance_connecte1)
- [Arduino Bathroom Scale With 50 Kg Load Cells and HX711 Amplifier](https://www.instructables.com/Arduino-Bathroom-Scale-With-50-Kg-Load-Cells-and-H/)
- [Powering Arduino Uno with Solar Cell](https://www.sensingthecity.com/powering-arduino-uno-with-solar-cell/)
- [Power ESP32/ESP8266 with Solar Panels (includes battery level monitoring)](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
- [Load Cell Amplifier HX711 Breakout Hookup Guide](https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide/all)

# Sponsors and partners

[![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/onomondo-community-2.png)](https://onomondo.com)
