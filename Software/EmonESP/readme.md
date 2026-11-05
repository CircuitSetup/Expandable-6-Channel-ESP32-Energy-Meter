### EmonESP

EmonESP sends energy meter data to [EmonCMS](https://emoncms.org/), or MQTT. This version has configuration settings for the 6 channel meter, and includes calibration settings that are able to be changed via the web interface. 

Some notes: 
- You must use an ESP32
- To compile, you must have the [ATM90E32 Arduino library](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/libraries/ATM90E32) in your Arduino libraries folder.
- If you need to calibrate a specific voltage channel on an add-on board, by default the values from the main board calibration are used, so they would need to have a seperate variable defined, and set manually in energy_meter.cpp

For details on setup, please see the [readme under the Split Single Phase Energy Meter.](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/EmonESP)

### Build and checks

The normal and deploy environments pin PlatformIO Espressif32 7.1.3 (Arduino
2.0.17), ESPAsyncWebServer 3.12.1, AsyncTCP 3.5.0, PubSubClient 2.8.0, and
ATM90E32 2.0.0. Espressif32 8 switches to Arduino 3; migrating the legacy OTA,
Wi-Fi and certificate-fingerprint APIs needs separate compatibility testing.
`emonesp_staging` tests upstream library heads on the same pinned platform.

Build without uploading:

```sh
pio run -e emonesp-nodemcu32s -e emonesp_deploy
pio run -e emonesp_spiffs -t buildfs
```

The web bundle includes jQuery 3.7.1, Knockout 3.5.3, and the existing Knockout
Mapping plugin. Rebuild it with `bash lib/js/combine.sh` after updating vendor
files. Use full jQuery: the settings page requires its AJAX methods. Flash the
SPIFFS image as well as the firmware when installing these browser updates.

Run the host regression checks with Python 3, Node.js, and a C++11 compiler:

```sh
python tests/test_config_eeprom.py
python tests/test_web_upload.py
python tests/test_web_auth.py
python tests/test_runtime.py
python tests/test_web_config.py
node tests/test_calibration_ui.cjs
```

The first three scripts accept a compiler path as their first argument. The other
C++ checks use `CXX` (default `g++`). Host checks exercise production code with
hardware doubles; they do not establish metering accuracy or flash reliability.

Set an admin login before allowing other devices to reach the meter. Firmware
uploads and settings enforce that login in both station and access-point modes.
Admin usernames and passwords are limited to 15 UTF-8 bytes by the existing
EEPROM slots. Invalid or
incomplete calibration forms return HTTP 400 without changing saved values.
The EEPROM layout remains compatible with existing settings.
