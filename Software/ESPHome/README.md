### Example Home Assistant/ESPHome config files for the 6 Channel Energy Meter
- [6chan_energy_meter_main_board.yaml](/Software/ESPHome/6chan_energy_meter_main_board.yaml) - 1 main board with 6 current channels
- [6chan_energy_meter-1-add-on.yaml](/Software/ESPHome/6chan_energy_meter-1-add-on.yaml) - Main board + 1 add-on board, 12 current channels
- [6chan_energy_meter_6-addons_42-channels.yaml](/Software/ESPHome/6chan_energy_meter_6-addons_42-channels.yaml) - Main board + 6 added boards, 42 current channels
- [6chan_energy_meter_house_solar_ha_kwh.yaml](/Software/ESPHome/6chan_energy_meter_house_solar_ha_kwh.yaml) - Main board example for Split Single Phase whole house, solar, and 2 other circuits - accomodates for solar return to grid
- [6chan_energy_meter_v1.2.yaml](/Software/ESPHome/6chan_energy_meter_v1.2.yaml) - Older main board example with power calculations
- [6chan_energy_meter_main_ethernet.yaml](/Software/ESPHome/6chan_energy_meter_main_ethernet.yaml) - Main board, 6 current channels using the Lilygo T-ETH Lite ESP32S3, and ethernet adapter
- [6chan_energy_meter_6-addons_ethernet.yaml](/Software/ESPHome/6chan_energy_meter_6-addons_ethernet.yaml) - Main board + 6 add-on boards, 42 current channels using the Lilygo T-ETH Lite ESP32S3, and ethernet adapter

## Common Files
These files contain ESP32 and Wifi setup info that is common to all of the above examples. They are only accessed when re-building or changing your config file. They can be copied to your local ESPHome directory (replace the github URL with !include 6chan_common.yaml) or copy the contents directly to your config file.

## Troubleshooting
If you're experiencing what looks like WiFi connectivity problems - the ESP32 is dropping off of your network, then reconnecting - there are 2 things that could be happening:
- You may be running out of memory because there are too many sensors or templates in your config file, causing the ESP32 to restart. See [here for instructions on logging memory usage](https://esphome.io/components/debug.html). If you find that your ESP32 is running out of memory, it is recommended to consolidate the amount of sensors/templates that are in your config file. 
- The ESP32 is too far away from your router or the WiFi signal is being blocked by something. If you suspect this is happening, first check the "Energy Meter WiFi" sensor value. If it appears to be low, around -80 or less, add the following to your config to see if the ESP32 is restarting or losing it's WiFi connection:
```
text_sensor:
  - platform: template
    name: "Energy Meter Uptime"
    id: uptimes
    icon: mdi:clock-start
    entity_category: diagnostic
    lambda: |-
      int seconds = (id(uptime_sensor).state);
      int days = seconds / (24 * 3600);
      seconds = seconds % (24 * 3600);
      int hours = seconds / 3600;
      seconds = seconds % 3600;
      int minutes = seconds /  60;
      seconds = seconds % 60;
      if ( days > 3650 ) {
        return { "Starting up" };
      } else if ( days ) {
        return { (String(days) +"d " + String(hours) +"h " + String(minutes) +"m "+ String(seconds) +"s").c_str() };
      } else if ( hours ) {
        return { (String(hours) +"h " + String(minutes) +"m "+ String(seconds) +"s").c_str() };
      } else if ( minutes ) {
        return { (String(minutes) +"m "+ String(seconds) +"s").c_str() };
      } else {
        return { (String(seconds) +"s").c_str() };
      }
```
