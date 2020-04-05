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
unsigned short voltage_cal = 0;
unsigned short voltage2_cal = 0;
unsigned short freq_cal = 0;
unsigned short gain_cal[NUM_BOARDS] = { 0 };
unsigned short ct_cal[NUM_CHANNELS] = { 0 };
float cur_mul[NUM_CHANNELS] = { 0.0 };
float pow_mul[NUM_CHANNELS] = { 0.0 };

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
#define EEPROM_EMON_FINGERPRINT_SIZE  21
#define EEPROM_MQTT_FEED_PREFIX_SIZE  10
#define EEPROM_WWW_USER_SIZE      16
#define EEPROM_WWW_PASS_SIZE      16
#define EEPROM_CAL_VOLTAGE_SIZE   3
#define EEPROM_CAL_FREQ_SIZE      3
#define EEPROM_CAL_GAIN_SIZE      3
#define EEPROM_CAL_CT_SIZE        3
#define EEPROM_CUR_MUL_SIZE       5
#define EEPROM_POW_MUL_SIZE       5
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
#define EEPROM_CAL_VOLTAGE2_START EEPROM_CAL_VOLTAGE_END
#define EEPROM_CAL_VOLTAGE2_END   (EEPROM_CAL_VOLTAGE2_START + EEPROM_CAL_VOLTAGE_SIZE)
#define EEPROM_CAL_FREQ_START     EEPROM_CAL_VOLTAGE2_END
#define EEPROM_CAL_FREQ_END       (EEPROM_CAL_FREQ_START + EEPROM_CAL_FREQ_SIZE)
#define EEPROM_CAL_GAIN_START     EEPROM_CAL_FREQ_END
#define EEPROM_CAL_GAIN_END       (EEPROM_CAL_GAIN_START + EEPROM_CAL_GAIN_SIZE*NUM_BOARDS)
#define EEPROM_CAL_CT_START       EEPROM_CAL_GAIN_END
#define EEPROM_CAL_CT_END         (EEPROM_CAL_CT_START + EEPROM_CAL_CT_SIZE*NUM_CHANNELS)
#define EEPROM_CUR_MUL_START      EEPROM_CAL_CT_END
#define EEPROM_CUR_MUL_END        (EEPROM_CUR_MUL_START + EEPROM_CUR_MUL_SIZE*NUM_CHANNELS)
#define EEPROM_POW_MUL_START      EEPROM_CUR_MUL_END
#define EEPROM_POW_MUL_END        (EEPROM_POW_MUL_START + EEPROM_POW_MUL_SIZE*NUM_CHANNELS)
#define EEPROM_CONFIG_END         EEPROM_POW_MUL_END

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

void EEPROM_read_fingerprint(int start, int count, String & val, String defaultVal = "") {
  byte checksum = CHECKSUM_SEED;
  bool valid = false;
  char string[62], * ptr = string;
  for (int i = 0; i < count - 1; ++i) {
    byte c = EEPROM.read(start + i);
    checksum ^= c;
    ptr += sprintf(ptr, "%.2X:", c);
    if (c != 0) {
      valid = true;
    }
  }
  string[(count-1)*3-1] = '\0';
  val = String(string);

  // Check the checksum
  byte c = EEPROM.read(start + (count - 1));
  DBUGF("Got '%s' %d == %d @ %d:%d", val.c_str(), c, checksum, start, count);
  if (!valid || c != checksum) {
    DBUGF("Using default '%s'", defaultVal.c_str());
    val = defaultVal;
  }
}

unsigned short EEPROM_read_ushort(int start, unsigned short defaultVal = 0) {
  byte checksum = CHECKSUM_SEED;
  byte c[3] = { 0 };
  int i;
  for (i = 0; i < 2; i ++)
  {
    c[i] = EEPROM.read(start + i);
    checksum ^= c[i];
  }

  // Check the checksum
  c[i] = EEPROM.read(start + i);
  DBUGF("Got '%u' %d == %d @ %d:%d", *((unsigned short *)&c), c[i], checksum, start, i);
  if (c[i] != checksum) {
    DBUGF("Using default '%u'", defaultVal);
    return defaultVal;
  }
  return *((unsigned short *)&c);
}

