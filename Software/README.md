### Software
The following are various implementations that can be used with the 6 Channel Energy Meter. Instructions for [setting up software for the meter is located here](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter#setting-up-software).
- ESPHome - Example configuration files
- EmonCMS - Device config files (these are now included with the device module for EmonCMS)
- EmonESP - ESP software that runs on the meter's ESP32, and sends data to EmonCMS or via MQTT
- Grafana - Dashboard examples

### Libraries
These are used to communicate directly with the ATM90E32 IC's used with the 6 channel meter. If you want to build your own software, you will need one of these.
- [Arduino/ESP32](https://github.com/CircuitSetup/ATM90E32)
- [CircuitPython](https://github.com/BitKnitting/CircuitSetup_CircuitPython)
- [MicroPython](https://github.com/BitKnitting/CircuitSetup_micropython)

### User Created Projects
- [CS/ESPHome](https://github.com/sillygoose/cs_esphome) - Python data collection utility. Sensor data is sourced using the ESPHome API and stored in an InfluxDB database. Grafana dashboard included.
- [ESPSense](https://github.com/cbpowell/ESPSense) - Can be used to send 6 Channel Energy Meter data via ESPHome to your Sense Home Energy Monitor software
- [Raspberry Pi to MQTT](https://github.com/tsaitsai/circuitsetup_energy_to_mqtt) - Hook up a 6 channel meter to a Raspberry Pi (in place of an ESP32) and send data over MQTT
- [6 Channel Energy Meter with a wired ethernet connection (WT32-ETH01)](https://community.home-assistant.io/t/wt32-eth01-with-esphome-driving-circuitsetup-expandable-6-channel-esp32-energy-meter-main-board/709027) - Wiring and ESPHome config for using the 6 channel meter with ethernet instead of WiFi.
