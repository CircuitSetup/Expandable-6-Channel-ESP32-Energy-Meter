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

#include "emonesp.h"
#include "app_config.h"
#include "espal.h"

#include <Arduino.h>
#include <EEPROM.h>                   // Save config settings

#define EEPROM_ESID_SIZE              32
#define EEPROM_EPASS_SIZE             64
#define EEPROM_EMON_API_KEY_SIZE      32
#define EEPROM_EMON_SERVER_SIZE       32
#define EEPROM_EMON_PATH_SIZE         16
#define EEPROM_EMON_NODE_SIZE         32
#define EEPROM_MQTT_SERVER_SIZE       32
#define EEPROM_MQTT_PORT_SIZE         2
#define EEPROM_MQTT_TOPIC_SIZE        32
#define EEPROM_MQTT_USER_SIZE         32
#define EEPROM_MQTT_PASS_SIZE         64
#define EEPROM_EMON_FINGERPRINT_SIZE  60
#define EEPROM_MQTT_FEED_PREFIX_SIZE  10
#define EEPROM_WWW_USER_SIZE          16
#define EEPROM_WWW_PASS_SIZE          16
#define EEPROM_CAL_VOLTAGE_SIZE       6
#define EEPROM_CAL_CT1_SIZE           6
#define EEPROM_CAL_CT2_SIZE           6
#define EEPROM_CAL_FREQ_SIZE          6
#define EEPROM_CAL_GAIN_SIZE          6
#ifdef SOLAR_METER
#define EEPROM_CAL_SVOLTAGE_SIZE      6
#define EEPROM_CAL_SCT1_SIZE          6
#define EEPROM_CAL_SCT2_SIZE          6
#endif
#define EEPROM_TIMER_START1_SIZE      2
#define EEPROM_TIMER_STOP1_SIZE       2
#define EEPROM_TIMER_START2_SIZE      2
#define EEPROM_TIMER_STOP2_SIZE       2
#define EEPROM_VOLTAGE_OUTPUT_SIZE    2
#define EEPROM_TIME_OFFSET_SIZE       2
#define EEPROM_SIZE                   1024

#define EEPROM_ESID_START             0
#define EEPROM_ESID_END               (EEPROM_ESID_START + EEPROM_ESID_SIZE)
#define EEPROM_EPASS_START            EEPROM_ESID_END
#define EEPROM_EPASS_END              (EEPROM_EPASS_START + EEPROM_EPASS_SIZE)
#define EEPROM_EMON_API_KEY_START     EEPROM_EPASS_END
#define EEPROM_EMON_API_KEY_END       (EEPROM_EMON_API_KEY_START + EEPROM_EMON_API_KEY_SIZE)
#define EEPROM_EMON_SERVER_START      EEPROM_EMON_API_KEY_END
#define EEPROM_EMON_SERVER_END        (EEPROM_EMON_SERVER_START + EEPROM_EMON_SERVER_SIZE)
#define EEPROM_EMON_NODE_START        EEPROM_EMON_SERVER_END
#define EEPROM_EMON_NODE_END          (EEPROM_EMON_NODE_START + EEPROM_EMON_NODE_SIZE)
#define EEPROM_MQTT_SERVER_START      EEPROM_EMON_NODE_END
#define EEPROM_MQTT_SERVER_END        (EEPROM_MQTT_SERVER_START + EEPROM_MQTT_SERVER_SIZE)
#define EEPROM_MQTT_PORT_START        EEPROM_MQTT_SERVER_END
#define EEPROM_MQTT_PORT_END          (EEPROM_MQTT_PORT_START + EEPROM_MQTT_PORT_SIZE)
#define EEPROM_MQTT_TOPIC_START       EEPROM_MQTT_PORT_END
#define EEPROM_MQTT_TOPIC_END         (EEPROM_MQTT_TOPIC_START + EEPROM_MQTT_TOPIC_SIZE)
#define EEPROM_MQTT_USER_START        EEPROM_MQTT_TOPIC_END
#define EEPROM_MQTT_USER_END          (EEPROM_MQTT_USER_START + EEPROM_MQTT_USER_SIZE)
#define EEPROM_MQTT_PASS_START        EEPROM_MQTT_USER_END
#define EEPROM_MQTT_PASS_END          (EEPROM_MQTT_PASS_START + EEPROM_MQTT_PASS_SIZE)
#define EEPROM_EMON_FINGERPRINT_START EEPROM_MQTT_PASS_END
#define EEPROM_EMON_FINGERPRINT_END   (EEPROM_EMON_FINGERPRINT_START + EEPROM_EMON_FINGERPRINT_SIZE)
#define EEPROM_MQTT_FEED_PREFIX_START EEPROM_EMON_FINGERPRINT_END
#define EEPROM_MQTT_FEED_PREFIX_END   (EEPROM_MQTT_FEED_PREFIX_START + EEPROM_MQTT_FEED_PREFIX_SIZE)
#define EEPROM_WWW_USER_START         EEPROM_MQTT_FEED_PREFIX_END
#define EEPROM_WWW_USER_END           (EEPROM_WWW_USER_START + EEPROM_WWW_USER_SIZE)
#define EEPROM_WWW_PASS_START         EEPROM_WWW_USER_END
#define EEPROM_WWW_PASS_END           (EEPROM_WWW_PASS_START + EEPROM_WWW_PASS_SIZE)
#define EEPROM_EMON_PATH_START        EEPROM_WWW_PASS_END
#define EEPROM_EMON_PATH_END          (EEPROM_EMON_PATH_START + EEPROM_EMON_PATH_SIZE)