float EEPROM_read_float(int start, float defaultVal = 0) {
  byte checksum = CHECKSUM_SEED;
  byte c[5] = { 0 };
  int i;
  for (i = 0; i < 4; i ++)
  {
    c[i] = EEPROM.read(start + i);
    checksum ^= c[i];
  }

  // Check the checksum
  c[i] = EEPROM.read(start + i);
  DBUGF("Got '%f' %d == %d @ %d:%d", *((float *)&c), c[i], checksum, start, i);
  if (c[i] != checksum) {
    DBUGF("Using default '%f'", defaultVal);
    return defaultVal;
  }
  return *((float *)&c);
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

void EEPROM_write_fingerprint(int start, int count, String val) {
  byte checksum = CHECKSUM_SEED;
  for (int i = 0; i < count - 1; ++i) {
    if (i*3 < val.length()) {
      byte b;
      sscanf(&val.c_str()[i*3], "%x", &b);
      checksum ^= b;
      EEPROM.write(start + i, b);
    } else {
      EEPROM.write(start + i, 0);
    }
  }
  EEPROM.write(start + (count - 1), checksum);
  DBUGF("Saved '%s' %d @ %d:%d", val.c_str(), checksum, start, count);
}

void EEPROM_write_ushort(int start, unsigned short val) {
  byte checksum = CHECKSUM_SEED;
  int i;
  for (i = 0; i < 2; i++)
  {
    checksum ^= *(((byte *)&val)+i);
    EEPROM.write(start+i, *(((byte *)&val)+i));
  }
  EEPROM.write(start+i, checksum);
  DBUGF("Saved '%d' %d @ %d:%d", val, checksum, start, i);
}

void EEPROM_write_float(int start, float val) {
  byte checksum = CHECKSUM_SEED;
  int i;
  for (i = 0; i < 4; i++)
  {
    checksum ^= *(((byte *)&val)+i);
    EEPROM.write(start+i, *(((byte *)&val)+i));
  }
  EEPROM.write(start+i, checksum);

  DBUGF("Saved '%f' %d @ %d:%d", val, checksum, start, i);
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
  EEPROM_read_fingerprint(EEPROM_EMON_FINGERPRINT_START,
                     EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

  // MQTT settings
  EEPROM_read_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);
  EEPROM_read_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);
  EEPROM_read_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);
  EEPROM_read_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);
  EEPROM_read_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  // Calibration settings
  voltage_cal = EEPROM_read_ushort(EEPROM_CAL_VOLTAGE_START, EEPROM_CAL_VOLTAGE_SIZE);
  voltage2_cal = EEPROM_read_ushort(EEPROM_CAL_VOLTAGE2_START, EEPROM_CAL_VOLTAGE_SIZE);
  freq_cal = EEPROM_read_ushort(EEPROM_CAL_FREQ_START, EEPROM_CAL_FREQ_SIZE);
  for (int i = 0; i < NUM_CHANNELS; i++)
  {
    ct_cal[i] = EEPROM_read_ushort(EEPROM_CAL_CT_START + (i*EEPROM_CAL_CT_SIZE));
    cur_mul[i] = EEPROM_read_float(EEPROM_CUR_MUL_START + (i*EEPROM_CUR_MUL_SIZE), 1.0);
    pow_mul[i] = EEPROM_read_float(EEPROM_POW_MUL_START + (i*EEPROM_POW_MUL_SIZE), 1.0);
  }
  for (int i = 0; i < NUM_BOARDS; i++)
  {
    gain_cal[i] = EEPROM_read_ushort(EEPROM_CAL_GAIN_START + (i*EEPROM_CAL_GAIN_SIZE));
  }

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

  // save emoncms HTTPS fingerprint to EEPROM max 20 bytes
  EEPROM_write_fingerprint(EEPROM_EMON_FINGERPRINT_START, EEPROM_EMON_FINGERPRINT_SIZE, emoncms_fingerprint);

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
void config_save_cal(AsyncWebServerRequest * request)
{
  char req[12];

  EEPROM.begin(EEPROM_SIZE);

  voltage_cal = request->arg("voltage_cal").toInt();
  voltage2_cal = request->arg("voltage2_cal").toInt();
  freq_cal = request->arg("freq_cal").toInt();

  EEPROM_write_ushort(EEPROM_CAL_VOLTAGE_START, voltage_cal);
  EEPROM_write_ushort(EEPROM_CAL_VOLTAGE2_START, voltage2_cal);
  EEPROM_write_ushort(EEPROM_CAL_FREQ_START, freq_cal);

  for (int i = 0; i < NUM_CHANNELS; i++)
  {
    sprintf(req, "ct%d_cal", i+1);
    ct_cal[i] = request->arg(req).toInt();
    EEPROM_write_ushort(EEPROM_CAL_CT_START + (i*EEPROM_CAL_CT_SIZE), ct_cal[i]);
    sprintf(req, "cur%d_mul", i+1);
    cur_mul[i] = request->arg(req).toFloat();
    EEPROM_write_float(EEPROM_CUR_MUL_START + (i*EEPROM_CUR_MUL_SIZE), cur_mul[i]);
    sprintf(req, "pow%d_mul", i+1);
    pow_mul[i] = request->arg(req).toFloat();
    EEPROM_write_float(EEPROM_POW_MUL_START + (i*EEPROM_POW_MUL_SIZE), pow_mul[i]);
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
    EEPROM_write_ushort(EEPROM_CAL_GAIN_START + (i*EEPROM_CAL_GAIN_SIZE), gain_cal[i]);
  }

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
