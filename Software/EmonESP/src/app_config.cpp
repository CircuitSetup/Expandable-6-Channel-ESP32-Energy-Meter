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

#include "app_config.h"
#include "emonesp.h"
#include "espal.h"
#include "mqtt.h"
#include "emoncms.h"
#include "input.h"
#include "energy_meter.h"

#define EEPROM_SIZE     4096
#define CHECKSUM_SEED    128

static int getNodeId() {
  #ifdef ESP32
  unsigned long chip_id = ESP.getEfuseMac();
  #else
  unsigned long chip_id = ESP.getChipId();
  #endif
  DBUGVAR(chip_id);
  int chip_tmp = chip_id / 10000;
  chip_tmp = chip_tmp * 10000;
  DBUGVAR(chip_tmp);
  return (chip_id - chip_tmp);
}

String node_type = NODE_TYPE;
String node_description = NODE_DESCRIPTION;
int node_id = getNodeId();
String node_name_default = node_type + String(node_id);
String node_name = "";
String node_describe = "";

// Wifi Network Strings
String esid = "";
String epass = "";

// Web server authentication (leave blank for none)
String www_username = "";
String www_password = "";

// EMONCMS SERVER strings
String emoncms_server = "";
String emoncms_path = "";
String emoncms_node = "";
String emoncms_apikey = "";
String emoncms_fingerprint = "";

// MQTT Settings
String mqtt_server = "";
int mqtt_port = 1883;
String mqtt_topic = "";
String mqtt_user = "";
String mqtt_pass = "";
String mqtt_feed_prefix = "";

// Calibration Settings
unsigned short voltage_cal = 0;
unsigned short voltage2_cal = 0;
int freq_cal = 0;
unsigned short gain_cal[NUM_BOARDS] = { 0 }; //PGA gain
String ct_name[NUM_CHANNELS] = "";
unsigned short ct_cal[NUM_CHANNELS] = { 0 }; // current gain
float cur_mul[NUM_CHANNELS] = { 0.0 };
float pow_mul[NUM_CHANNELS] = { 0.0 };
#ifdef SOLAR_METER
int svoltage_cal = 0;
int sct1_cal = 0;
int sct2_cal = 0;
#endif

// Timer Settings 
int timer_start1 = 0;
int timer_stop1 = 0;
int timer_start2 = 0;
int timer_stop2 = 0;

int voltage_output = 0;

String ctrl_mode = "Off";
bool ctrl_update = 0;
bool ctrl_state = 0;
int time_offset = 0;

uint32_t flags;

void config_changed(String name);

ConfigOptDefenition<uint32_t> flagsOpt = ConfigOptDefenition<uint32_t>(flags, 0, "flags", "f");