#define EEPROM_TIMER_START1_START     EEPROM_EMON_PATH_END
#define EEPROM_TIMER_START1_END       (EEPROM_TIMER_START1_START + EEPROM_TIMER_START1_SIZE)
#define EEPROM_TIMER_STOP1_START      EEPROM_TIMER_START1_END
#define EEPROM_TIMER_STOP1_END        (EEPROM_TIMER_STOP1_START + EEPROM_TIMER_STOP1_SIZE)
#define EEPROM_TIMER_START2_START     EEPROM_TIMER_STOP1_END
#define EEPROM_TIMER_START2_END       (EEPROM_TIMER_START2_START + EEPROM_TIMER_START2_SIZE)
#define EEPROM_TIMER_STOP2_START      EEPROM_TIMER_START2_END
#define EEPROM_TIMER_STOP2_END        (EEPROM_TIMER_STOP2_START + EEPROM_TIMER_STOP2_SIZE)

#define EEPROM_VOLTAGE_OUTPUT_START   EEPROM_TIMER_STOP2_END
#define EEPROM_VOLTAGE_OUTPUT_END     (EEPROM_VOLTAGE_OUTPUT_START + EEPROM_VOLTAGE_OUTPUT_SIZE)

#define EEPROM_TIME_OFFSET_START      EEPROM_VOLTAGE_OUTPUT_END
#define EEPROM_TIME_OFFSET_END        (EEPROM_TIME_OFFSET_START + EEPROM_TIME_OFFSET_SIZE)

#define EEPROM_CAL_VOLTAGE_START  EEPROM_TIME_OFFSET_END
#define EEPROM_CAL_VOLTAGE_END    (EEPROM_CAL_VOLTAGE_START + EEPROM_CAL_VOLTAGE_SIZE)
#define EEPROM_CAL_VOLTAGE2_START  EEPROM_CAL_VOLTAGE_END
#define EEPROM_CAL_VOLTAGE2_END   EEPROM_CAL_VOLTAGE2_START + EEPROM_CAL_VOLTAGE_SIZE
#define EEPROM_CAL_CT1_START   EEPROM_CAL_VOLTAGE_END
#define EEPROM_CAL_CT1_END     (EEPROM_CAL_CT1_START + EEPROM_CAL_CT1_SIZE)
#define EEPROM_CAL_CT2_START    EEPROM_CAL_CT1_END
#define EEPROM_CAL_CT2_END      (EEPROM_CAL_CT2_START + EEPROM_CAL_CT2_SIZE)
#define EEPROM_CAL_FREQ_START    EEPROM_CAL_CT2_END
#define EEPROM_CAL_FREQ_END      (EEPROM_CAL_FREQ_START + EEPROM_CAL_FREQ_SIZE)
#define EEPROM_CAL_GAIN_START    EEPROM_CAL_FREQ_END
#define EEPROM_CAL_GAIN_END      (EEPROM_CAL_GAIN_START + EEPROM_CAL_GAIN_SIZE)
#ifdef SOLAR_METER
#define EEPROM_CAL_SVOLTAGE_START EEPROM_CAL_GAIN_END
#define EEPROM_CAL_SVOLTAGE_END   (EEPROM_CAL_SVOLTAGE_START + EEPROM_CAL_SVOLTAGE_SIZE)
#define EEPROM_CAL_SCT1_START     EEPROM_CAL_SVOLTAGE_END
#define EEPROM_CAL_SCT1_END       (EEPROM_CAL_SCT1_START + EEPROM_CAL_SCT1_SIZE)
#define EEPROM_CAL_SCT2_START     EEPROM_CAL_SCT1_END
#define EEPROM_CAL_SCT2_END       (EEPROM_CAL_SCT2_START + EEPROM_CAL_SCT2_SIZE)
#define EEPROM_CONFIG_END         EEPROM_CAL_SCT2_END
#else
#define EEPROM_CONFIG_END         EEPROM_CAL_GAIN_END
#endif

#if EEPROM_CONFIG_END > EEPROM_SIZE
#error EEPROM_SIZE too small
#endif

int read_offset = 0;

void EEPROM_read_string(int start, int count, String & val) {
  String newVal;
  start += read_offset;
  for (int i = 0; i < count; ++i){
    byte c = EEPROM.read(start+i);
    if (c != 0 && c != 255)
      newVal += (char) c;
  }

  if(newVal) {
    val = newVal;
  }
}

//this function should be wrong... ints for ESP32 are four bytes, not two.
void EEPROM_read_int(int start, int & val) {
  start += read_offset;
  byte high = EEPROM.read(start);
  byte low = EEPROM.read(start+1);
  val=word(high,low);
}

