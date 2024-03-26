[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)
[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
[![Build](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml)
[![Download](https://img.shields.io/badge/Download-bin-green.svg)](https://github.com/mathieucarbou/Beelance/releases)
[![Doc](https://img.shields.io/badge/Doc-html-green.svg)](https://beelance.carbou.me/)

- [Beelance](#beelance)
- [Downloads](#downloads)
- [How to build](#how-to-build)
  - [Connectivity provider](#connectivity-provider)
  - [Shopping list](#shopping-list)
  - [Wiring](#wiring)
- [How to install](#how-to-install)
  - [Firmware flash (first time)](#firmware-flash-first-time)
  - [SIM card](#sim-card)
  - [Configuration](#configuration)
- [How to use](#how-to-use)
  - [Dashboard](#dashboard)
  - [Update the firmware](#update-the-firmware)
  - [Configuration reset, backup and restore](#configuration-reset-backup-and-restore)
  - [Debugging and logging](#debugging-and-logging)
  - [Factory reset](#factory-reset)
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
- Battery voltage and level monitoring
- DS18B20 Temperature sensor
- Supported boards:
  - [LILYGO® T-SIM7080G S3 with GPS](https://www.lilygo.cc/products/t-sim7080-s3)
  - [LILYGO® T-A7670G R2 with GPS](https://www.lilygo.cc/products/t-sim-a7670e?variant=43043706077365)

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-dashboard-overview.png)

# Downloads

Firmware downloads are available in the [Releases](https://github.com/mathieucarbou/Beelance/releases) page of the project.

Firmware files are named as follow:

- `Beelance-<VERSION>-<BOARD>.bin`: the firmware used to update through web interface
- `Beelance-<VERSION>-<BOARD>.factory.bin`: the firmware used to flash for the first time

Where:

- `VERSION`: version, or `main` for the latest development build
- `BOARD`: the board name

# How to build

## Connectivity provider

You need to choose if you go with:

- A IoT / M2M SIM card (with `LILYGO® T-SIM7080G S3 with GPS`)
  - Cheaper if you have several beehives
  - Better coverage: LTE-M and NB-IoT are special optimized bandwidth for IoT devices
  - Requires to get SIM card from the right IoT / M2M provider
- A normal 4G SIM card from a traditional mobile operator (with `LILYGO® T-A7670G R2 with GPS`)
  - Easier to setup (i.e. in France a cheap Free Mobile SIM card could work)
  - Won't scale with many beehives, and can be more expensive

Here is a matrix of tested LTE-M / NB-IOT provider for France and boards:

| Provider                                      | T-SIM7080G-S3 | T-A7670G R2 |
| :-------------------------------------------- | :-----------: | :---------: |
| [Things Mobile](https://www.thingsmobile.com) |      ✅       |     ✅      |
| [Onomondo](https://onomondo.com)              |      ✅       |     ✅      |
| [Free Mobile](https://mobile.free.fr)         |      ❌       | Should work |

**[Onomondo](https://onomondo.com) is the recommended communication provider**.
It has been tested extensively during development and works very well with the `T-SIM7080G S3` board.
Onomondo also has accessible pricing, very good technical support, and provides connectors, a way to optimize the traffic in order to reduce cost even more.
This is the way to go if you have several beehives.
**Beelance supports Onomondo connectors**, which is a way to considerably reduce costly data traffic and decouple the device from the website collecting the data.

**[Things Mobile](https://www.thingsmobile.com)** has been tested during development and works well with the `T-SIM7080G S3` board.
Things Mobile should should cost around 1-2 euros per month per SIM card (beehive), but the solution is not scalable: it's not possible to order a batch of SIM cards, and each SIM card costs 5 euros to ship, 1.5 euros to activate, and the minimal balance to add on the platform is 30 euros.
Also, the platform is a mere web interface where we can manage SIM card. There is no connector API like Onomondo, and no details and logs about the connections and traffic.
This solution is good if you have a few connected beehives.

**[Free Mobile](https://mobile.free.fr)** provides cheap SIM cards at 2 euros (or free for people having a Freebox), with 200MB of data per month, which should be enough for a beehive.
Since it works on traditional frequencies (700 MHz band 28), it _should_ work with the `T-A7670G R2` board.

## Shopping list

**Hardware**

- Pick one board:
  - LTE-M / NB-IOT: [LILYGO® T-SIM7080G S3](https://www.lilygo.cc/products/t-sim7080-s3) ([AliExpress](https://fr.aliexpress.com/item/1005005188988179.html)) - 42 euros
  - LTE / 4G: [LILYGO® T-A7670G R2 with GPS](https://www.lilygo.cc/products/t-sim-a7670e?variant=43043706077365) ([AliExpress](https://fr.aliexpress.com/item/1005003036514769.html)) - 47 euros
- Temperature Sensor: [DS18B20 Cable + DS18B20 Adapter](https://fr.aliexpress.com/item/4000143479592.html) - _SAMIORE Store_ - 3 euros
- [Solar Panels](https://fr.aliexpress.com/item/1005005509831452.html) - 33 euros for 2 items
- Weight scale
  - [4x Load Sensors](https://fr.aliexpress.com/item/1005005916651412.html) - _SAMIORE Store_ - 11 euros for 4 items
  - [HX711](https://fr.aliexpress.com/item/33041823995.html) - _SAMIORE Store_ - 3 euros
  - Alternative: [Set of 4x Load Sensors + HX711](https://fr.aliexpress.com/item/1005004455387340.html) - _SAMIORE Store_ - 6 euros
- [Dupont Cable Kit](https://fr.aliexpress.com/item/1699285992.html) - _SAMIORE Store_ - 4 euros
- Battery & Charger:
  - [Li-ion Battery 18650 3.7V 3200-3600mAh](https://www.amazon.fr/gp/product/B09DY1QVDW) - 15 euros for 2 items
  - [Li-ion 18650 battery Charger](https://www.amazon.fr/gp/product/B08FDMGKMZ) - 16 euros
- Box: [Waterprood Electric Box about 150mm x 100mm x 60mm](https://www.amazon.fr/gp/product/B00GWPF840) - 8 euros
- Optional:
  - [Waterproof USB-C Sockets](https://www.amazon.fr/gp/product/B0BX37L2V3) - 9 euros for 5 items (facilitate charging and powering the device without opening the box)

**WARNINGS:**

1. Make sure to verify what you receive.
   The seller _SAMIORE Store_ is quite reliable: I often buy electronic stuff at this store.
   I already received broken items or unfinished items from some other sellers.

2. Verify the Solar Panel Voc: it should be within 4.4-6V.

## Wiring

- Connect the GPS antenna and LTE antenna to the module
- Connect de DS18B20 adapter (data to GPIO pin 21, VCC to 3.3V DC1, GND to GND on board)

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-1.jpeg) ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-2.jpeg)

# How to install

## Firmware flash (first time)

First time, flash the entire firmware which includes the partition table and all partitions.
Pick the right firmware for the right board.

1. Erase the flash

```bash
esptool.py --port /dev/ttyUSB0 \
  erase_flash
```

2. Flash the firmware

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

On Windows, you can use the official [ESP32 Flash Download Tool](https://www.espressif.com/en/support/download/other-tools).
Pay attention to set the flash address to 0.

## SIM card

Install the SIM card and power the board.

## Configuration

- Connect to the WiFi access point `Beelance-XXXXXX` and open a web browser to `http://192.168.4.1`.

- Go to the configuration page (`http://192.168.4.1/config`)

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-config.png)

- Set:

  - `admin_pwd`: The password to access the web interface
  - `bh_name`: The name for your beehive
  - `modem_apn`: The APN of your connectivity provider (`TM` for Things Mobile, `onomondo` for Onomondo)
  - `send_delay`: The time to sleep between each data send (in seconds). Default to 1 hour (3600 seconds)
  - `send_url`: **ONLY IF YOU DO NOT USE Onomondo**: The URL to send the data to (must be a HTTP or HTTPS POST endpoint that will receive a Json payload)
  - `sys_tmp_pin`: THe sensor temperature GPIO that you used (21 by default, but you can change it)

- Restart the device. It should:
  - Connect to the cellular network
  - Show the time and GPS location (if you are outside)

If you are not using Onomondo or want to setup your own website to receive the data, here here a sample of Json that the device will send to the configured URL:

```json
{
  "altitude": 7.400000095, // altitude == 0 if GPS fix failed
  "battery_charging": false, // true or false
  "battery_level": 90.79998016, // 0 if charging or not running on battery
  "battery_voltage": 4.107999802, // 0 if charging or not running on battery
  "beehive": "ruche_02",
  "boot": 852, // device boot count: useful to know if the device reboots often because of a bug
  "build": "3714812", // exact firmware build hash
  "device": "C01B0C", // ESP32 device ID
  "firmware": "Beelance-main-lilygo_t_a7670g-debug.bin", // firmware name that was installed
  "iccid": "89457300000014000000",
  "imei": "867284062000000",
  "latitude": 44.1234, // latitude == 0 if GPS fix failed
  "longitude": -2.1234, // longitude == 0 if GPS fix failed
  "modem": "A7670G-LLSE",
  "operator": "20801", // Name or code of current operator
  "temperature": 25.5, // temperature in Celsius, 0 if not activated
  "timestamp": "2024-03-26T14:49:50Z", // UTC timestamp
  "uptime": 119, // device uptime in seconds, useful to know if the device reboots often because of a bug
  "version": "v1.2.3", // firmware version
  "weight": 30 // weight in kg of the beehive. Not that the weight is not accurate because what is important is to track the evolution over time
}
```

**Important information about the Modem**

Connecting the first time with a new SIM card can take a very long time: first 30 seconds, the app tries oto automatically register the SIM with an operator.
Cellular communication goes through several steps:

1. Modem startup: can take up to 20 seconds
2. Network registration: can take up to 30 seconds
3. Network search: if step 2 fails can take up to 1 minute
4. Network registration for each search result until one succeeds (can take up to 30 seconds per operator tried)
5. GPS search: can take up to 1 minute, but this timeout can be configured in the configuration page (`gps_timeout`)

# How to use

## Dashboard

The Dashboard is located at `http://192.168.4.1/`.
For there you can see the device status and access some basic buttons.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-dashboard-overview.png)

- `Send now and sleep!`: will send the data now and go to sleep for the configured amount of time (`send_delay`) if sleep is not prevented
- `Scan Operators`: Trigger a new operator scan and new GPS search
- `Prevent Sleep`: Keeps the device active and prevents it from going to sleep
- `Debug Logging`: activate debug logging, which you can see in the console at `http://192.168.4.1/console`
- `Restart`: Restart the device

The statistics menu shows some metrics regarding the device itself.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-dashboard-sats.png)

Other pages are available:

- `http://192.168.4.1/update`: OTA Web Update page
- `http://192.168.4.1/config`: Configuration page (with backup and restore)
- `http://192.168.4.1/console`: Web Console to see logs
- `http://192.168.4.1/api/`: API endpoints with more technical information about system, sensors and connectivity
  - `http://192.168.4.1/api/app`
  - `http://192.168.4.1/api/beelance`
  - `http://192.168.4.1/api/config`
  - `http://192.168.4.1/api/network`
  - `http://192.168.4.1/api/system`

## Update the firmware

It is always recommended to update the firmware to the latest version.
Download the latest version on your mobile phone or computer, then connect to each device on their WiFi access point.
Then go to `http://192.168.4.1/update` and upload the firmware.
The device will automatically reboot after the upload.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-ota-update.png)

## Configuration reset, backup and restore

If you need to backup, restore or reset the configuration to factory settings, go to `http://192.168.4.1/config` and click on the corresponding button.

The same page can be used to change your configuration and some more advanced settings.

## Debugging and logging

Select `Debug Logging` in the Dashboard (`http://192.168.4.1/`).
This will activate debug logging and show the logs in the console at `http://192.168.4.1/console`.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-console.png)

**WARNING**: the console allows you to interact with the Modem internals (called `AT commands`).
**DO NOT SEND ANY DATA UNLESS YOU KNOW WHAT YOU ARE DOING.**

## Factory reset

To do a factory reset, proceed with the **Firmware flash (first time)** step.

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