ConfigOpt *opts[] =
{
// Wifi Network Strings, 0
  new ConfigOptDefenition<String>(esid, "", "ssid", "ws"),
  new ConfigOptSecret(epass, "", "pass", "wp"),

// Web server authentication (leave blank for none), 2
  new ConfigOptDefenition<String>(www_username, "", "www_username", "au"),
  new ConfigOptSecret(www_password, "", "www_password", "ap"),

// Advanced settings, 4
  new ConfigOptDefenition<String>(node_name, node_name_default, "hostname", "hn"),

// EMONCMS SERVER strings, 5
  new ConfigOptDefenition<String>(emoncms_server, "emoncms.org", "emoncms_server", "es"),
  new ConfigOptDefenition<String>(emoncms_path, "", "emoncms_path", "ep"),
  new ConfigOptDefenition<String>(emoncms_node, node_name, "emoncms_node", "en"),
  new ConfigOptSecret(emoncms_apikey, "", "emoncms_apikey", "ea"),
  new ConfigOptDefenition<String>(emoncms_fingerprint, "", "emoncms_fingerprint", "ef"),

// MQTT Settings, 10
  new ConfigOptDefenition<String>(mqtt_server, "emonpi", "mqtt_server", "ms"),
  new ConfigOptDefenition<int>(mqtt_port, 1883, "mqtt_port", "mpt"),
  new ConfigOptDefenition<String>(mqtt_topic, "emonesp", "mqtt_topic", "mt"),
  new ConfigOptDefenition<String>(mqtt_user, "emonpi", "mqtt_user", "mu"),
  new ConfigOptSecret(mqtt_pass, "emonpimqtt2016", "mqtt_pass", "mp"),
  new ConfigOptDefenition<String>(mqtt_feed_prefix, "", "mqtt_feed_prefix", "mp"),

// Calibration settings
/*
  new ConfigOptDefenition<int>(voltage_cal, 3920, "voltage_cal", "cv"),
  new ConfigOptDefenition<int>(ct1_cal, 39473, "ct1_cal", "ct1"),
  new ConfigOptDefenition<int>(ct2_cal, 39473, "ct2_cal", "ct2"),
  new ConfigOptDefenition<int>(freq_cal, 4485, "freq_cal", "cf"),
  new ConfigOptDefenition<int>(gain_cal, 21, "gain_cal", "cg"),
  #ifdef SOLAR_METER
  new ConfigOptDefenition<int>(svoltage_cal, 3920, "svoltage_cal", "scv"),
  new ConfigOptDefenition<int>(sct1_cal, 39473, "sct1_cal", "sct1"),
  new ConfigOptDefenition<int>(sct2_cal, 39473, "sct2_cal", "sct2"),
  #endif
*/
  //new calibration settings
  //new ConfigOptDefenition<unsigned short>(voltage_cal, VOLTAGE_GAIN, "voltage_cal1", "cv1"),
  //new ConfigOptDefenition<unsigned short>(voltage2_cal, VOLTAGE_GAIN, "voltage_cal2", "cv2"),

  new ConfigOptDefenition<unsigned short>((ct_cal[0*NUM_INPUTS+0]), 0, "ic1_current_gain1", "ct1_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[0*NUM_INPUTS+1]), 0, "ic1_current_gain2", "ct1_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[0*NUM_INPUTS+2]), 0, "ic1_current_gain3", "ct1_3"),
  new ConfigOptDefenition<unsigned short>((ct_cal[0*NUM_INPUTS+3]), 0, "ic2_current_gain1", "ct2_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[0*NUM_INPUTS+4]), 0, "ic2_current_gain2", "ct2_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[0*NUM_INPUTS+5]), 0, "ic2_current_gain3", "ct2_3"),

  new ConfigOptDefenition<unsigned short>((gain_cal[0]), 0, "gain1_cal", "gain1_cal"),

  new ConfigOptDefenition<float>((cur_mul[0*NUM_INPUTS+0]), 0, "ic1_current_multiplier1", "cur_mul1_1"),
  new ConfigOptDefenition<float>((cur_mul[0*NUM_INPUTS+1]), 0, "ic1_current_multiplier2", "cur_mul1_2"),
  new ConfigOptDefenition<float>((cur_mul[0*NUM_INPUTS+2]), 0, "ic1_current_multiplier3", "cur_mul1_3"),
  new ConfigOptDefenition<float>((cur_mul[0*NUM_INPUTS+3]), 0, "ic2_current_multiplier1", "cur_mul2_1"),
  new ConfigOptDefenition<float>((cur_mul[0*NUM_INPUTS+4]), 0, "ic2_current_multiplier2", "cur_mul2_2"),
  new ConfigOptDefenition<float>((cur_mul[0*NUM_INPUTS+5]), 0, "ic2_current_multiplier3", "cur_mul2_3"),

  new ConfigOptDefenition<float>((pow_mul[0*NUM_INPUTS+0]), 0, "ic1_power_multiplier1", "pow_mul1_1"),
  new ConfigOptDefenition<float>((pow_mul[0*NUM_INPUTS+1]), 0, "ic1_power_multiplier2", "pow_mul1_2"),
  new ConfigOptDefenition<float>((pow_mul[0*NUM_INPUTS+2]), 0, "ic1_power_multiplier3", "pow_mul1_3"),
  new ConfigOptDefenition<float>((pow_mul[0*NUM_INPUTS+3]), 0, "ic2_power_multiplier1", "pow_mul2_1"),
  new ConfigOptDefenition<float>((pow_mul[0*NUM_INPUTS+4]), 0, "ic2_power_multiplier2", "pow_mul2_2"),
  new ConfigOptDefenition<float>((pow_mul[0*NUM_INPUTS+5]), 0, "ic2_power_multiplier3", "pow_mul2_3"),

  #if NUM_BOARDS > 1
  
  new ConfigOptDefenition<unsigned short>((ct_cal[1*NUM_INPUTS+0]), 0, "ic3_current_gain1", "ct3_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[1*NUM_INPUTS+1]), 0, "ic3_current_gain2", "ct3_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[1*NUM_INPUTS+2]), 0, "ic3_current_gain3", "ct3_3"),
  new ConfigOptDefenition<unsigned short>((ct_cal[1*NUM_INPUTS+3]), 0, "ic4_current_gain1", "ct4_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[1*NUM_INPUTS+4]), 0, "ic4_current_gain2", "ct4_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[1*NUM_INPUTS+5]), 0, "ic4_current_gain3", "ct4_3"),

  new ConfigOptDefenition<unsigned short>((gain_cal[1]), 0, "gain2_cal", "gain2_cal"),

  new ConfigOptDefenition<float>((cur_mul[1*NUM_INPUTS+0]), 0, "ic3_current_multiplier1", "cur_mul3_1"),
  new ConfigOptDefenition<float>((cur_mul[1*NUM_INPUTS+1]), 0, "ic3_current_multiplier2", "cur_mul3_2"),
  new ConfigOptDefenition<float>((cur_mul[1*NUM_INPUTS+2]), 0, "ic3_current_multiplier3", "cur_mul3_3"),
  new ConfigOptDefenition<float>((cur_mul[1*NUM_INPUTS+3]), 0, "ic4_current_multiplier1", "cur_mul4_1"),
  new ConfigOptDefenition<float>((cur_mul[1*NUM_INPUTS+4]), 0, "ic4_current_multiplier2", "cur_mul4_2"),
  new ConfigOptDefenition<float>((cur_mul[1*NUM_INPUTS+5]), 0, "ic4_current_multiplier3", "cur_mul4_3"),

  new ConfigOptDefenition<float>((pow_mul[1*NUM_INPUTS+0]), 0, "ic3_power_multiplier1", "pow_mul3_1"),
  new ConfigOptDefenition<float>((pow_mul[1*NUM_INPUTS+1]), 0, "ic3_power_multiplier2", "pow_mul3_2"),
  new ConfigOptDefenition<float>((pow_mul[1*NUM_INPUTS+2]), 0, "ic3_power_multiplier3", "pow_mul3_3"),
  new ConfigOptDefenition<float>((pow_mul[1*NUM_INPUTS+3]), 0, "ic4_power_multiplier1", "pow_mul4_1"),
  new ConfigOptDefenition<float>((pow_mul[1*NUM_INPUTS+4]), 0, "ic4_power_multiplier2", "pow_mul4_2"),
  new ConfigOptDefenition<float>((pow_mul[1*NUM_INPUTS+5]), 0, "ic4_power_multiplier3", "pow_mul4_3"),

  #endif

  #if NUM_BOARDS > 2
  
  new ConfigOptDefenition<unsigned short>((ct_cal[2*NUM_INPUTS+0]), 0, "ic5_current_gain1", "ct5_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[2*NUM_INPUTS+1]), 0, "ic5_current_gain2", "ct5_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[2*NUM_INPUTS+2]), 0, "ic5_current_gain3", "ct5_3"),
  new ConfigOptDefenition<unsigned short>((ct_cal[2*NUM_INPUTS+3]), 0, "ic6_current_gain1", "ct6_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[2*NUM_INPUTS+4]), 0, "ic6_current_gain2", "ct6_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[2*NUM_INPUTS+5]), 0, "ic6_current_gain3", "ct6_3"),

  new ConfigOptDefenition<unsigned short>((gain_cal[2]), 0, "gain3_cal", "gain3_cal"),

  new ConfigOptDefenition<float>((cur_mul[2*NUM_INPUTS+0]), 0, "ic5_current_multiplier1", "cur_mul5_1"),
  new ConfigOptDefenition<float>((cur_mul[2*NUM_INPUTS+1]), 0, "ic5_current_multiplier2", "cur_mul5_2"),
  new ConfigOptDefenition<float>((cur_mul[2*NUM_INPUTS+2]), 0, "ic5_current_multiplier3", "cur_mul5_3"),
  new ConfigOptDefenition<float>((cur_mul[2*NUM_INPUTS+3]), 0, "ic6_current_multiplier1", "cur_mul6_1"),
  new ConfigOptDefenition<float>((cur_mul[2*NUM_INPUTS+4]), 0, "ic6_current_multiplier2", "cur_mul6_2"),
  new ConfigOptDefenition<float>((cur_mul[2*NUM_INPUTS+5]), 0, "ic6_current_multiplier3", "cur_mul6_3"),

  new ConfigOptDefenition<float>((pow_mul[2*NUM_INPUTS+0]), 0, "ic5_power_multiplier1", "pow_mul5_1"),
  new ConfigOptDefenition<float>((pow_mul[2*NUM_INPUTS+1]), 0, "ic5_power_multiplier2", "pow_mul5_2"),
  new ConfigOptDefenition<float>((pow_mul[2*NUM_INPUTS+2]), 0, "ic5_power_multiplier3", "pow_mul5_3"),
  new ConfigOptDefenition<float>((pow_mul[2*NUM_INPUTS+3]), 0, "ic6_power_multiplier1", "pow_mul6_1"),
  new ConfigOptDefenition<float>((pow_mul[2*NUM_INPUTS+4]), 0, "ic6_power_multiplier2", "pow_mul6_2"),
  new ConfigOptDefenition<float>((pow_mul[2*NUM_INPUTS+5]), 0, "ic6_power_multiplier3", "pow_mul6_3"),

  #endif

  #if NUM_BOARDS > 3
  
  new ConfigOptDefenition<unsigned short>((ct_cal[3*NUM_INPUTS+0]), 0, "ic7_current_gain1", "ct7_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[3*NUM_INPUTS+1]), 0, "ic7_current_gain2", "ct7_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[3*NUM_INPUTS+2]), 0, "ic7_current_gain3", "ct7_3"),
  new ConfigOptDefenition<unsigned short>((ct_cal[3*NUM_INPUTS+3]), 0, "ic8_current_gain1", "ct8_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[3*NUM_INPUTS+4]), 0, "ic8_current_gain2", "ct8_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[3*NUM_INPUTS+5]), 0, "ic8_current_gain3", "ct8_3"),

  new ConfigOptDefenition<unsigned short>((gain_cal[3]), 0, "gain4_cal", "gain4_cal"),

  new ConfigOptDefenition<float>((cur_mul[3*NUM_INPUTS+0]), 0, "ic7_current_multiplier1", "cur_mul7_1"),
  new ConfigOptDefenition<float>((cur_mul[3*NUM_INPUTS+1]), 0, "ic7_current_multiplier2", "cur_mul7_2"),
  new ConfigOptDefenition<float>((cur_mul[3*NUM_INPUTS+2]), 0, "ic7_current_multiplier3", "cur_mul7_3"),
  new ConfigOptDefenition<float>((cur_mul[3*NUM_INPUTS+3]), 0, "ic8_current_multiplier1", "cur_mul8_1"),
  new ConfigOptDefenition<float>((cur_mul[3*NUM_INPUTS+4]), 0, "ic8_current_multiplier2", "cur_mul8_2"),
  new ConfigOptDefenition<float>((cur_mul[3*NUM_INPUTS+5]), 0, "ic8_current_multiplier3", "cur_mul8_3"),

  new ConfigOptDefenition<float>((pow_mul[3*NUM_INPUTS+0]), 0, "ic7_power_multiplier1", "pow_mul7_1"),
  new ConfigOptDefenition<float>((pow_mul[3*NUM_INPUTS+1]), 0, "ic7_power_multiplier2", "pow_mul7_2"),
  new ConfigOptDefenition<float>((pow_mul[3*NUM_INPUTS+2]), 0, "ic7_power_multiplier3", "pow_mul7_3"),
  new ConfigOptDefenition<float>((pow_mul[3*NUM_INPUTS+3]), 0, "ic8_power_multiplier1", "pow_mul8_1"),
  new ConfigOptDefenition<float>((pow_mul[3*NUM_INPUTS+4]), 0, "ic8_power_multiplier2", "pow_mul8_2"),
  new ConfigOptDefenition<float>((pow_mul[3*NUM_INPUTS+5]), 0, "ic8_power_multiplier3", "pow_mul8_3"),

  #endif

  #if NUM_BOARDS > 4
  
  new ConfigOptDefenition<unsigned short>((ct_cal[4*NUM_INPUTS+0]), 0, "ic9_current_gain1", "ct9_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[4*NUM_INPUTS+1]), 0, "ic9_current_gain2", "ct9_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[4*NUM_INPUTS+2]), 0, "ic9_current_gain3", "ct9_3"),
  new ConfigOptDefenition<unsigned short>((ct_cal[4*NUM_INPUTS+3]), 0, "ic10_current_gain1", "ct10_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[4*NUM_INPUTS+4]), 0, "ic10_current_gain2", "ct10_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[4*NUM_INPUTS+5]), 0, "ic10_current_gain3", "ct10_3"),

  new ConfigOptDefenition<unsigned short>((gain_cal[4]), 0, "gain5_cal", "gain5_cal"),

  new ConfigOptDefenition<float>((cur_mul[4*NUM_INPUTS+0]), 0, "ic9_current_multiplier1", "cur_mul9_1"),
  new ConfigOptDefenition<float>((cur_mul[4*NUM_INPUTS+1]), 0, "ic9_current_multiplier2", "cur_mul9_2"),
  new ConfigOptDefenition<float>((cur_mul[4*NUM_INPUTS+2]), 0, "ic9_current_multiplier3", "cur_mul9_3"),
  new ConfigOptDefenition<float>((cur_mul[4*NUM_INPUTS+3]), 0, "ic10_current_multiplier1", "cur_mul10_1"),
  new ConfigOptDefenition<float>((cur_mul[4*NUM_INPUTS+4]), 0, "ic10_current_multiplier2", "cur_mul10_2"),
  new ConfigOptDefenition<float>((cur_mul[4*NUM_INPUTS+5]), 0, "ic10_current_multiplier3", "cur_mul10_3"),

  new ConfigOptDefenition<float>((pow_mul[4*NUM_INPUTS+0]), 0, "ic9_power_multiplier1", "pow_mul9_1"),
  new ConfigOptDefenition<float>((pow_mul[4*NUM_INPUTS+1]), 0, "ic9_power_multiplier2", "pow_mul9_2"),
  new ConfigOptDefenition<float>((pow_mul[4*NUM_INPUTS+2]), 0, "ic9_power_multiplier3", "pow_mul9_3"),
  new ConfigOptDefenition<float>((pow_mul[4*NUM_INPUTS+3]), 0, "ic10_power_multiplier1", "pow_mul10_1"),
  new ConfigOptDefenition<float>((pow_mul[4*NUM_INPUTS+4]), 0, "ic10_power_multiplier2", "pow_mul10_2"),
  new ConfigOptDefenition<float>((pow_mul[4*NUM_INPUTS+5]), 0, "ic10_power_multiplier3", "pow_mul10_3"),

  #endif

    #if NUM_BOARDS > 5
  
  new ConfigOptDefenition<unsigned short>((ct_cal[5*NUM_INPUTS+0]), 0, "ic11_current_gain1", "ct11_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[5*NUM_INPUTS+1]), 0, "ic11_current_gain2", "ct11_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[5*NUM_INPUTS+2]), 0, "ic11_current_gain3", "ct11_3"),
  new ConfigOptDefenition<unsigned short>((ct_cal[5*NUM_INPUTS+3]), 0, "ic12_current_gain1", "ct12_1"),
  new ConfigOptDefenition<unsigned short>((ct_cal[5*NUM_INPUTS+4]), 0, "ic12_current_gain2", "ct12_2"),
  new ConfigOptDefenition<unsigned short>((ct_cal[5*NUM_INPUTS+5]), 0, "ic12_current_gain3", "ct12_3"),

  new ConfigOptDefenition<unsigned short>((gain_cal[5]), 0, "gain6_cal", "gain6_cal"),

  new ConfigOptDefenition<float>((cur_mul[5*NUM_INPUTS+0]), 0, "ic11_current_multiplier1", "cur_mul11_1"),
  new ConfigOptDefenition<float>((cur_mul[5*NUM_INPUTS+1]), 0, "ic11_current_multiplier2", "cur_mul11_2"),
  new ConfigOptDefenition<float>((cur_mul[5*NUM_INPUTS+2]), 0, "ic11_current_multiplier3", "cur_mul11_3"),
  new ConfigOptDefenition<float>((cur_mul[5*NUM_INPUTS+3]), 0, "ic12_current_multiplier1", "cur_mul12_1"),
  new ConfigOptDefenition<float>((cur_mul[5*NUM_INPUTS+4]), 0, "ic12_current_multiplier2", "cur_mul12_2"),
  new ConfigOptDefenition<float>((cur_mul[5*NUM_INPUTS+5]), 0, "ic12_current_multiplier3", "cur_mul12_3"),

  new ConfigOptDefenition<float>((pow_mul[5*NUM_INPUTS+0]), 0, "ic11_power_multiplier1", "pow_mul11_1"),
  new ConfigOptDefenition<float>((pow_mul[5*NUM_INPUTS+1]), 0, "ic11_power_multiplier2", "pow_mul11_2"),
  new ConfigOptDefenition<float>((pow_mul[5*NUM_INPUTS+2]), 0, "ic11_power_multiplier3", "pow_mul11_3"),
  new ConfigOptDefenition<float>((pow_mul[5*NUM_INPUTS+3]), 0, "ic12_power_multiplier1", "pow_mul12_1"),
  new ConfigOptDefenition<float>((pow_mul[5*NUM_INPUTS+4]), 0, "ic12_power_multiplier2", "pow_mul12_2"),
  new ConfigOptDefenition<float>((pow_mul[5*NUM_INPUTS+5]), 0, "ic12_power_multiplier3", "pow_mul12_3"),

  #endif

