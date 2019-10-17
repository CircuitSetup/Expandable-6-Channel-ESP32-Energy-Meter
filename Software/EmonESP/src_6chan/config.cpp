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

#include "emonesp.h"
#include "energy_meter.h"
#include "config.h"

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
String mqtt_topic = "";
String mqtt_user = "";
String mqtt_pass = "";
String mqtt_feed_prefix = "";

// Calibration Settings
String voltage_cal = "";
String voltage2_cal = "";
String ct1_cal = "";
String ct2_cal = "";
String ct3_cal = "";
String ct4_cal = "";
String ct5_cal = "";
String ct6_cal = "";
String freq_cal = "";
String gain_cal = "";

#define EEPROM_ESID_SIZE          32
#define EEPROM_EPASS_SIZE         64
#define EEPROM_EMON_API_KEY_SIZE  33
#define EEPROM_EMON_SERVER_SIZE   32
#define EEPROM_EMON_PATH_SIZE     16
#define EEPROM_EMON_NODE_SIZE     32
#define EEPROM_MQTT_SERVER_SIZE   32
#define EEPROM_MQTT_TOPIC_SIZE    32
#define EEPROM_MQTT_USER_SIZE     32
#define EEPROM_MQTT_PASS_SIZE     64
#define EEPROM_EMON_FINGERPRINT_SIZE  60
#define EEPROM_MQTT_FEED_PREFIX_SIZE  10
#define EEPROM_WWW_USER_SIZE      16
#define EEPROM_WWW_PASS_SIZE      16
#define EEPROM_CAL_VOLTAGE_SIZE   6
#define EEPROM_CAL_VOLTAGE2_SIZE   6
#define EEPROM_CAL_CT1_SIZE       6
#define EEPROM_CAL_CT2_SIZE       6
#define EEPROM_CAL_CT3_SIZE       6
#define EEPROM_CAL_CT4_SIZE       6
#define EEPROM_CAL_CT5_SIZE       6
#define EEPROM_CAL_CT6_SIZE       6
#define EEPROM_CAL_FREQ_SIZE      6
#define EEPROM_CAL_GAIN_SIZE      6
#define EEPROM_SIZE               1024

