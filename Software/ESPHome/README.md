# ESPHome Configuration for Expandable 6-Channel ESP32 Energy Meter

This folder contains [ESPHome](https://esphome.io) configuration files used to monitor power and energy via the CircuitSetup Expandable 6-Channel Energy Meter. These configurations support flexible deployments - with or without add-on boards - as well as Wi-Fi or Ethernet connections. Many files use `!include` statements to modularize calibration, sensor definitions, power quality metrics, and device status.

---

## Compiled Firmware

[See here to flash your ESP32](https://circuitsetup.github.io/ESPWebInstaller/)

---

## 📁 Base Config Files

| File | Description |
|------|-------------|
| `6chan_common.yaml` | Base shared configuration: sets platform (`esp32`), SPI, includes time and API components. Meant to be included in Wi-Fi-based setups. |
| `6chan_common_ethernet.yaml` | Same as `6chan_common.yaml`, but for the [Lilygo T-ETH-Lite ESP32S3](https://lilygo.cc/products/t-eth-lite) and the [CircuitSetup 6 Channel Energy Meter Ethernet Adapter](https://circuitsetup.us/product/6-channel-energy-meter-to-lilygo-t-eth-lite-esp32s3-ethernet-adapter/) . |
| `6chan_common_ethernet_waveshare.yaml` | Ethernet base config for the Waveshare ESP32-S3-ETH and CircuitSetup Ethernet WS adapter. |
| `6chan_energy_meter_main_board.yaml` | Full configuration for monitoring only the main board's 6 channels via Wi-Fi. |
| `6chan_energy_meter_main_ethernet.yaml` | Ethernet-based variant of `6chan_energy_meter_main_board.yaml` (Lilygo adapter). |
| `6chan_energy_meter_main_ethernet_waveshare.yaml` | Ethernet-based variant of `6chan_energy_meter_main_board.yaml` (Waveshare adapter). |
| `6chan_energy_meter_1-addon.yaml` | Main board + 1 add-on board (12 channels) over Wi-Fi. |
| `6chan_energy_meter_1-addon_ethernet.yaml` | Main board + 1 add-on board (12 channels) over Ethernet (Lilygo adapter). |
| `6chan_energy_meter_1-addon_ethernet_waveshare.yaml` | Main board + 1 add-on board (12 channels) over Ethernet (Waveshare adapter). |
| `6chan_energy_meter_2-addons.yaml` | Main board + 2 add-on boards (18 channels) over Wi-Fi. |
| `6chan_energy_meter_2-addons_ethernet.yaml` | Main board + 2 add-on boards (18 channels) over Ethernet (Lilygo adapter). |
| `6chan_energy_meter_2-addons_ethernet_waveshare.yaml` | Main board + 2 add-on boards (18 channels) over Ethernet (Waveshare adapter). |
| `6chan_energy_meter_3-addons.yaml` | Main board + 3 add-on boards (24 channels) over Wi-Fi. |
| `6chan_energy_meter_3-addons_2-voltages.yaml` | Main board + 3 add-on boards (24 channels) with second-voltage monitoring enabled. |
| `6chan_energy_meter_3-addons_ethernet.yaml` | Main board + 3 add-on boards (24 channels) over Ethernet (Lilygo adapter). |
| `6chan_energy_meter_3-addons_ethernet_waveshare.yaml` | Main board + 3 add-on boards (24 channels) over Ethernet (Waveshare adapter). |
| `6chan_energy_meter_4-addons.yaml` | Main board + 4 add-on boards (30 channels) over Wi-Fi. |
| `6chan_energy_meter_4-addons_ethernet.yaml` | Main board + 4 add-on boards (30 channels) over Ethernet (Lilygo adapter). |
| `6chan_energy_meter_4-addons_ethernet_waveshare.yaml` | Main board + 4 add-on boards (30 channels) over Ethernet (Waveshare adapter). |
| `6chan_energy_meter_5-addons.yaml` | Main board + 5 add-on boards (36 channels) over Wi-Fi. |
| `6chan_energy_meter_5-addons_ethernet.yaml` | Main board + 5 add-on boards (36 channels) over Ethernet (Lilygo adapter). |
| `6chan_energy_meter_5-addons_ethernet_waveshare.yaml` | Main board + 5 add-on boards (36 channels) over Ethernet (Waveshare adapter). |
| `6chan_energy_meter_6-addons.yaml` | Main board + 6 add-on boards (42 channels) over Wi-Fi. |
| `6chan_energy_meter_6-addons_ethernet.yaml` | Main board + 6 add-on boards (42 channels) over Ethernet (Lilygo adapter). |
| `6chan_energy_meter_6-addons_ethernet_waveshare.yaml` | Main board + 6 add-on boards (42 channels) over Ethernet (Waveshare adapter). |

---

## 📂 calibration/

When these files are included, and calibration is enabled, semi-automatic calibration can be done for each current and voltage channel by providing a current and voltage reference. Calculated values for offset and gain stored in memory take priority over config values.
To save calculated calibration values, copy them from the ESPHome logs to your config then use the clear buttons to clear the calibration values from memory

[See details on semi-automatic calibrations here](https://esphome.io/components/sensor/atm90e32.html#calibration)

| File | Description |
|------|-------------|
| `6chan_xxxx_calibration.yaml` | Enables calibration buttons for gain, power offset, current/voltage offset, and clear buttons for each |
| `6chan_xxxx_offset_calibrations.yaml` | Offset calibration values fine tuned via the output of the semi-automatic calibrations. Values should be copied in from the meter's ESPHome log |

---

## 📂 meter_sensors/

Default current, power (watts), and voltage sensor entities per meter board
[More details on each sensor here](https://esphome.io/components/sensor/atm90e32.html#configuration-variables)

---

## 📂 status_fields/

These fields display the status of each CT, or phase, with a short text description. [See more details here](https://esphome.io/components/sensor/atm90e32.html#text-sensor)

---

## 📂 power_quality/

Contains power quality values that are available for each phase, and can be enabled per meter board. Enabling these on every current channel is possible, but will produce a lot of data and sensors that are likely not needed. They include reactive_power, apparent_power, harmonic_power, power_factor, phase_angle, peak_current
[More details on each sensor here](https://esphome.io/components/sensor/atm90e32.html#configuration-variables)

---

## 📂 examples/

Pre-configured use cases to jumpstart your setup.

| File | Description |
|------|-------------|
| `6chan_energy_meter_3-phase_ethernet.yaml` | Configuration for 3-phase measurement with Ethernet. Uses main board + 1 add-on. |
| `6chan_energy_meter_6-addons_ethernet.yaml` | Max configuration: main board + 6 add-ons over Ethernet (42 channels). |
| `6chan_energy_meter_house_solar_ha_kwh.yaml` | Real-world example measuring house + solar circuits. |
| `6chan_main_calibration_beta.yaml` | Beta calibration test — for experimentation only, not recommended for production. |

---

## Measuring More Than 65A on a Current Channel
The max value for current that the meter can output is 65.535. If you expect to measure current over 65A, divide `current_cal_ctx` by 2 (120A CT) or 4 (200A CT) and multiply the current and power values by 2 or 4, uncommenting the below and changing the `id` and `phase_x` accordingly.

Under `sensor:` add:
```
- id: !extend ${main_meter_id1} # CTs 1-3 
  phase_a: # CT1
    current:
      filters:
        - multiply: 2
    power:
      filters:
        - multiply: 2
```
---

## Monitoring More Than 1 Voltage
By default, all voltage channels are connected, and set up to monitor a single phase. [See here on what needs to be done with hardware if monitoring more than 1 voltage.](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master?tab=readme-ov-file#measuring-a-second-voltage)

In the config, under `sensor:` add:
```
- id: !extend ${main_meter_id2}
  phase_a:
    voltage:
       name: Voltage 2
       id: ic2Volts
       accuracy_decimals: 1
  frequency:
    name: Frequency 2
```
---

## Excluding/Removing Sensors
If you, for example, want to see power quality sensors on only 2 out of the 4 CT channels of the main board, you could add something like this in your base config file. This is assuming you are including `Software/ESPHome/power_quality/6chan_main_power_quality.yaml` and you want to exclude power quality sensors for the last 4 CTs:

Under `sensor:` add:
```
- id: !extend ${main_meter_id1}
  phase_c: # CT3
    reactive_power: !remove
    apparent_power: !remove
    harmonic_power: !remove
    power_factor: !remove
    phase_angle: !remove
    peak_current: !remove

- id: !extend ${main_meter_id2}
  phase_a: # CT4
    reactive_power: !remove
    apparent_power: !remove
    harmonic_power: !remove
    power_factor: !remove
    phase_angle: !remove
    peak_current: !remove
  phase_b: #CT5
    reactive_power: !remove
    apparent_power: !remove
    harmonic_power: !remove
    power_factor: !remove
    phase_angle: !remove
    peak_current: !remove
  phase_c: #CT6
    reactive_power: !remove
    apparent_power: !remove
    harmonic_power: !remove
    power_factor: !remove
    phase_angle: !remove
    peak_current: !remove
```
---

## Changing Default CS Pins
If you've changed the jumpers on add-on board(s) from the default, they can be changed by adding the following to your main config under ```sensor:```:
```
- id: !extend ${addon1_id1} #1st add-on, CTs 1-3
  cs_pin: 16 #change to match physical jumper position

- id: !extend ${addon2_id2} #2nd add-on, CTs 4-6
  cs_pin: 27
```
---

## Including Files Locally
You can copy config files to your local directories and change the `packages:` part of your config to something like this:
```
packages:
  common: !include Software/ESPHome/6chan_common.yaml
  main: !include Software/ESPHome/meter_sensors/6chan_main_sensor.yaml
  main_status: !include Software/ESPHome/status_fields/6chan_main_status.yaml
  main_cal: !include Software/ESPHome/calibration/6chan_main_calibration.yaml
```
---