// Timer Settings, 16
  new ConfigOptDefenition<int>(timer_start1, 0, "timer_start1", "tsr1"),
  new ConfigOptDefenition<int>(timer_stop1, 0, "timer_stop1", "tsp1"),
  new ConfigOptDefenition<int>(timer_start2, 0, "timer_start2", "tsr2"),
  new ConfigOptDefenition<int>(timer_stop2, 0, "timer_stop2", "tsp2"),
  new ConfigOptDefenition<int>(time_offset, 0, "time_offset", "to"),

  new ConfigOptDefenition<int>(voltage_output, 0, "voltage_output", "vo"),

  new ConfigOptDefenition<String>(ctrl_mode, "Off", "ctrl_mode", "cm"),

// Flags, 23
  &flagsOpt,

// Virtual Options, 24
  new ConfigOptVirtualBool(flagsOpt, CONFIG_SERVICE_EMONCMS, CONFIG_SERVICE_EMONCMS, "emoncms_enabled", "ee"),
  new ConfigOptVirtualBool(flagsOpt, CONFIG_SERVICE_MQTT, CONFIG_SERVICE_MQTT, "mqtt_enabled", "me"),
  new ConfigOptVirtualBool(flagsOpt, CONFIG_CTRL_UPDATE, CONFIG_CTRL_UPDATE, "ctrl_update", "ce"),
  new ConfigOptVirtualBool(flagsOpt, CONFIG_CTRL_STATE, CONFIG_CTRL_STATE, "ctrl_state", "cs")
};

