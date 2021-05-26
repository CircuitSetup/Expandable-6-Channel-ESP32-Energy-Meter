/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * Modified to use with the CircuitSetup.us energy meters by jdeglavina
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _EMONESP_CONFIG_H
#define _EMONESP_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>             // Save config settings
#include <ConfigJson.h> //https://github.com/jeremypoulter/ConfigJson.git
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// -------------------------------------------------------------------
// Load and save the EmonESP config.
//
// This initial implementation saves the config to the EEPROM area of flash
// -------------------------------------------------------------------

//define how many boards and inputs are used
#define NUM_BOARDS 7
#define NUM_INPUTS 6

#define NUM_CHANNELS (NUM_BOARDS*NUM_INPUTS)

// Global config varables
extern String node_type;
extern int node_id;
extern String node_name;
extern String node_describe;
extern String node_description;

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
extern int mqtt_port;
extern String mqtt_topic;
extern String mqtt_user;
extern String mqtt_pass;
extern String mqtt_feed_prefix;

// Calibration Settings for CircuitSetup energy meter
extern unsigned short voltage_cal;
extern unsigned short voltage2_cal;
extern int freq_cal;
extern unsigned short gain_cal[NUM_BOARDS];
extern unsigned short ct_cal[NUM_CHANNELS];
extern float cur_mul[NUM_CHANNELS];
extern float pow_mul[NUM_CHANNELS];
#ifdef SOLAR_METER
extern int svoltage_cal;
extern int sct1_cal;
extern int sct2_cal;
#endif

// Timer Settings 
extern int timer_start1;
extern int timer_stop1;
extern int timer_start2;
extern int timer_stop2;
extern int time_offset;

extern int voltage_output;

extern String ctrl_mode;
extern bool ctrl_update;
extern bool ctrl_state;

// 24-bits of Flags
extern uint32_t flags;

#define CONFIG_SERVICE_EMONCMS  (1 << 0)
#define CONFIG_SERVICE_MQTT     (1 << 1)
#define CONFIG_CTRL_UPDATE      (1 << 2)
#define CONFIG_CTRL_STATE       (1 << 3)

inline bool config_emoncms_enabled() {
  return CONFIG_SERVICE_EMONCMS == (flags & CONFIG_SERVICE_EMONCMS);
}

inline bool config_mqtt_enabled() {
  return CONFIG_SERVICE_MQTT == (flags & CONFIG_SERVICE_MQTT);
}

inline bool config_ctrl_update() {
  return CONFIG_CTRL_UPDATE == (flags & CONFIG_CTRL_UPDATE);
}

inline bool config_ctrl_state() {
  return CONFIG_CTRL_STATE == (flags & CONFIG_CTRL_STATE);
}
// -------------------------------------------------------------------
// Load saved settings
// -------------------------------------------------------------------
extern void config_load_settings();
extern void config_load_v1_settings();

// -------------------------------------------------------------------
// Save the EmonCMS server details
// -------------------------------------------------------------------
extern void config_save_emoncms(bool enable, String server, String path, String node, String apikey, String fingerprint);

// -------------------------------------------------------------------
// Save the MQTT broker details
// -------------------------------------------------------------------
extern void config_save_mqtt(bool enable, String server, int port, String topic, String prefix, String user, String pass);
extern void config_save_mqtt_server(String server);

// -------------------------------------------------------------------
// Save the Calibration details
// -------------------------------------------------------------------
#ifdef SOLAR_METER
extern void config_save_cal(int voltage, int ct1, int ct2, int freq, int gain, int svoltage, int sct1, int sct2);
#else
extern void config_save_cal(int voltage, int ct1, int ct2, int freq, int gain);
#endif

// -------------------------------------------------------------------
// Save the admin/web interface details
// -------------------------------------------------------------------
extern void config_save_admin(String user, String pass);

// -------------------------------------------------------------------
// Save the timer interface details
// -------------------------------------------------------------------
extern void config_save_timer(int start1, int stop1, int start2, int stop2, int qvoltage_output, int qtime_offset);
extern void config_save_voltage_output(int qvoltage_output, int save_to_eeprom);

// -------------------------------------------------------------------
// Save the Wifi details
// -------------------------------------------------------------------
extern void config_save_wifi(String qsid, String qpass);

// -------------------------------------------------------------------
// Reset the config back to defaults
// -------------------------------------------------------------------
extern void config_reset();

void config_set(const char *name, uint32_t val);
void config_set(const char *name, String val);
void config_set(const char *name, bool val);
void config_set(const char *name, double val);

// Read config settings from JSON object
bool config_deserialize(String& json);
bool config_deserialize(const char *json);
bool config_deserialize(DynamicJsonDocument &doc);
void config_commit();

// Write config settings to JSON object
bool config_serialize(String& json, bool longNames = true, bool compactOutput = false, bool hideSecrets = false);
bool config_serialize(DynamicJsonDocument &doc, bool longNames = true, bool compactOutput = false, bool hideSecrets = false);

#endif // _EMONESP_CONFIG_H
