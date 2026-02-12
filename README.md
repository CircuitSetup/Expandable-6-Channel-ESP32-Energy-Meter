![Expandable 6 Channel ESP32 Energy Meter v1.3 Main Board](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/6-channel_v1.3.jpg)

# CircuitSetup Expandable 6 Channel ESP32 Energy Meter
The Expandable 6 Channel ESP32 Energy Meter can read 6 current channels and 2 voltage channels at a time. Much like our [Split Single Phase Energy Meter](https://circuitsetup.us/index.php/product/split-single-phase-real-time-whole-house-energy-meter-v1-4/), the 6 channel uses current transformers and an AC transformer to measure voltage and power the board(s)/ESP32\. The main board includes a buck converter to power the electronics and ESP32 dev board, which plugs directly into the board. Up to 6 add-on boards can stack on top of the main board to allow you to monitor **up to 42 current channels** in 16-bit resolution, in real time, all at once! 

#### **Usage:**

* North American split single phase 120V/240V 60Hz - mains and/or individual circuits
* European single phase 240V 50Hz (must provide [AC-AC transformer](https://learn.openenergymonitor.org/electricity-monitoring/voltage-sensing/different-acac-power-adapters) 9V or 12V with at least 500mA output)
* 3 phase - It is recommended to measure all 3 voltages with 3 voltage transformers. This can be done by using a main board with 1 add-on board (more information below). A single meter can be used to measure 3-phase, but power (wattage) will not be calculated correctly. Power can be calculated in software, but the [power factor](https://en.wikipedia.org/wiki/Power_factor) will have to be estimated ((voltage*current)*power_factor)). 


#### **Features:**

* Samples 6 current channels & 1 voltage channel (expandable to 2 voltage)
* Add-on boards (up to 6) can expand the meter up to 42 current channels & 8 voltage channels
* Uses 2 [Microchip ATM90E32AS](https://www.microchip.com/wwwproducts/en/atm90e32as) - 3 current channels & 1 voltage per IC
* For each channel the following can also be calculated by the meter:
	* Active Power
	* Reactive Power
	* Apparent Power
	* Power Factor
	* Frequency
	* Temperature
* Uses standard current transformer clamps to sample current
* 22ohm burden resistors per current channel
* Includes built-in buck converter to power ESP32 & electronics
* 2 IRQ interrupts, and 1 Warning output connected to ESP32
* Zero crossing outputs
* Energy Pulse outputs per IC (4 per IC x2)
* SPI Interface
* IC Measurement Error: 0.1%
* IC Dynamic Range: 6000:1
* Current Gain Selection: Up to 4x
* Voltage Reference Drift Typical (ppm/°C): 6
* ADC Resolution (bits): 16


#### **What you'll need:**

* **Current Transformers** (any combination of the following, or any current transformer that does not exceed 720mV RMS, or 33mA output)
	* [SCT-006 20A/25mA Micro](https://circuitsetup.us/product/20a-25ma-micro-current-transformer-yhdc-sct-006-6mm/) (6mm opening - 3.5mm connectors)
	* [SCT-010 80A/26.6mA Mini](https://circuitsetup.us/product/80a-26-6ma-mini-current-transformer-yhdc-sct-010-10mm/) (10mm opening - 3.5mm connectors)
	* [SCT-013-000 100A/50mA](https://circuitsetup.us/product/100a-50ma-current-transformer-yhdc-sct-013/) (13mm opening - 3.5mm connectors)
	* [SCT-016 120A/40mA](https://circuitsetup.us/product/120a-40ma-current-transformer-yhdc-sct-016-with-3-5mm-jack-16mm-opening/) (16mm opening - 3.5mm connectors)
	* [Magnelab SCT-0750-100](https://amzn.to/2IF8xnY) (screw connectors - must sever burden resistor connection on the back of the board since they have a built in burden resistor).
	* [SCT-024 200A/50mA](https://circuitsetup.us/product/200a-50ma-current-transformer-yhdc-sct-024-24mm/) (24mm opening - 3.5mm connectors)
	* Others can also be used as long as they're rated for the amount of power that you are wanting to measure, and have a current output no more than 720mV RMS, or 33mA at peak output.
* **AC Transformer (NOT DC):** 
	* North America: [Jameco Reliapro 120V to 9V AC-AC](https://circuitsetup.us/product/jameco-ac-to-ac-wall-adapter-transformer-9-volt-1000ma-black-straight-2-5mm-female-plug/) or 12v. (Also [available on Amazon.com](https://amzn.to/4qy97ak))The positive pin must be 2.5mm (some are 2.1)
	* Europe: 240V to 9V or 12V AC-AC at least 500mA - [See a list of some recommended transformers here](https://learn.openenergymonitor.org/electricity-monitoring/voltage-sensing/different-acac-power-adapters)
* **ESP32** (choose one):
	* [NodeMCU 32s](https://circuitsetup.us/product/nodemcu-32s-esp32-esp-wroom-32-development-board/) [Also available on Amazon.com](https://amzn.to/4aweG3l)
	* [Espressif ESP32-DevKitC-32E](https://amzn.to/3zRmY7x)
	* [Espressif ESP32-DevKitC-VIE](https://amzn.to/3Ngp2c9) if you need better wifi reception.
	* Anything else with the same pinouts as the above, which are usually 19 pins per side with 3v3 in the upper left & CLK in the lower right
  	* Ethernet: [CircuitSetup ESP32S3 Ethernet Adapter](https://circuitsetup.us/product/6-channel-energy-meter-ethernet-adapter/), the [Lilygo T-ETH-Lite ESP32S3](https://www.lilygo.cc/products/t-eth-lite?variant=43120880779445) or the [Waveshare ESP32-S3 ETH](https://www.waveshare.com/esp32-s3-eth.htm?sku=28972) (for ESPHome only, at the moment)
* **Software** (choose one):
	* Our custom version of [EmonESP](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master/Software/EmonESP) and the [ATM90E32](https://github.com/CircuitSetup/ATM90E32) Arduino library
	* The current release of [ESPHome.](https://esphome.io/components/sensor/atm90e32.html) Details on [integrating with Home Assistant are located here.](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter#esphomehome-assistant) and [here on ESPHome.io](https://esphome.io/components/sensor/atm90e32.html). [More examples of configs are located here.](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master/Software/ESPHome)
	* Libraries for [CircuitPython](https://github.com/BitKnitting/CircuitSetup_CircuitPython) & [MicroPython](https://github.com/BitKnitting/CircuitSetup_micropython)
    

### **Setting up the Meter**

![Expandable 6 Channel ESP32 Energy Meter Diagram](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/6-channel_diagram.png)
![Expandable 6 Channel ESP32 Energy Meter Back Diagram](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/6_channel_v1.4_back.png)

#### **Plugging in the ESP32**
The Expandable 6 Channel ESP32 Energy Meter is made so that an ESP32 dev board can be plugged directly into the meter. See the list above for compatible ESP32 dev boards. 
**Always insert the ESP32 with the 3V3 pin in the upper left of the meter**. The bottom pins are used to connect the voltage signal (from the power plug) to add-on boards. If the ESP32 is inserted into the bottom pins it will more than likely short the ESP32.

#### **Communicating with the ESP32**
The Expandable 6 Channel ESP32 Energy Meter uses SPI to communicate with the ESP32. Each board uses 2 CS pins. 

The **main board** uses the following SPI pins:
* CLK - 18
* MISO - 19
* MOSI - 23
* CS1 - 5 (CT1-CT3 & Voltage 1)
* CS2 - 4 (CT4-CT6 & Voltage 2)

The version of [EmonESP available here](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master/Software/EmonESP) has all of these pins set by default. 

For examples of how to set up your config in ESPHome, [see here](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master/Software/ESPHome) and [here.](https://esphome.io/components/sensor/atm90e32.html)

#### **Add-on Boards**
Add-on boards (up to 6) can expand the main energy meter up to 42 current channels & 8 voltage channels. The add-on boards plug directly into the main board as seen here.

The **add-on board** allows the CS pin to be selected based on the jumper settings at the bottom of the board. This is so multiple add-on boards can be used - up to 6 maximum. Do NOT select more than one CS pin per bank. 
The CS pins can be:
* CT1-CT3 (CS):
	* 0
	* 27
	* 2 (may prevent the ESP32 from being programmed - disconnect jumper if so)
	* 13
	* 14
	* 15
* CT4-CT6 (CS2):
	* 16
	* 17
	* 21
	* 22
	* 25
	* 26
    
![Expandable 6 Channel ESP32 Energy Meter Add-on Diagram](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/6-channel-add-on_diagram.png)

#### **Calibrating Current Sensors & Voltage (AC Transformer)** 
[See here for the calibration procedure](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter#calibration),
[or here for a video](https://youtu.be/BOgy6QbfeZk?t=1261)

##### **Common Calibration Values**
* Current Transformers:
	* 20A/25mA SCT-006: 11143
	* 30A/1V SCT-013-030: 8650
	* 50A/1V SCT-013-050: 15420
	* 80A/26.6mA SCT-010: 41660
	* 100A/50ma SCT-013-000: 27518
	* 120A/40mA: SCT-016: 41787
	* 200A/100mA SCT-024: 27518
	* 200A/50mA SCT-024: 55036
* AC Transformers
	* Jameco 9VAC Transformer 157041: 7305
         
#### **Measuring Power & Other Metering Values**
The Expandable 6 Channel ESP32 Energy Meter uses 2 ATM90E32AS ICs. Each IC has 3 voltage channels and 3 current channels. In order for power metering data to be calculated internally, each current channel must have a reference voltage. If the voltage is out of phase with the current, then the current and power will read as negative, affecting the power factor and power calculations. If you have a split single phase or dual phase setup, the solution is to turn around the current transformer on the wire.

v1.1 of the meter used 1 of the voltage channels for each IC. This means that power and metering data would have to be calculated in software, or voltage channels would have to be mapped via changing registers on the IC to get power and metering data from CT2, CT3, CT5, CT6. 

v1.2 & v1.3 have JP8-JP11 on the back of the board, that would allow all voltage channels to be connected together, which would allow power and other metering values to be calculated. Most of v1.3 came soldered together.

v1.4 removed JP8-JP11, and has voltage channels connected internally on the pcb. 

#### **Measuring Dual Pole (240V) Circuits (split single phase)**
For split single phase applications, dual pole circuits have 2 hot wires that total 240V (usually red and black in newer buildings). In most cases both poles are used equally, but in others there may be electronics in the appliance that use only 1 pole. 
There are 3 different options for measuring these circuits:
- Monitor 1 phase with 1 CT, and double the current & power output in software (least accurate) An example of how to do this in ESPHome is the same as the [multiply filter in this example config](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/blob/master/Software/ESPHome/6chan_energy_meter_main_board.yaml#L84).
- Use 2 CTs to monitor each hot wire on the circuit (if you are monitoring 1 voltage, the CTs should be in opposite directions from eachother)
- If the circuit has enough wire coming out of the breaker, and the CT is large enough, run both hot wires through 1 CT in opposite directions

#### **Measuring A Second Voltage**
The holes labeled VA2 next to the power plug on the meter main board, and in the bottom right of the add-on board are for measuring a second voltage. To do this you must:
* Sever (with a knife) JP12 and JP13 on the back of the board for v1.3+, or JP7 for prior versions
* Use a second AC transformer, ideally one identical to the primary
* Plug in the second AC transformer to an outlet on the opposite phase to the primary
* Solder on a pin header, 3.5mm (2.54mm for v1.3 and earlier) screw connector, or DC style jack pigtail on to VA2+ & VA2- (next to the main power/voltage jack)

When voltage jumpers are severed, the voltage reference for CT4-CT6 will be from VA2. This means that current transformers for CT4-CT6 should be hooked up to circuits that are on the same phase as VA2, and CT1-CT3 should be hooked up to circuits that are in phase with the primary voltage. If a CT is not in phase with the voltage its current and power readings will be negative. If, for example, you have 4 circuits in phase with the primary, and 2 in phase with VA2, you can reverse the current transformer on the wire to put it in phase with the voltage (assuming split single phase or dual phase)

For add-on boards, the primary voltage will come from the main board. The optional secondary voltage measurement (also VA2 pins), will be in phase with CT4-CT6.

#### **Measuring 3-Phase Electricity**
What you'll need to measure all 3 phases properly: 
* 6 Channel Main Board (v1.4 and above)
* 6 Channel Add-on Board
* 3 voltage transformers - one for each phase. These can either be wall, plug-in type (if you have an outlet wired to each phase), or stand-alone transformers wired directly to breakers. They must bring down the voltage to between 9-14VAC. The first one that plugs into the main board also powers the ESP32 and electronics, so it must output at least 500mA.
* Headers soldered to the VA2 terminals on the main board and add-on board
* JP12 and JP13 severed on the main board & add-on boards
Similar to the above for measuring a second voltage, once JP12 and JP13 are severed:
* CT1-CT3 on the main board, and CT1-CT3 on the add-on board will be in phase with the 1st phase
* CT4-CT6 on the main board with the 2nd phase
* CT1-CT6 on the add-on board with the 3rd phase. 

The transformers should be calibrated individually for greater accuracy.

Alternatively, you can use *2 add-on boards* and connect 1 transformer to each board:
* Locate the bottom most pins (i.e. those which the ESP32 does *not* plug into), marked "VA-" and "VA+" for each add-on board. These normally provide the voltage reference to the add-on board from the main board
* Cut these pins off
* Solder a connector, or the wires from a transformer, in the bottom right holes marked "VA2" on the add-on boards. Polarity does not matter (if you are getting negative current readings, reverse the CT on the wire)
* *Do not sever JP12 and JP13* 

On 3-phase systems, a current transformer that's connected to a phase not in phase with the voltage reference will always result in near-zero active power and power factor.

### **Setting Up Software**
#### **EmonESP/EmonCMS**
EmonESP is used to send energy meter data to a [local install of EmonCMS](https://github.com/emoncms/emoncms), or [emoncms.org](https://emoncms.org/). Data can also be sent to a MQTT broker through this. EmonCMS has Android and IOS apps.
[The ESP32 software for EmonESP is located here](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master/Software/EmonESP), and can be flash to an ESP32 using the [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/). See [details on setup here.](https://github.com/CircuitSetup/Split-Single-Phase-Energy-Meter/tree/master/Software/EmonESP)

#### **ESPHome/Home Assistant**
[ESPHome](https://esphome.io/components/sensor/atm90e32.html) can be loaded on an ESP32 to seamlessly integrate energy data into [Home Assistant](https://www.home-assistant.io/). Energy data can then be saved in InfluxDB and displayed with Grafana. At the same time, the energy data can also be used for automations in Home Assistant. 

A [newer feature in Home Assistant allows you to monitor electricity usage](https://www.home-assistant.io/blog/2021/08/04/home-energy-management/) [directly in Home Assistant](https://demo.home-assistant.io/#/energy). You can also track usage of individual devices and/or solar using the 6 channel meter!

##### **Installing on Home Assistant**
- If you have Home Assistant installed, click the button below OR go to **Settings** in the left menu, click **Add-ons**, then **Add-on Store** (bottom right blue button), Search for **ESPHome** - Click on **Install**

[![Install ESPHome Add-on](https://my.home-assistant.io/badges/config_flow_start.svg)](https://my.home-assistant.io/redirect/config_flow_start?domain=esphome)

![ESPHome add-on](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/esphome_add-on_install.png)

##### **Flashing ESPHome to your ESP32**
- The easiest way to load ESPHome is to use our [ESPHome Installer](https://circuitsetup.github.io/ESPWebInstaller/)
- Connect your ESP32 to your computer via USB port
- Choose your 6 Channel Meter configuration, and click on **Connect**
- Choose the **COM port** for your ESP32 (if the ESP32 does not connect, check that you're using the correct COM port)
- Click on **Install CircuitSetup 6 Channel Energy Meter...**
- Before clicking **Install** you may need to hold down the right button on the ESP32
- Enter your **WiFi Credentials**
- Your meter should now appear under **ESPHome Builder** and **Settings > Devices & Services**
- A default config is loaded - names and calibrations can be modified, additional energy data enabled, or you can copy all config files locally [from here](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/tree/master/Software/ESPHome)
- It is also recommended to set an api key for Home Assistant, [which can be generated here](https://esphome.io/components/api.html#configuration-variables) if not done automatically

##### **Semi-Automatic Calibration**
Default values can be used, and calibration does not have to be done, but is recommended if you want super-accurate results. Luckily a new system has been made for ESPHome that makes this process much easier, which allows you to input known values for voltage and current, and then calculate the gain values. Offset calibration can also be calculated if you are seeing non-zero values when voltage and current should be reading 0.
[More details on how semi-automatic calibration works is located here](https://esphome.io/components/sensor/atm90e32.html#calibration)

#### **Getting Data in InfluxDB**
- If you don't already, install the InfluxDB add-on in Home Assistant via **Settings > Add-ons > Add-on Store** (bottom right blue button)
- Open the Web UI, and click on the **InfluxDB Admin** tab, add a database **homeassistant**
- Click on the **Users** tab (under Databases on the same screen), and create a new user **homeassistant** with All permissions
- Go to **Settings > Add-ons > InfluxDB > Documentation (top menu) > Integrating into Home Assistant**, and follow the instructions for editing your Home Assistant configuration.yaml
- Restart Home Assistant
- Data should now be available in Home Assistant and available under http://homeassistant.local:8086 or the IP of Home Assistant

#### **Getting Data in the Home Assistant Energy Dashboard**
![Home Assistant Energy Config](https://raw.githubusercontent.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/master/Images/ha_energy_config.png)

To display data in the Home Assistant Energy Dashboard you must be using ESPHome v1.20.4 or higher, and have at least one ```total_daily_energy``` platform configured in your ESPHome config. ```time``` is also needed.

##### **For Total Energy Consumption**
```yaml
#Total kWh
  - platform: total_daily_energy
    name: ${disp_name} Total kWh
    power_id: totalWatts
    filters:
      - multiply: 0.001
    unit_of_measurement: kWh
time:
  - platform: sntp
    id: sntp_time 
```
Where ```totalWatts``` is the sum of all watt calculations on the meter. [See an example of this here.](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/blob/master/Software/ESPHome/6chan_energy_meter_main_board.yaml) In the example, this was done with a [lambda template](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/blob/ae0c42b86ec5faa7bc923f488fb5cc09cf5517eb/Software/ESPHome/6chan_energy_meter_main_board.yaml#L157).

##### **For Solar Panels**
The same can be done as above to track solar panel use and export. The current channels on the meter that are tracking solar usage must have their own lambda template calculation.

[See this example for how you could set this up with the 6 Channel Meter.](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/blob/master/Software/ESPHome/6chan_energy_meter_house_solar_ha_kwh.yaml)

##### **For Individual Device/Circuit Tracking**
To do this you must have power calculated by the meter, or a lambda template that calculates watts per circuit. Then can use a kWh platform for each of the current channels on the 6 channel energy meter. For example:
```yaml
#CT1 kWh
  - platform: total_daily_energy
    name: ${disp_name} CT1 Watts Daily
    power_id: ct1Watts
    filters:
      - multiply: 0.001
    unit_of_measurement: kWh
```
```ct1Watts``` references the id of the watt calculation. In the [example config](https://github.com/CircuitSetup/Expandable-6-Channel-ESP32-Energy-Meter/blob/ae0c42b86ec5faa7bc923f488fb5cc09cf5517eb/Software/ESPHome/6chan_energy_meter_main_board.yaml#L79), this is:
```yaml
      power:
        name: ${disp_name} CT1 Watts
        id: ct1Watts
```

##### **Setup in Home Assistant**
- Go to **Configuration > Energy**
- For total energy, click **Add Consumption** under Electricity grid
- The name of the total_daily_energy platform, like 6C Total kWh, should be available to choose
- You can also set a static cost per kWh or choose an entity that tracks the cost of your electricity
- For **Individual devices** chose the name of the individual circuits, like 6C CT1 Watts Daily
- If monitoring your Solar Panels with a 6 channel meter, you can also set this here, but it will not register unless energy is being consumed by your house or flowing out to the grid.

### **FAQ**
##
**Q:** I am getting a low reading, or nothing at all for one CT - what is wrong?

**A:** Sometimes the jack for the CT is a bit stiff, and you may need to push in the CT connector into the board jack until it clicks. If it is definitely all the way in, it's possible the connector or somewhere else has a loose connection, and we will replace the meter for free.

##
**Q:** Does the 6 channel energy meter work in my country?

**A:** Yes! There is a setting to set the meter to 50Hz or 60Hz power. You will need to purchase an **AC** transformer that brings down the voltage to between 9-12V **AC**. Transformers for the US are for sale in the circuitsetup.us store.

##
**Q:** I'm getting a negative value on one current channel. What is going on? 

**A:** This usually means that the CT is on the wire backwards - just turn it around! If all CT readings are negative, you can flip the AC transformer around in the plug.

##
**Q:** I'm getting a small negative value when there is no load at all, but a positive value otherwise. What is going on?

**A:** This is caused by variances in resistors and current transformers. You can either calibrate the current transformers to the meter, or add this lambda section to only allow positive values for a current channel:
```yaml
  - platform: template
    name: ${disp_name} CT1 Watts Positive
    id: ct1WattsPositive
    lambda: |-
      if (id(ct1Watts).state < 0) {
        return 0;
      } else {
        return id(ct1Watts).state ;
      }
    accuracy_decimals: 2
    unit_of_measurement: W
    icon: "mdi:flash-circle"
    update_interval: ${update_time}
```
Then for your total watts calculation, use ct1WattsPositive

Calibrating voltage offset and current offset will correct this issue as well.

##
**Q:** The CT wires are not long enough. Can I extend them? 

**A:** Yes, you absolutely can! Something like a headphone extension or even an ethernet wire can be used (if you don't mind doing some wiring). It is recommended to calibrate the CTs after adding any particularly long extension.

##
**Q:** Can I use this CT with the 6 channel meter?

**A:** More than likely, yes! As long as the output is rated at less than 720mV RMS, or 33mA.

##
**Q:** Can I use SCT-024 200A CTs with the 6 channel meter?

**A:** If you need to measure up to 200A, then this is not recommended. At 200A, our newest SCT-024's will output 50mA. That means, **the max you should measure with the SCT-024 plugged into the 6 channel meter is 132A**. In a residential setting with 200A service, it is highly unlickly that you will use more than 132A per phase sustained. In fact, unless you have your own dedicated transformer, and a very large house, it is impossible.

##
**Q:** How do I know if my CT has a burden resistor?

**A:** There is a built in burden resistor if the output is rated in volts. In this case the corresponding jumper on the rear of the meter should be severed.

##
**Q:** Can I connect the meter to ethernet?

**A:** Yes! We have an [adapter availble](https://circuitsetup.us/product/6-channel-energy-meter-to-lilygo-t-eth-lite-esp32s3-ethernet-adapter/) to use the Lilygo T-ETH-Lite ESP32S3 with the 6 channel energy meter. An example configuration is [available here](/blob/master/Software/ESPHome/6chan_energy_meter_main_ethernet.yaml).

##
**Q:** How do I set up a solar configuration and power that is returned to the grid?

**A:** This is assuming you are measuring mains and the circuit that connects your soloar grid to your panel, and the CTs are reading negative when exporting power to the grid. [See this ESPHome example config](/blob/master/Software/ESPHome/6chan_energy_meter_house_solar_ha_kwh.yaml).

##
**Q:** How can I do calculations across 2 power sensors from 2 different meters (2 or more ESP32's)?

**A:** The easiest way to do the math between 2 meters would be to [create a helper in Home Assistant](https://my.home-assistant.io/redirect/config_flow_start?domain=template). _Create Helper > Template > Template a sensor_

Then for State template enter:
```
{% set meter1 = states(‘sensor.energy_meter1_total_watts’) | float(0) %}
{% set meter2 = states(‘sensor.energy_meter2_total_watts’) | float(0) %}
{{ meter1 – meter2 }}
```
Where ```energy_meterX_total_watts``` is the name of the total_watts for that meter as defined in the ESPhome config. If you start typing after ‘sensor.’ it should pop up.

Unit of measurement: W
Device class: Power
State class: Measurement
Device: The primary meter

The same can be repeated for Amps, if you want.

The output will then be available in the energy dashboard as ```sensor.helper_name```.


### **More resources:**
* [How to flash ESPHome to your ESP32](https://esphome.io/guides/getting_started_hassio.html)
* [Digiblur video of energy meter calibration and setup process of ESPHome](https://www.youtube.com/watch?v=BOgy6QbfeZk)
* [DIY Home Power & Solar Energy Dashboard - Home Assistant w/ ESPHome](https://www.youtube.com/watch?v=n2XZzciz0s4)
* [TH3D video with add-on board](https://www.youtube.com/watch?v=zfB4znO6_Z0)