#define EEPROM_ESID_START         0
#define EEPROM_ESID_END           (EEPROM_ESID_START + EEPROM_ESID_SIZE)
#define EEPROM_EPASS_START        EEPROM_ESID_END
#define EEPROM_EPASS_END          (EEPROM_EPASS_START + EEPROM_EPASS_SIZE)
#define EEPROM_EMON_API_KEY_START EEPROM_EPASS_END
#define EEPROM_EMON_API_KEY_END   (EEPROM_EMON_API_KEY_START + EEPROM_EMON_API_KEY_SIZE)
#define EEPROM_EMON_SERVER_START  EEPROM_EMON_API_KEY_END
#define EEPROM_EMON_SERVER_END    (EEPROM_EMON_SERVER_START + EEPROM_EMON_SERVER_SIZE)
#define EEPROM_EMON_NODE_START    EEPROM_EMON_SERVER_END
#define EEPROM_EMON_NODE_END      (EEPROM_EMON_NODE_START + EEPROM_EMON_NODE_SIZE)
#define EEPROM_MQTT_SERVER_START  EEPROM_EMON_NODE_END
#define EEPROM_MQTT_SERVER_END    (EEPROM_MQTT_SERVER_START + EEPROM_MQTT_SERVER_SIZE)
#define EEPROM_MQTT_TOPIC_START   EEPROM_MQTT_SERVER_END
#define EEPROM_MQTT_TOPIC_END     (EEPROM_MQTT_TOPIC_START + EEPROM_MQTT_TOPIC_SIZE)
#define EEPROM_MQTT_USER_START    EEPROM_MQTT_TOPIC_END
#define EEPROM_MQTT_USER_END      (EEPROM_MQTT_USER_START + EEPROM_MQTT_USER_SIZE)
#define EEPROM_MQTT_PASS_START    EEPROM_MQTT_USER_END
#define EEPROM_MQTT_PASS_END      (EEPROM_MQTT_PASS_START + EEPROM_MQTT_PASS_SIZE)
#define EEPROM_EMON_FINGERPRINT_START  EEPROM_MQTT_PASS_END
#define EEPROM_EMON_FINGERPRINT_END    (EEPROM_EMON_FINGERPRINT_START + EEPROM_EMON_FINGERPRINT_SIZE)
#define EEPROM_MQTT_FEED_PREFIX_START  EEPROM_EMON_FINGERPRINT_END
#define EEPROM_MQTT_FEED_PREFIX_END    (EEPROM_MQTT_FEED_PREFIX_START + EEPROM_MQTT_FEED_PREFIX_SIZE)
#define EEPROM_WWW_USER_START     EEPROM_MQTT_FEED_PREFIX_END
#define EEPROM_WWW_USER_END       (EEPROM_WWW_USER_START + EEPROM_WWW_USER_SIZE)
#define EEPROM_WWW_PASS_START     EEPROM_WWW_USER_END
#define EEPROM_WWW_PASS_END       (EEPROM_WWW_PASS_START + EEPROM_WWW_PASS_SIZE)
#define EEPROM_EMON_PATH_START    EEPROM_WWW_PASS_END
#define EEPROM_EMON_PATH_END      (EEPROM_EMON_PATH_START + EEPROM_EMON_PATH_SIZE)
#define EEPROM_CAL_VOLTAGE_START  EEPROM_EMON_PATH_END
#define EEPROM_CAL_VOLTAGE_END    (EEPROM_CAL_VOLTAGE_START + EEPROM_CAL_VOLTAGE_SIZE)
#define EEPROM_CAL_VOLTAGE2_START  EEPROM_CAL_VOLTAGE_END
#define EEPROM_CAL_VOLTAGE2_END    (EEPROM_CAL_VOLTAGE2_START + EEPROM_CAL_VOLTAGE2_SIZE)
#define EEPROM_CAL_CT1_START   EEPROM_CAL_VOLTAGE2_END
#define EEPROM_CAL_CT1_END     (EEPROM_CAL_CT1_START + EEPROM_CAL_CT1_SIZE)
#define EEPROM_CAL_CT2_START    EEPROM_CAL_CT1_END
#define EEPROM_CAL_CT2_END      (EEPROM_CAL_CT2_START + EEPROM_CAL_CT2_SIZE)
#define EEPROM_CAL_CT3_START   EEPROM_CAL_CT2_END
#define EEPROM_CAL_CT3_END     (EEPROM_CAL_CT3_START + EEPROM_CAL_CT3_SIZE)
#define EEPROM_CAL_CT4_START    EEPROM_CAL_CT3_END
#define EEPROM_CAL_CT4_END      (EEPROM_CAL_CT4_START + EEPROM_CAL_CT4_SIZE)
#define EEPROM_CAL_CT5_START   EEPROM_CAL_CT4_END
#define EEPROM_CAL_CT5_END     (EEPROM_CAL_CT5_START + EEPROM_CAL_CT5_SIZE)
#define EEPROM_CAL_CT6_START    EEPROM_CAL_CT5_END
#define EEPROM_CAL_CT6_END      (EEPROM_CAL_CT6_START + EEPROM_CAL_CT6_SIZE)
#define EEPROM_CAL_FREQ_START    EEPROM_CAL_CT6_END
#define EEPROM_CAL_FREQ_END      (EEPROM_CAL_FREQ_START + EEPROM_CAL_FREQ_SIZE)
#define EEPROM_CAL_GAIN_START    EEPROM_CAL_FREQ_END
#define EEPROM_CAL_GAIN_END      (EEPROM_CAL_GAIN_START + EEPROM_CAL_GAIN_SIZE)
#define EEPROM_CONFIG_END         EEPROM_CAL_GAIN_END

#if EEPROM_CONFIG_END > EEPROM_SIZE
#error EEPROM_SIZE too small
#endif

#define CHECKSUM_SEED 128

// -------------------------------------------------------------------
// Reset EEPROM, wipes all settings
// -------------------------------------------------------------------
void ResetEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  //DBUGS.println("Erasing EEPROM");
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0xff);
    //DBUGS.print("#");
  }
  EEPROM.end();
}

