### EmonESP

EmonESP can send data to EmonCMS, or MQTT. This version has configuration settings for the 6 channel meter, and includes calibration settings that are able to be changed via the web interface. 

Some notes: 
- You must use an ESP32
- To compile, you must have the the [ATM90E32 Arduino library](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/libraries/ATM90E32)
- If you need to calibrate a specific voltage channel on an add-on board, by default the values from the main board calibration are used, so they would need to have a seperate variable defined, and set manually in energy_meter.cpp

For details on setup, please see the [readme under the Split Single Phase Energy Meter.](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/EmonESP)
