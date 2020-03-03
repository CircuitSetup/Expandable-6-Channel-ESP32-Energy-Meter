## Expandable 6 Channel Energy Meter Change Log
**3/2/2020 - v1.4 rev1**
- Changed micro-hdmi connector to through-hole
- Changed spacing on VA2+/VA2- (voltage 2 connection) to 3.5mm to allow for standard screw connectors

**12/16/2019 – v1.4**
- Added optional micro-hdmi connection to connect the current transformer remote board. This allows for all 6 current transformer connections to connect to a remote pcb, which then connects to the main board via 1 micro-hdmi cable. Instead of 6 wires coming out of the meter, there can now be 1.
- Added current transformer remote pcb
- Removed JP8-JP11 and connected all voltage channels together for each ATM90E32

**10/15/2019 – v1.3 rev1**
- Added voltage divider on ground for voltage channel 2
- Added JP12 & JP13 to disconnect voltage channel 1 from 2

**10/1/2019 – v1.3**
- Added voltage divider on ground for voltage channel 1
- Added 20k pullup for voltage channel 1
- Changed all voltage dividers to 20k and 1k so voltage input from a 12v AC transformer is around 650mV - this allows more voltage in than previous versions, lowering the voltage calibration
- Removed C37

**8/1/2019 – v1.2**
- Added C37 to connect neutral and DC ground
- Added solder jumpers JP8JP12 to connect all voltage channel inputs together

**6/1/2019 – v1.1**
- Initial public release
