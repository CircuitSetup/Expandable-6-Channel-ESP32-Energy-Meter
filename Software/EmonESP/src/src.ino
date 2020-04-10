/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Adaptation of Chris Howells OpenEVSE ESP Wifi
   by Trystan Lea, Glyn Hudson, OpenEnergyMonitor

   Modified to use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

   All adaptation GNU General Public License as below.

   -------------------------------------------------------------------

   This file is part of OpenEnergyMonitor.org project.
   EmonESP is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.
   EmonESP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with EmonESP; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// See energy meter specific configuration in energy_meter.h
#define ENABLE_ENERGY_METER

// If USE_SERIAL_INPUT is defined, EmonESP will check the serial port for
// input. Only enable this if there is input expected on the serial port;
// otherwise it seems to read garbage data.
//#define USE_SERIAL_INPUT

#include "emonesp.h"
#include "config.h"
#include "wifi_new.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"


#ifdef ENABLE_ENERGY_METER
#include "energy_meter.h"
#endif

// #ifndef WIFI_LED
// #define WIFI_LED 4
// #endif

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {

  Serial.begin(115200);
#ifdef DEBUG_SERIAL1
  Serial1.begin(115200);
#endif
  delay(1000);

  // Read saved settings from the config
  config_load_settings();
  delay(500);

  // Initialise the WiFi
  wifi_setup();
  delay(500);

  // Bring up the web server
  web_server_setup();
  delay(500);

#ifdef ENABLE_ENERGY_METER
  energy_meter_setup();
#endif

  DBUGS.println("Server started");

} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  web_server_loop();

#ifdef ENABLE_ENERGY_METER
  energy_meter_loop();
#endif

  String input = "";
  boolean gotInput = input_get(input);
  if (gotInput) {
    DBUGS.println(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (emoncms_apikey != 0 && gotInput) {
      DBUGS.println(input);
      emoncms_publish(input);
    }
    if (mqtt_server != 0) {
      mqtt_loop();
      if (gotInput) {
        mqtt_publish(input);
      }
    }
  }
    //Local intialization. Once its business is done, there is no need to keep it around
    ESP_WiFiManager ESP_wifiManager;
    
    //Check if there is stored WiFi router/password credentials.
    //If not found, device will remain in configuration mode until switched off via webserver.
    DBUGS.print("Opening configuration portal. ");
    esid = ESP_wifiManager.WiFi_SSID();
    if (esid != "")
    {
      ESP_wifiManager.setConfigPortalTimeout(60); //If no access point name has been previously entered disable timeout.
      DBUGS.println("Got stored Credentials. Timeout 60s");
    }
    else
      DBUGS.println("No stored Credentials. No timeout");
    
    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!ESP_wifiManager.startConfigPortal((const char *) ssid.c_str(), password)) 
    {
      DBUGS.println("Not connected to WiFi but continuing anyway.");
    } 
    else 
    {
      //if you get here you have connected to the WiFi
      DBUGS.println("connected...yeey :)");
    }
} // end loop