ConfigJson appconfig(opts, sizeof(opts) / sizeof(opts[0]), EEPROM_SIZE);

// -------------------------------------------------------------------
// Reset EEPROM, wipes all settings
// -------------------------------------------------------------------
void ResetEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  //DEBUG.println("Erasing EEPROM");
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0xff);
    //DEBUG.print("#");
  }
  EEPROM.end();
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void config_load_settings() {
  appconfig.onChanged(config_changed);

  if(!appconfig.load()) {
    DBUGF("No JSON config found, trying v1 settings");
    config_load_v1_settings();
  }
}

void config_changed(String name) {
  DBUGF("%s changed", name.c_str());

  if(name.equals(F("flags"))) {
    if(mqtt_connected() != config_mqtt_enabled()) {
      mqtt_restart();
    }
    if(emoncms_connected != config_emoncms_enabled()) {
      emoncms_updated = true;
    } 
  } else if(name.startsWith(F("mqtt_"))) {
    mqtt_restart();
  } else if(name.startsWith(F("emoncms_"))) {
    emoncms_updated = true;
  }
}

void config_commit() {
  appconfig.commit();
}

bool config_deserialize(String& json) {
  return appconfig.deserialize(json.c_str());
}

bool config_deserialize(const char *json) {
  return appconfig.deserialize(json);
}

