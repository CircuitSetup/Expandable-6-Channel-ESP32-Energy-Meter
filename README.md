![Expandable 6 Channel ESP32 Energy Meter v1.3 Main Board](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/6-channel_v1.3.jpg)

# CircuitSetup Expandable 6 Channel ESP32 Energy Meter
The Expandable 6 Channel ESP32 Energy Meter can read 6 current channels and 2 voltage channels at a time. Much like our [Split Single Phase Energy Meter](https://circuitsetup.us/index.php/product/split-single-phase-real-time-whole-house-energy-meter-v1-4/), the 6 channel uses current transformers and an AC transformer to measure voltage and power the board(s)/ESP32\. The main board includes a buck converter to power the electronics and ESP32 dev board, which plugs directly into the board. Up to 6 add-on boards can stack on top of the main board to allow you to monitor **up to 42 current channels** in 16-bit resolution, in real time, all at once! This product is currently in the prototype stage, so components may change.

#### **Features:**

*   Samples 6 current channels & 1 voltage channel (expandable to 2 voltage)
*   Add-on boards (up to 6) can expand the meter up to 42 current channels & 8 voltage channels
*   Uses 2 [Microchip ATM90E32AS](https://www.microchip.com/wwwproducts/en/atm90e32as) - 3 current channels & 1 voltage per IC
*   For each channel the following can also be calculated by the meter:
    *   Active Power
    *   Reactive Power
    *   Apparent Power
    *   Power Factor
    *   Frequency
    *   Temperature
*   Uses standard current transformer clamps to sample current
*   22ohm burden resistors per current channel
*   Includes built-in buck converter to power ESP32 & electronics
*   2 IRQ interrupts, and 1 Warning outputs
*   Zero crossing outputs
*   Energy Pulse outputs per IC (4 per IC x2)
*   SPI Interface
*   Measurement Error: 0.1%
*   Dynamic Range: 6000:1
*   Gain Selection: Up to 4x
*   Voltage Reference Drift Typical (ppm/°C): 6
*   ADC Resolution (bits): 16

#### **What you'll need:**

*   **Current Transformers** (depending on your application)
    *   [SCT-013-030 30A/1V](https://amzn.to/2ZNDTQb)
    *   [SCT-013-050 50A/1V](https://amzn.to/2NKSb1P)
    *   [SCT-013-000 100A/50mA](https://circuitsetup.us/index.php/product/100a-50ma-current-transformer-yhdc-sct-013/) (13mm opening - 3.5mm connectors)
    *   [SCT-016 120A/40mA](https://circuitsetup.us/index.php/product/120a-40ma-current-transformer-yhdc-sct-016-with-3-5mm-jack-16mm-opening/) (16mm opening - 3.5mm connectors)
    *   [Magnelab SCT-0750-100](https://amzn.to/2IF8xnY) (screw connectors - must sever burden resistor connection on the back of the board since they have a built in burden resistor).
    *   Others can also be used as long as they're rated for the amount of power that you are wanting to measure, and have a current output no more than 720mA.
*   **AC Transformer:** [Jameco Reliapro 9v](https://amzn.to/2XcWJjI) or 12v. The positive pin must be 2.5mm (some are 2.1)
*   **ESP32** (choose one):
    *   [NodeMCU](https://amzn.to/2pCtTtz)
    *   [Espressif DevKitC](https://amzn.to/2PHvVsg)
    *   [DevKitC-32U](https://www.mouser.com/ProductDetail/Espressif-Systems/ESP32-DevKitC-32U?qs=%252BEew9%252B0nqrCEVvpkdH%2FG5Q%3D%3D) if you need better wifi reception (don't forget the antenna)
    *   Anything else with the same pinouts as the above, which are usually 19 pins per side with 3v3 in the upper left & CLK in the lower right
*   **Software** (choose one):
    *   Our custom version of [EmonESP](https://github.com/CircuitSetup/EmonESP) and the [ATM90E32](https://github.com/CircuitSetup/ATM90E32) Arduino library
    *   The current dev release of [ESPHome.](https://github.com/esphome/esphome/tree/dev) Details on [integration with Home Assistant are located here.](https://github.com/digiblur/digiNRG_ESPHome) and [here on ESPHome.io](https://next.esphome.io/components/sensor/atm90e32.html)
    *   Libraries for [CircuitPython](https://github.com/BitKnitting/CircuitSetup_CircuitPython) & [MicroPython](https://github.com/BitKnitting/CircuitSetup_micropython)
