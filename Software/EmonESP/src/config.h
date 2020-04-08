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

#ifndef _EMONESP_CONFIG_H
#define _EMONESP_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>                   // Save config settings

// -------------------------------------------------------------------
// Load and save the EmonESP config.
//
// This initial implementation saves the config to the EEPROM area of flash
// -------------------------------------------------------------------

// Global config varables

// Wifi Network Strings
extern String esid;
extern String epass;

// Web server authentication (leave blank for none)
extern String www_username;
extern String www_password;

// EMONCMS SERVER strings
extern String emoncms_server;
extern String emoncms_path;
extern String emoncms_node;
extern String emoncms_apikey;
extern String emoncms_fingerprint;

// MQTT Settings
extern String mqtt_server;
extern String mqtt_topic;
extern String mqtt_user;
extern String mqtt_pass;
extern String mqtt_feed_prefix;

// Calibration Settings for CircuitSetup 6 channel energy meter
extern String voltage_cal;
extern String voltage2_cal;
extern String ct1_cal;
extern String ct2_cal;
extern String ct3_cal;
extern String ct4_cal;
extern String ct5_cal;
extern String ct6_cal;
extern String freq_cal;
extern String gain_cal;


// -------------------------------------------------------------------
// Load saved settings
// -------------------------------------------------------------------
extern void config_load_settings();

// -------------------------------------------------------------------
// Save the EmonCMS server details
// -------------------------------------------------------------------
extern void config_save_emoncms(String server, String path, String node, String apikey, String fingerprint);

// -------------------------------------------------------------------
// Save the MQTT broker details
// -------------------------------------------------------------------
extern void config_save_mqtt(String server, String topic, String prefix, String user, String pass);

// -------------------------------------------------------------------
// Save the Calibration details
// -------------------------------------------------------------------
extern void config_save_cal(String voltage, String voltage2, String ct1, String ct2, String ct3, String ct4, String ct5, String ct6, String freq, String gain);

// -------------------------------------------------------------------
// Save the admin/web interface details
// -------------------------------------------------------------------
extern void config_save_admin(String user, String pass);

// -------------------------------------------------------------------
// Save the Wifi details
// -------------------------------------------------------------------
extern void config_save_wifi(String qsid, String qpass);

// -------------------------------------------------------------------
// Reset the config back to defaults
// -------------------------------------------------------------------
extern void config_reset();

#endif // _EMONESP_CONFIG_H