bool config_deserialize(DynamicJsonDocument &doc) {
  return appconfig.deserialize(doc);
}

bool config_serialize(String& json, bool longNames, bool compactOutput, bool hideSecrets) {
  return appconfig.serialize(json, longNames, compactOutput, hideSecrets);
}

bool config_serialize(DynamicJsonDocument &doc, bool longNames, bool compactOutput, bool hideSecrets) {
  return appconfig.serialize(doc, longNames, compactOutput, hideSecrets);
}

void config_set(const char *name, uint32_t val) {
  appconfig.set(name, val);
}
void config_set(const char *name, String val) {
  appconfig.set(name, val);
}
void config_set(const char *name, bool val) {
  appconfig.set(name, val);
}
void config_set(const char *name, double val) {
  appconfig.set(name, val);
}

void config_save_emoncms(bool enable, String server, String path, String node, String apikey,
                    String fingerprint) {
  uint32_t newflags = flags & ~CONFIG_SERVICE_EMONCMS;
  if(enable) {
    newflags |= CONFIG_SERVICE_EMONCMS;
  }

  appconfig.set(F("emoncms_server"), server);
  appconfig.set(F("emoncms_path"), path);
  appconfig.set(F("emoncms_node"), node);
  appconfig.set(F("emoncms_apikey"), apikey);
  appconfig.set(F("emoncms_fingerprint"), fingerprint);
  appconfig.set(F("flags"), newflags);
  appconfig.commit();
}

