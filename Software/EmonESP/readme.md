### EmonESP

EmonESP can send data to EmonCMS, or MQTT. This version has configuration settings for the 6 channel meter, and includes calibration settings that are able to be changed via the web interface. 

Some notes: 
- You must use an ESP32
- To compile, you must have the the [ATM90E32 Arduino library](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/libraries/ATM90E32)
- To add an add-on board, uncomment ADDON_BOARDS and set the number for NUM_OF_ADDON_BOARDS in energy_meter.h
- If you wish to read metering values and have the meter calculate active power (as opposed to doing it in software), solder together jp9-jp11 on the back of the board, and uncomment JP9_JP11_SET in energy_meter.h
- If you need to calibrate a specific current or voltage channel on an add-on board, by default the values from the main board calibration are used, so they would need to have a seperate variable defined, and set manually in energy_meter.cpp
- By default individual current channels from the main board are sent to EmonCMS. Add-on boards are added to the total current and watts/power.
- If you want to export metering values (Power Factor, Fundamental, Harmonic, Reactive, Apparent Power, and Phase Angle) uncomment EXPORT_METERING_VALS in energy_meter.h

For details on setup, please see the [readme under the Split Single Phase Energy Meter.](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/EmonESP)
