[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)
[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
[![Build](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml)
[![Download](https://img.shields.io/badge/Download-bin-green.svg)](https://github.com/mathieucarbou/Beelance/releases)
[![Doc](https://img.shields.io/badge/Doc-html-green.svg)](https://beelance.carbou.me/)

# Beelance

Beelance comes from the French word "Balance" which means "weight scale".

Beelance is an autonomous and remotely connected weight scale for beehives 🐝.
It can monitor the weight of beehives and send the data to a remote server.
The communication is done through LTE-M, a special communication frequency for IoT devices which allows cellular coverage for more remote locations.

**Features:**

- Powered by Solar Panel
- Li-ion backup battery, recharged by solar panel
- LTE-M connectivity with [Onomondo](https://onomondo.com)
- [LILYGO® T-SIM7080G S3](https://www.lilygo.cc/products/t-sim7080-s3)
- Load Cell and HX711 Amplifier
- [ESP-DASH](https://docs.espdash.pro) interface

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

# Usage

Connect to the WiFi access point `Beelance-XXXXXX` and open a web browser to `http://192.168.4.1`.

The home page is a dashboard showing the main metrics.

You can configure the application by going to `http://192.168.4.1/config`

You can update the firmware by going to `http://192.168.4.1/update`

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

# Related projects

- [The Internet of Bees: Adding Sensors to Monitor Hive Health](https://makezine.com/projects/bees-sensors-monitor-hive-health)
- [Balance Pour Surveiller Le Poids Des Ruches Pour Moins De 150€](https://miel-jura.com/balance-pour-surveiller-le-poids-des-ruches-pour-moi-de-150e/)
- [Beehive project issue](https://community.blynk.cc/t/beehive-project-issue/27245/48)
- [Balance de ruche à 30€ : épisode 1 - Fabrication du capteur](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-1-fabrication-du-capteur/)
- [Balance de ruche à 30€ : épisode 2 - L'électronique](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-2-lelectronique/)
- [Balance de ruche à 30€ : épisode 3 - L'étalonnage](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-3-letalonnage/)
- [Balance de ruche à 30€ : épisode 4 - Le programme](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-4-le-programme/)
- [Arduino Bathroom Scale With 50 Kg Load Cells and HX711 Amplifier](https://www.instructables.com/Arduino-Bathroom-Scale-With-50-Kg-Load-Cells-and-H/)
- [Powering Arduino Uno with Solar Cell](https://www.sensingthecity.com/powering-arduino-uno-with-solar-cell/)
- [Power ESP32/ESP8266 with Solar Panels (includes battery level monitoring)](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