void config_save_mqtt(bool enable, String server, int port, String topic, String prefix, String user, String pass) {
  uint32_t newflags = flags & ~CONFIG_SERVICE_MQTT;
  if(enable) {
    newflags |= CONFIG_SERVICE_MQTT;
  }

  appconfig.set(F("mqtt_server"), server);
  appconfig.set(F("mqtt_port"), port);
  appconfig.set(F("mqtt_topic"), topic);
  appconfig.set(F("mqtt_prefix"), prefix);
  appconfig.set(F("mqtt_user"), user);
  appconfig.set(F("mqtt_pass"), pass);
  appconfig.set(F("flags"), newflags);
  appconfig.commit();
}

void config_save_mqtt_server(String server) {
  appconfig.set(F("mqtt_server"), server);
  appconfig.commit();
}
/*
void config_save_cal(int voltage, int ct1, int ct2, int freq, int gain
#ifdef SOLAR_METER
  , int svoltage, int sct1, int sct2);
#else
  )
#endif
{
  appconfig.set(F("voltage_cal"), voltage);
  appconfig.set(F("ct1_cal"), ct1);
  appconfig.set(F("ct2_cal"), ct2);
  appconfig.set(F("freq_cal"), freq);
  appconfig.set(F("gain_cal"), gain);
  #ifdef SOLAR_METER
  appconfig.set(F("svoltage_cal"), svoltage);
  appconfig.set(F("sct1_cal"), sct1);
  appconfig.set(F("sct2_cal"), sct2);
  #endif
  appconfig.commit();
}*/

