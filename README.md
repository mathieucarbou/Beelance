[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)
[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
[![Build](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/mathieucarbou/Beelance/actions/workflows/build.yml)
[![Download](https://img.shields.io/badge/Download-bin-green.svg)](https://github.com/mathieucarbou/Beelance/releases)
[![Doc](https://img.shields.io/badge/Doc-html-green.svg)](https://beelance.carbou.me/)

- [Beelance](#beelance)
  - [Overview](#overview)
  - [Features](#features)
    - [Powering](#powering)
    - [Communication](#communication)
    - [Measurements](#measurements)
    - [Configuration](#configuration)
    - [Integrations](#integrations)
  - [Downloads](#downloads)
- [How to build](#how-to-build)
  - [Connectivity provider](#connectivity-provider)
  - [Shopping list](#shopping-list)
  - [Wiring](#wiring)
    - [Default GPIO Pinout](#default-gpio-pinout)
  - [Assembly](#assembly)
- [How to install](#how-to-install)
  - [SIM card](#sim-card)
  - [Powering](#powering-1)
  - [Firmware flash (first time)](#firmware-flash-first-time)
  - [Device Configuration](#device-configuration)
    - [Important information about the Modem](#important-information-about-the-modem)
  - [Weight Scale Calibration](#weight-scale-calibration)
    - [Temperature effect](#temperature-effect)
    - [Offset Calibration](#offset-calibration)
    - [Scale Calibration](#scale-calibration)
    - [Under the hood](#under-the-hood)
  - [Receiving the data](#receiving-the-data)
    - [JSON Payload](#json-payload)
    - [IFTTT Integration](#ifttt-integration)
- [How to use](#how-to-use)
  - [Dashboard](#dashboard)
  - [Update the firmware](#update-the-firmware)
  - [Configuration reset, backup and restore](#configuration-reset-backup-and-restore)
  - [Debugging and logging](#debugging-and-logging)
- [Developer guide](#developer-guide)
  - [Project structure](#project-structure)
  - [Building and uploading the firmware](#building-and-uploading-the-firmware)
- [Contact](#contact)
- [Sponsors and partners](#sponsors-and-partners)
- [Related projects, guides and articles](#related-projects-guides-and-articles)

# Beelance

## Overview

Beelance comes from the French word "Balance" which means "weight scale".

Beelance is an autonomous and remotely connected weight scale for beehives 🐝.
It can monitor the weight and temperature of beehives and send the data to a remote server or a Google Sheet (through IFTTT or alternative).
Beelance supports 2 types of communication:

- either specific frequencies optimized for IoT devices (long-range, low bandwidth), called LTE-M and NB-IoT (especially useful for remote areas)
- or normal cellular phone frequencies with a normal SIM card using LTE / 4G

Device interface:

[![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-dashboard-overview.png)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-dashboard-overview.png)

Example of Google Sheet receiving the data:

[![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-3.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-3.jpeg)

## Features

### Powering

Beelance can be powered by USB-C, Solar Panel or battery.
On the field, it is recommended to use a Solar Panel to power the device.
If a battery is used, the device will automatically switch to the battery when the solar panel is not providing enough power.
Otherwise, the battery will be charged by the solar panel at the same time the solar panel powers the device.

Battery type: 3.7V 18650 LFP or Li-ion

### Communication

Supported frequencies: CAT-M (LTE-M), NB-IOT, LTE, 4G

Normal cellular SIM cards or specialized M2M (IoT) SIM cards can be used.

### Measurements

The device takes measurements and sends them periodically:

- Temperature (DS18B20 Temperature sensor)
- Weight of the beehive (with 4 load sensors and HX711 amplifier)
- GPS location and altitude
- Time
- Battery level and voltage
- Operator and SIM card information

It will also keep a local history of the 10 last measurements, the maximum values of the last 10 hours and the maximum values of the last 10 days, both for weight and temperature.

### Configuration

The device has an access point, we can connect to it to:

- Access its dashboard to view the metrics and manage the device
- Update the firmware
- Backup and restore its configuration
- Configure the device:
  - Beelance (beehive) name
  - Time between each measurement push
  - The URL to send the data to (optional)
  - The APN of the connectivity provider
  - The night hours to avoid sending data at night
  - Activate Power saving mode (deep sleep between each push)
  - etc

### Integrations

The device can be configured to send its data to a remove server of your choice.
It can also be an [IFTTT](https://ifttt.com) webhook, which can then insert the data in a Google Sheet.

## Downloads

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
  - LTE-M / NB-IOT: [LILYGO® T-SIM7080G S3 with GPS](https://www.lilygo.cc/products/t-sim7080-s3) ([AliExpress](https://fr.aliexpress.com/item/1005005188988179.html)) - 42 euros
  - LTE / 4G: [LILYGO® T-A7670G R2 with GPS](https://www.lilygo.cc/products/t-sim-a7670e?variant=43043706077365) ([AliExpress](https://fr.aliexpress.com/item/1005003036514769.html)) - 47 euros
- Temperature Sensor: [DS18B20 Cable + DS18B20 Adapter](https://fr.aliexpress.com/item/4000143479592.html) - 3 euros - _SAMIORE Store_
- [Solar Panels](https://fr.aliexpress.com/item/1005005509831452.html) - 33 euros for 2 items
- Weight scale: [Set of 4x Load Sensors + HX711](https://fr.aliexpress.com/item/1005004455387340.html) - 6 euros - _SAMIORE Store_
- [Dupont Cable Kit](https://fr.aliexpress.com/item/1699285992.html) - 4 euros - _SAMIORE Store_
- One box:
  - [IP55 Box about 150mm x 100mm x 60mm](https://www.amazon.fr/gp/product/B00GWPF840) - 8 euros
  - [IP67 Box about 200mm x 120mm x 75mm](https://fr.aliexpress.com/item/1005001304761174.html) - 11 euros
  - If using a battery, take a UL94V-0 box (fire retardant)
- Optional but recommended:
  - [Waterproof USB-C Sockets](https://www.amazon.fr/gp/product/B0BX37L2V3) - 9 euros for 5 items - _Facilitate charging and powering the device without opening the box_
  - Battery (18650): [Li-ion Battery 18650 3.7V 3200-3600mAh](https://www.amazon.fr/gp/product/B09DY1QVDW) - 15 euros for 2 items
  - Charger: [Li-ion 18650 battery Charger](https://www.amazon.fr/gp/product/B08FDMGKMZ) - 16 euros
  - [Expandable braided sleeving](https://www.amazon.fr/gp/product/B0B3RBS4DX) to protect small wires
  - [Waterproof connectors](https://fr.aliexpress.com/item/1005003084329744.html) - only if you want to go further in the perfection
  - [3D printed supports for load sensors](https://www.thingiverse.com/thing:4869785), or you can make some in metal or wood yourself

**WARNINGS:**

1. Make sure to verify what you receive.
   The seller _SAMIORE Store_ is quite reliable: I often buy electronic stuff at this store.
   I already received broken items or unfinished items from some other sellers.

2. Verify the Solar Panel Voc: **it should be within 4.4-6V.**

## Wiring

- Connect the GPS antenna and LTE antenna to the module
- GPS antenna must be placed on a flat surface, in the same position as in the photo (I decided to put it on the board with a dual face tape)
- Connect de DS18B20 adapter (see GPIO pins table below) and the temperature sensor
- Connect the HX11 adapter (see GPIO pins table below)
- Wire the weight cells to the adapter according to the schemas and photos below. Please also see this more [complete guide also](https://www.instructables.com/Arduino-Bathroom-Scale-With-50-Kg-Load-Cells-and-H/).
- Solar Panel must be connected to USB-C port: this will automatically boot the device when the solar panel are delivering enough power
- Optionally wire an external USB-C connector on the box, connected to the board like the photo (to charge the battery or power the board more easily)

### Default GPIO Pinout

|                         | T-SIM7080G-S3 Pin | T-A7670G R2 Pin |
| :---------------------- | :---------------: | :-------------: |
| Temperature Sensor DATA |        12         |       15        |
| Temperature Sensor VCC  |       3.3V        |      3.3V       |
| Temperature Sensor GND  |        GND        |       GND       |
| HX711 VCC               |       3.3V        |      3.3V       |
| HX711 SCK (CLOCK)       |        13         |       13        |
| HX711 DT (DATA)         |        14         |       14        |
| HX711 GND               |        GND        |       GND       |

**How to wire weight cells**

| [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cell-wiring.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cell-wiring.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cell-testing.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cell-testing.jpeg) |

## Assembly

**The box and solar panel:**

| [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-2.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-2.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-4.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-4.jpeg) |

**The weight sensors with the 3D printed enclosures:**

| [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cell-usage.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cell-usage.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-5.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-5.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-6.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-6.jpeg) |

**Calibration**

| [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-7.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-7.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-8.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-8.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-9.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/build-9.jpeg) |

# How to install

## SIM card

Do not forget to install the SIM card before powering.

## Powering

- Install the battery in the board (optional)
- Connect the solar panel to USB-C port
- Set the board switch to `ON`
- If the solar panel is delivering enough power, the device will boot automatically
- On the `T-SIM7080G-S3`, to start from battery, you need to press 3 seconds on the power button

[![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/lilygo-power.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/lilygo-power.jpeg)

**WARNING / DISCLAIMER**:

**_Leaving a device in nature with a battery is a potential hazard, especially with Li-ion batteries which are not supposed to be used above 60 degrees._**
**_Please make sure you are informed with the associated risks with Li-ion or LFP batteries._**
**_If you are not sure about the safety of the device, do not leave it unattended or without any monitoring, or do not install any battery._**
**_In any case, use an appropriate box which is fire retardant (UL94V-0) and waterproof._**
**_The project's authors decline any responsibility regarding the use and assembly of this project._**
**_You are entirely responsible of any loss, damage or injury caused by the use of this project._**

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
**Pay attention to first erase and then flash the entire memory by setting the flash address to 0**.

## Device Configuration

- Connect to the WiFi access point `Beelance-XXXXXX` and open a web browser to `http://192.168.4.1`. This is the dashboard of the device.

- Go to the configuration page (`http://192.168.4.1/config`)

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-config.png)

- Set the following **mandatory** settings:

  - `modem_apn`: The APN of your connectivity provider (`TM` for Things Mobile, `onomondo` for Onomondo)
  - `send_url` (**ONLY IF YOU DO NOT USE an Onomondo Connector**): The URL to send the data to.
    Must be an HTTP endpoint that will receive a Json payload, like an IFTTT webhook.
    HTTPS sometimes work, but is not recommended because https calls from a little device like that take a lot of memory, CPU, are slower and have a larger payload.
    So it will cost more money and use more battery power.

- Optionally change the following settings:

  - `admin_pwd`: The password to access the web interface (optional, but recommended)
  - `bh_name`: A custom name for your beehive
  - `send_delay`: The time to pause between each data send (in seconds). Default to 1 hour (3600 seconds) and it is not possible to go below 1800 seconds (30 minutes)
  - `night_start`: Format: `HH:MM`. Defines the start of the night, when no data is sent
  - `night_end`: Format: `HH:MM`. Defines the end of the night, when data is sent periodically

In the main dashboard, click to restart the device and wait to makes sure it becomes ready (modem ready, time and GPS fix, etc.).
You can also go to the console at `http://192.168.4.1/console` to see the logs, and if you really want to see verbose logging, you can activate debug logs in he console.

Once your testing is finished and when installed, you can deactivate the option `Prevent Sleep On Battery`: when disabled, the device will be allowed to go into deep sleep after it has sent the data, and when it is powered by battery.
When the device is powered by external power like USB-C or Solar Panel, it will not go to sleep.
If the device is in deep sleep and you need to access it, just power it from USB-C and press the reset button to restart it, or disconnect power and restart it.

### Important information about the Modem

Connecting the first time with a new SIM card can take a very long time: first 30 seconds, the app tries to automatically register the SIM with an operator.
Cellular communication goes through several steps:

1. Modem startup: can take up to 20 seconds
2. Network registration: can take up to 30 seconds
3. Network search: if step 2 fails can take up to 1 minute
4. Try network registration for each search result until one succeeds (can take up to 30 seconds per operator tried)
5. GPS search: can take up to 1 minute (timeout can be configured in the configuration page with `gps_timeout`)

The device includes a watchdog for all these steps: it will automatically restart if it becomes locked in a state for 5 minutes.

If the device becomes disconnected (loss of Modem IP address), it will try to reconnect automatically.

## Weight Scale Calibration

The weight cells are not calibrated by default because each setup is different and will require different calibration values (called `offset` and `scale`).

When the device is installed and powered on, you will see some weird values for the weight, which can even be negative:

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-1.jpeg)

Follow the steps below to calibrate...

### Temperature effect

**This is important that you run the calibration at a time of day when you want to see more accurate results.**

The temperature has an effect on the load: for example, during the night, the load will be higher because the bees are in the beehive and there is also more humidity in the air and wood.
Under a steady bright sun, the temperature is increasing and the load is heating / drying, so the weight will decrease.
Here is an example of a 19.8 kg load put on the weight scale for several hours under the sun.
You can see that the more the temperature is rising, the more the weight is decreasing.
So you need to take this effect in consideration when looking at the statistics.
You will still have the same effect whether you include the beehive in your calibration tare or not.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-7.jpeg)

Also, the temperature rising has an opposite effect on the metal of the weight cells themselves: the more the temperature is rising, the more the metal is expanding and the weight is increasing.
But this effect is negligible compared to the effect of the bees, honey and humidity.

### Offset Calibration

**The first step is to calibrate your `zero` weight (which is called `tare`).**
You can decide to do it with or without the beehive on the scale, depending if you want the reported weight to include the weight of the beehive or not..
Press the `Tare` button to do so.

| ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-offset.jpeg) | ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-2.jpeg) | ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-3.jpeg) |

Once pressed, the weight should become close to `zero`.
You can press the `Tare` button again to find a more accurate `zero` weight, but note that there will still be some inaccuracy of about 100 grams.
If you end up with a weight which is changing between -50 and 50 grams, this is pretty good.

### Scale Calibration

Once the baseline is set, **the second step is to calibrate the scale.**
This ensures to get the right weight in grams when there is a load on the scale.
To do that, place on the setup (on the beehive or on the scale) a load of about 20 kg, like a big bag of sugar or anything else.

**This is important that:**

1. You know the exact weight of the big bag of sugar.
2. The load represents the load you will measure.
   For example, if you have calibrated the baseline with your beehive, you will only measure the bees and honey.
   So the weight will be lower compared to if you had calibrated the baseline without the beehive, and then put the beehive on the scale later.

When placing the load on the scale, the numbers will change to something strange and could even be negative.
For example, I've put a total of 11.6 kg on the scale and here is what I see:

| ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-offset.jpeg) | ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-4.jpeg) |

The next step is to move the cursor to place it to the exact value of the load you have put on the scale.
For example in my case I set the cursor to `11600 g`.

| ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-scale.jpeg) | ![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-6.jpeg) |

Once the cursor is set to the right position, after 2 seconds, the device will resume the measurements and you will see the weight changing around the calibrated value.
If it becomes too high or too low, move the cursor again to the expected weight value.

### Under the hood

The `statistics` section of the dashboard will show you the calibrated offset and scale values.
The tare value represents the weight that is not accounted for in the weight calculation.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-stats.jpeg)

The `offset` and `scale` values are saved on the device in the configuration, which can be backed up and restored and accessed in the configuration page at `http://192.168.4.1/config`.
You can even manually adjust these values.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/cal-conf.jpeg)

## Receiving the data

### JSON Payload

Here is below a sample of the JSON payload that the device will send to the configured URL in a `POST` request.

```json
{
  "ts": 1712578315,
  "bh": "Ruche-01",
  "temp": 24.25,
  "wt": 15000,
  "lat": 44.1234,
  "long": -2.1234,
  "alt": 7.4,
  "sim": "89457300000014000000",
  "op": "20801",
  "dev": "73FADC",
  "boot": 1358,
  "ver": "v1.2.3",
  "up": 10,
  "pow": "bat",
  "bat": 93,
  "volt": 4.13
}
```

- `ts`: the UTC timestamp as [Unix time](https://en.wikipedia.org/wiki/Unix_time) in seconds. Most systems support such timestamp. I.e. in Javascript: `d=new Date(); d.setTime(ts*1000);`
- `bh`: the name of the beehive
- `temp`: the temperature in Celsius, 0 if not activated
- `wt`: the weight **in grams** of the beehive. Note that the weight is not accurate because what is important is to track the evolution over time
- `lat`: the latitude, 0 if GPS fix failed
- `long`: the longitude, 0 if GPS fix failed
- `alt`: the altitude in meters, 0 if GPS fix failed
- `sim`: the SIM ID (ICCID)
- `op`: the name or code of the current operator
- `dev`: the ESP32 device ID
- `boot`: the device boot count, useful to know if the device reboots often because of a bug
- `ver`: the firmware version
- `up`: the device uptime in seconds, useful to know if the device reboots often because of a bug
- `pow`: `bat` (for battery powered) or `ext` (when powered by USB-C or Solar Panel)
- `bat`: the battery level in percentage, 0 if not running on battery
- `volt`: the battery voltage, or the external supplied voltage

### IFTTT Integration

If you have an [IFTTT](https://ifttt.com/) account, you can create an IFTTT applet to receive the JSON payload and insert it in a a new row in a Google Sheet spreadsheet.
To do that, create a new applet with a webhook. The URL of the webhook has to be configured in the device configuration page in the `send_url` parameter (try use `http` instead of `https`).

If you use an Onomondo Connector, the webhook URL has to be configured in your Onomondo connector page.

Here is the filter code you can use to transform the JSON data into cell values. This is optional: you can also receive the JSON in Google Sheet and process it in Google Sheet.

```js
var payload = JSON.parse(MakerWebhooks.jsonEvent.JsonPayload);
payload = payload.payload || payload; // this line is to unwrap the payload from the Onomondo JSON envelope
var formattedRow = "";
for (var k in payload) formattedRow += `|||${payload[k]}`;
GoogleSheets.appendToGoogleSpreadsheet.setFormattedRow(
  formattedRow.substring(3)
);
```

| [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-1.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-1.jpeg) | [![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-2.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-2.jpeg) |

Once the applet is in place, every push from the device will reach IFTTT which will feed the Google Sheet. From there, you can create graphs and process the data as you wish.

[![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-3.jpeg)](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/ifttt-3.jpeg)

# How to use

## Dashboard

The Dashboard is located at `http://192.168.4.1/`.
For there you can see the device status and access some basic buttons.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-dashboard-overview.png)

- `Send now and sleep!`: will send the data now and go to sleep for the configured amount of time (`send_delay`) if sleep is not prevented
- `Scan Operators`: Trigger a new operator scan and new GPS search
- `Prevent Sleep On Battery`: Keeps the device active and prevents it from going to sleep when powered by battery
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

**Factory reset**

To do a factory reset, proceed with the **Firmware flash (first time)** step.

## Debugging and logging

The Web Console can be opened at `http://192.168.4.1/console`.

To see more verbose logging including the modem AT commands, you can activate the `debug_enable` switch in the configuration page.

![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/screenshot-console.png)

**WARNING**: the console allows you to interact with the Modem internals (called `AT commands`).
**DO NOT SEND ANY DATA UNLESS YOU KNOW WHAT YOU ARE DOING.**

# Developer guide

Recommended IDE: [Visual Studio Code](https://code.visualstudio.com) with the [PlatformIO](https://platformio.org) extension.

## Project structure

- `.github`: CI/CD workflows
- `data`: Build components added to the firmware
- `docs`: Documentation and website
- `include`: Firmware include code
- `lib`: Firmware libraries
- `pio`: pio scripts
- `src`: Firmware source code
- `platformio.ini`: PlatformIO configuration

## Building and uploading the firmware

```bash
pio run -t build -e <env>
pio run -t upload -e <env>
pio run -t monitor -e <env>
```

# Contact

If you have any question related to the project, please use the project's [discussion forum](https://github.com/mathieucarbou/Beelance/discussions).
I won't answer any project-related question by email.

To report any bug or request a feature, please use the project's [issue tracker](https://github.com/mathieucarbou/Beelance/issues).
I won't answer any project-related bug or feature by email.

If you want to contact me for a partnership or schedule an assembly session, please send an email to `beelance@carbou.me`.

If you would like to buy a pre-assembled box (optionally setup with IFTTT and Google Sheet), please send an email to `beelance@carbou.me`.
But I strongly suggest you to build it yourself.
Since I do not maintain any stock and don't have a way to cut cost, building a box for you will be 1.5-2x more expensive than doing it yourself.

# Sponsors and partners

[![](https://raw.githubusercontent.com/mathieucarbou/Beelance/main/docs/assets/images/onomondo-community-2.png)](https://onomondo.com)

# Related projects, guides and articles

- [The Internet of Bees: Adding Sensors to Monitor Hive Health](https://makezine.com/projects/bees-sensors-monitor-hive-health)
- [Balance Pour Surveiller Le Poids Des Ruches Pour Moins De 150€](https://miel-jura.com/balance-pour-surveiller-le-poids-des-ruches-pour-moi-de-150e/)
- [Beehive project issue](https://community.blynk.cc/t/beehive-project-issue/27245/48)
- [Balance de ruche à 30€ : épisode 1 - Fabrication du capteur](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-1-fabrication-du-capteur/)
- [Balance de ruche à 30€ : épisode 2 - L'électronique](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-2-lelectronique/)
- [Balance de ruche à 30€ : épisode 3 - L'étalonnage](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-3-letalonnage/)
- [Balance de ruche à 30€ : épisode 4 - Le programme](https://iot.educ.cloud/balance-de-ruche-a-30eu-episode-4-le-programme/)
- [Arduino Bathroom Scale With 50 Kg Load Cells and HX711 Amplifier](https://www.instructables.com/Arduino-Bathroom-Scale-With-50-Kg-Load-Cells-and-H/)