void EEPROM_read_string(int start, int count, String & val, String defaultVal = "") {
  byte checksum = CHECKSUM_SEED;
  for (int i = 0; i < count - 1; ++i) {
    byte c = EEPROM.read(start + i);
    if (c != 0 && c != 255) {
      checksum ^= c;
      val += (char) c;
    } else {
      break;
    }
  }

  // Check the checksum
  byte c = EEPROM.read(start + (count - 1));
  DBUGF("Got '%s' %d == %d @ %d:%d", val.c_str(), c, checksum, start, count);
  if (c != checksum) {
    DBUGF("Using default '%s'", defaultVal.c_str());
    val = defaultVal;
  }
}

void EEPROM_write_string(int start, int count, String val) {
  byte checksum = CHECKSUM_SEED;
  for (int i = 0; i < count - 1; ++i) {
    if (i < val.length()) {
      checksum ^= val[i];
      EEPROM.write(start + i, val[i]);
    } else {
      EEPROM.write(start + i, 0);
    }
  }
  EEPROM.write(start + (count - 1), checksum);
  DBUGF("Saved '%s' %d @ %d:%d", val.c_str(), checksum, start, count);
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void config_load_settings()
{
  EEPROM.begin(EEPROM_SIZE);

  // Load WiFi values
  EEPROM_read_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, esid);
  EEPROM_read_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, epass);

  // EmonCMS settings
  EEPROM_read_string(EEPROM_EMON_API_KEY_START, EEPROM_EMON_API_KEY_SIZE,
                     emoncms_apikey);
  EEPROM_read_string(EEPROM_EMON_SERVER_START, EEPROM_EMON_SERVER_SIZE,
                     emoncms_server);
  EEPROM_read_string(EEPROM_EMON_PATH_START, EEPROM_EMON_PATH_SIZE,
                     emoncms_path);
  EEPROM_read_string(EEPROM_EMON_NODE_START, EEPROM_EMON_NODE_SIZE,
                     emoncms_node);
  EEPROM_read_string(EEPROM_EMON_FINGERPRINT_START,
                     EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  // MQTT settings
  EEPROM_read_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);
  EEPROM_read_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);
  EEPROM_read_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);
  EEPROM_read_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);
  EEPROM_read_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  // Calibration settings
  EEPROM_read_string(EEPROM_CAL_VOLTAGE_START, EEPROM_CAL_VOLTAGE_SIZE, voltage_cal);
  EEPROM_read_string(EEPROM_CAL_VOLTAGE2_START, EEPROM_CAL_VOLTAGE2_SIZE, voltage2_cal);
  EEPROM_read_string(EEPROM_CAL_CT1_START, EEPROM_CAL_CT1_SIZE, ct1_cal);
  EEPROM_read_string(EEPROM_CAL_CT2_START, EEPROM_CAL_CT2_SIZE, ct2_cal);
  EEPROM_read_string(EEPROM_CAL_CT3_START, EEPROM_CAL_CT3_SIZE, ct3_cal);
  EEPROM_read_string(EEPROM_CAL_CT4_START, EEPROM_CAL_CT4_SIZE, ct4_cal);
  EEPROM_read_string(EEPROM_CAL_CT5_START, EEPROM_CAL_CT5_SIZE, ct5_cal);
  EEPROM_read_string(EEPROM_CAL_CT6_START, EEPROM_CAL_CT6_SIZE, ct6_cal);
  EEPROM_read_string(EEPROM_CAL_FREQ_START, EEPROM_CAL_FREQ_SIZE, freq_cal);
  EEPROM_read_string(EEPROM_CAL_GAIN_START, EEPROM_CAL_GAIN_SIZE, gain_cal);


  // Web server credentials
  EEPROM_read_string(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, www_username);
  EEPROM_read_string(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, www_password);

  EEPROM.end();
}