//for CircuitSetup 6 channel energy meter
void config_save_cal(AsyncWebServerRequest * request)
{
  char req[12];

  EEPROM.begin(EEPROM_SIZE);

  voltage_cal = request->arg("voltage_cal").toInt();
  voltage2_cal = request->arg("voltage2_cal").toInt();
  freq_cal = request->arg("freq_cal").toInt();

  appconfig.set(F("voltage_cal"), voltage_cal);
  appconfig.set(F("voltage2_cal"), voltage2_cal);
  appconfig.set(F("freq_cal"), freq_cal);
  //EEPROM_write_ushort(EEPROM_CAL_VOLTAGE_START, voltage_cal);
  //EEPROM_write_ushort(EEPROM_CAL_VOLTAGE2_START, voltage2_cal);
  //EEPROM_write_ushort(EEPROM_CAL_FREQ_START, freq_cal);

  for (int i = 0; i < NUM_CHANNELS; i++)
  {
    sprintf(req, "ct%d_name", i+1);
    ct_name[i] = request->arg(req);
    //EEPROM_write_string(EEPROM_NAME_CT_START + (i*EEPROM_NAME_CT_SIZE), EEPROM_NAME_CT_SIZE, ct_name[i]);
    appconfig.set(F(req), ct_name[i]);
    sprintf(req, "ct%d_cal", i+1);
    ct_cal[i] = request->arg(req).toInt();
    appconfig.set(F(req), ct_cal[i]);
    //EEPROM_write_ushort(EEPROM_CAL_CT_START + (i*EEPROM_CAL_CT_SIZE), ct_cal[i]);
    sprintf(req, "cur%d_mul", i+1);
    cur_mul[i] = request->arg(req).toFloat();
    appconfig.set(F(req), cur_mul[i]);
    //EEPROM_write_float(EEPROM_CUR_MUL_START + (i*EEPROM_CUR_MUL_SIZE), cur_mul[i]);
    sprintf(req, "pow%d_mul", i+1);
    pow_mul[i] = request->arg(req).toFloat();
    appconfig.set(F(req), pow_mul[i]);
    //EEPROM_write_float(EEPROM_POW_MUL_START + (i*EEPROM_POW_MUL_SIZE), pow_mul[i]);
  }

  for (int i = 0; i < NUM_BOARDS; i++)
  {
    byte pgaA, pgaB, pgaC;

    sprintf(req, "gain%d_cal", i*NUM_INPUTS+1);
    pgaA = request->arg(req).toInt() >> 1;
    sprintf(req, "gain%d_cal", i*NUM_INPUTS+2);
    pgaB = request->arg(req).toInt() >> 1;
    sprintf(req, "gain%d_cal", i*NUM_INPUTS+3);
    pgaC = request->arg(req).toInt() >> 1;

    gain_cal[i] = pgaA | (pgaB << 2) | (pgaC << 4);

    sprintf(req, "gain%d_cal", i*NUM_INPUTS+4);
    pgaA = request->arg(req).toInt() >> 1;
    sprintf(req, "gain%d_cal", i*NUM_INPUTS+5);
    pgaB = request->arg(req).toInt() >> 1;
    sprintf(req, "gain%d_cal", i*NUM_INPUTS+6);
    pgaC = request->arg(req).toInt() >> 1;

    gain_cal[i] |= (pgaA << 8) | (pgaB << 10) | (pgaC << 12);
    sprintf(req, "gain%d_cal", i);
    appconfig.set(F(req), gain_cal[i]);
    //EEPROM_write_ushort(EEPROM_CAL_GAIN_START + (i*EEPROM_CAL_GAIN_SIZE), gain_cal[i]);
  }
  appconfig.commit();
  //EEPROM.end();
}