//float should be 4 bytes
void EEPROM_read_float(int start, float & val){

}

//ushort should be two bytes
void EEPROM_read_ushort(int start, unsigned short & val){
  start += read_offset;
  byte high = EEPROM.read(start);
  byte low = EEPROM.read(start + 1);
  val = word(high, low);
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void config_load_v1_settings()
{
  DBUGLN("Loading config");

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

  flags &= ~CONFIG_SERVICE_EMONCMS;
  if(emoncms_apikey != 0) {
    flags |= CONFIG_SERVICE_EMONCMS;
  }

  // MQTT settings
  EEPROM_read_string(EEPROM_MQTT_SERVER_START, EEPROM_MQTT_SERVER_SIZE, mqtt_server);
  EEPROM_read_int(EEPROM_MQTT_PORT_START, mqtt_port);
  DBUGF("mqtt_port %d %2s", mqtt_port, (char *)&mqtt_port);
  if (mqtt_port==0) {
    mqtt_port = 1883; // apply a default port
  }

  // Anoyingly the mqtt_port was added in the middle of the mqtt block not at the end of EEPROM
  // detect some values that may be older firmwares
  if(word('e', 'm')) { 
    mqtt_port = 1883; // apply a default port
    read_offset = -EEPROM_MQTT_PORT_SIZE;
  }
  
  EEPROM_read_string(EEPROM_MQTT_TOPIC_START, EEPROM_MQTT_TOPIC_SIZE, mqtt_topic);
  EEPROM_read_string(EEPROM_MQTT_FEED_PREFIX_START, EEPROM_MQTT_FEED_PREFIX_SIZE, mqtt_feed_prefix);
  EEPROM_read_string(EEPROM_MQTT_USER_START, EEPROM_MQTT_USER_SIZE, mqtt_user);
  EEPROM_read_string(EEPROM_MQTT_PASS_START, EEPROM_MQTT_PASS_SIZE, mqtt_pass);

  flags &= ~CONFIG_SERVICE_MQTT;
  if(mqtt_server != 0) {
    flags |= CONFIG_SERVICE_MQTT;
  }

  // Calibration settings
  /*EEPROM_read_int(EEPROM_CAL_VOLTAGE_START, voltage_cal);
  EEPROM_read_int(EEPROM_CAL_CT1_START, ct1_cal);
  EEPROM_read_int(EEPROM_CAL_CT2_START, ct2_cal);
  EEPROM_read_int(EEPROM_CAL_FREQ_START, freq_cal);
  EEPROM_read_int(EEPROM_CAL_GAIN_START, gain_cal);
#ifdef SOLAR_METER
  EEPROM_read_int(EEPROM_CAL_SVOLTAGE_START, svoltage_cal);
  EEPROM_read_int(EEPROM_CAL_SCT1_START, sct1_cal);
  EEPROM_read_int(EEPROM_CAL_SCT2_START, sct2_cal);
#endif
*/
  EEPROM_read_ushort(EEPROM_CAL_VOLTAGE_START, voltage_cal);
  EEPROM_read_ushort(EEPROM_CAL_VOLTAGE2_START, voltage2_cal);
  EEPROM_read_int(EEPROM_CAL_FREQ_START, freq_cal);
  for (int i = 0; i < NUM_CHANNELS; i++)
  {
    EEPROM_read_string(EEPROM_NAME_CT_START + (i*EEPROM_NAME_CT_SIZE), EEPROM_NAME_CT_SIZE, ct_name[i], "ct" + String(i+1));
    ct_cal[i] = EEPROM_read_ushort(EEPROM_CAL_CT_START + (i*EEPROM_CAL_CT_SIZE), CURRENT_GAIN_DEFAULT);
    cur_mul[i] = EEPROM_read_float(EEPROM_CUR_MUL_START + (i*EEPROM_CUR_MUL_SIZE), 1.0);
    pow_mul[i] = EEPROM_read_float(EEPROM_POW_MUL_START + (i*EEPROM_POW_MUL_SIZE), 1.0);
  }
  for (int i = 0; i < NUM_BOARDS; i++)
  {
    gain_cal[i] = EEPROM_read_ushort(EEPROM_CAL_GAIN_START + (i*EEPROM_CAL_GAIN_SIZE), PGA_GAIN_DEFAULT);
  }

  // Web server credentials
  EEPROM_read_string(EEPROM_WWW_USER_START, EEPROM_WWW_USER_SIZE, www_username);
  EEPROM_read_string(EEPROM_WWW_PASS_START, EEPROM_WWW_PASS_SIZE, www_password);
  
  // Read timer settings
  EEPROM_read_int(EEPROM_TIMER_START1_START, timer_start1);
  EEPROM_read_int(EEPROM_TIMER_STOP1_START, timer_stop1);
  EEPROM_read_int(EEPROM_TIMER_START2_START, timer_start2);
  EEPROM_read_int(EEPROM_TIMER_STOP2_START, timer_stop2);

  EEPROM_read_int(EEPROM_VOLTAGE_OUTPUT_START, voltage_output);
  
  EEPROM_read_int(EEPROM_TIME_OFFSET_START, time_offset);

  EEPROM.end();
}