void config_save_emoncms(String server, String path, String node, String apikey, String fingerprint)
{
  EEPROM.begin(EEPROM_SIZE);

  emoncms_server = server;
  emoncms_path = path;
  emoncms_node = node;
  emoncms_apikey = apikey;
  emoncms_fingerprint = fingerprint;

  // save apikey to EEPROM
  EEPROM_write_string(EEPROM_EMON_API_KEY_START, EEPROM_EMON_API_KEY_SIZE, emoncms_apikey);

  // save emoncms server to EEPROM max 45 characters
  EEPROM_write_string(EEPROM_EMON_SERVER_START, EEPROM_EMON_SERVER_SIZE, emoncms_server);

  // save emoncms server to EEPROM max 16 characters
  EEPROM_write_string(EEPROM_EMON_PATH_START, EEPROM_EMON_PATH_SIZE, emoncms_path);

  // save emoncms node to EEPROM max 32 characters
  EEPROM_write_string(EEPROM_EMON_NODE_START, EEPROM_EMON_NODE_SIZE, emoncms_node);

  // save emoncms HTTPS fingerprint to EEPROM max 60 characters
  EEPROM_write_string(EEPROM_EMON_FINGERPRINT_START, EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  EEPROM.end();
}

void config_save_mqtt(String server, String topic, String prefix, String user, String pass)
{
  EEPROM.begin(EEPROM_SIZE);

  mqtt_server = server;
  mqtt_topic = topic;
  mqtt_feed_prefix = prefix;
  mqtt_user = user;
  mqtt_pass = pass;

  // Save MQTT server max 45 characters
  EEPROM_write_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);

  // Save MQTT topic max 32 characters
  EEPROM_write_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);

  // Save MQTT topic separator max 10 characters
  EEPROM_write_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);

  // Save MQTT username max 32 characters
  EEPROM_write_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);

  // Save MQTT pass max 64 characters
  EEPROM_write_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  EEPROM.end();
}

//for CircuitSetup 6 channel energy meter
void config_save_cal(String voltage, String voltage2, String ct1, String ct2, String ct3, String ct4, String ct5, String ct6, String freq, String gain)
{
  EEPROM.begin(EEPROM_SIZE);

  voltage_cal = voltage;
  voltage2_cal = voltage2;
  ct1_cal = ct1;
  ct2_cal = ct2;
  ct3_cal = ct3;
  ct4_cal = ct4;
  ct5_cal = ct5;
  ct6_cal = ct6;
  freq_cal = freq;
  gain_cal = gain;

  EEPROM_write_string(EEPROM_CAL_VOLTAGE_START, EEPROM_CAL_VOLTAGE_SIZE, voltage_cal);
  EEPROM_write_string(EEPROM_CAL_VOLTAGE2_START, EEPROM_CAL_VOLTAGE2_SIZE, voltage2_cal);
  EEPROM_write_string(EEPROM_CAL_CT1_START, EEPROM_CAL_CT1_SIZE, ct1_cal);
  EEPROM_write_string(EEPROM_CAL_CT2_START, EEPROM_CAL_CT2_SIZE, ct2_cal);
  EEPROM_write_string(EEPROM_CAL_CT3_START, EEPROM_CAL_CT3_SIZE, ct3_cal);
  EEPROM_write_string(EEPROM_CAL_CT4_START, EEPROM_CAL_CT4_SIZE, ct4_cal);
  EEPROM_write_string(EEPROM_CAL_CT5_START, EEPROM_CAL_CT5_SIZE, ct5_cal);
  EEPROM_write_string(EEPROM_CAL_CT6_START, EEPROM_CAL_CT6_SIZE, ct6_cal);
  EEPROM_write_string(EEPROM_CAL_FREQ_START, EEPROM_CAL_FREQ_SIZE, freq_cal);
  EEPROM_write_string(EEPROM_CAL_GAIN_START, EEPROM_CAL_GAIN_SIZE, gain_cal);

  EEPROM.end();
}

void config_save_admin(String user, String pass)
{
  EEPROM.begin(EEPROM_SIZE);

  www_username = user;
  www_password = pass;

  EEPROM_write_string(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, user);
  EEPROM_write_string(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, pass);

  EEPROM.commit();
}

void config_save_wifi(String qsid, String qpass)
{
  EEPROM.begin(EEPROM_SIZE);

  esid = qsid;
  epass = qpass;

  EEPROM_write_string(EEPROM_ESID_START, EEPROM_ESID_SIZE, qsid);
  EEPROM_write_string(EEPROM_EPASS_START, EEPROM_EPASS_SIZE, qpass);

  EEPROM.commit();
}

void config_reset()
{
  ResetEEPROM();
}