void config_save_admin(String user, String pass) {
  appconfig.set(F("www_username"), user);
  appconfig.set(F("www_password"), pass);
  appconfig.commit();
}

void config_save_timer(int start1, int stop1, int start2, int stop2, int qvoltage_output, int qtime_offset) {
  appconfig.set(F("timer_start1"), start1);
  appconfig.set(F("timer_stop1"), stop1);
  appconfig.set(F("timer_start2"), start2);
  appconfig.set(F("timer_stop2"), stop2);
  appconfig.set(F("voltage_output"), qvoltage_output);
  appconfig.set(F("time_offset"), qtime_offset);
  appconfig.commit();
}

void config_save_voltage_output(int qvoltage_output, int save_to_eeprom) {
  voltage_output = qvoltage_output;
  
  if (save_to_eeprom) {
    appconfig.set(F("voltage_output"), qvoltage_output);
    appconfig.commit();
  }
}

void config_save_advanced(String hostname) {
  appconfig.set(F("hostname"), hostname);
  appconfig.commit();
}

void config_save_wifi(String qsid, String qpass) {
  appconfig.set(F("ssid"), qsid);
  appconfig.set(F("pass"), qpass);
  appconfig.commit();
}

void config_save_flags(uint32_t newFlags) {
  appconfig.set(F("flags"), newFlags);
  appconfig.commit();
}

void config_reset() {
  ResetEEPROM();
  appconfig.reset();
}
