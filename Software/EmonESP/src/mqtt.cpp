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
#include "mqtt.h"
#include "config.h"
#include "esp_wifi.h"
#include "energy_meter.h"

#define MQTT_TIMEOUT 3

WiFiClient espClient;                 // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT

long lastMqttReconnectAttempt = 0;
int clientTimeout = 0;
int i = 0;

char input_json[MAX_DATA_LEN];

static char mqtt_topic_prefix[128] = "";
static char mqtt_data[64] = "";
static int mqtt_connection_error_count = 0;

// -------------------------------------------------------------------
// MQTT Connect
// Called only when MQTT server field is populated
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  DBUGS.println("MQTT Connecting...");
  DBUGS.println(mqttclient.state());

  if (espClient.connect(mqtt_server.c_str(), 1883, MQTT_TIMEOUT * 1000) != 1)
  {
     DBUGS.println("MQTT connect timeout.");
     return (0);
  }

  espClient.setTimeout(MQTT_TIMEOUT);

#ifdef ESP32
  String strID = String((uint32_t)(ESP.getEfuseMac() >> 16), HEX);
#else
  String strID = String(ESP.getChipId());
#endif

  if (mqtt_user.length() == 0) {
    //allows for anonymous connection
    mqttclient.connect(strID.c_str()); // Attempt to connect
  } else {
    mqttclient.connect(strID.c_str(), mqtt_user.c_str(), mqtt_pass.c_str()); // Attempt to connect
  }

  if (mqttclient.state() == 0)
  {
    DBUGS.println("MQTT connected");
    if (!config_flags.mqtt_json)
    {
      mqttclient.publish(mqtt_topic.c_str(), "connected"); // Once connected, publish an announcement..
    }
  } else {
    DBUGS.print("MQTT failed: ");
    DBUGS.println(mqttclient.state());
    return (0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// Split up data string into sub topics: e.g
// data = CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// base topic = emon/emonesp
// MQTT Publish: emon/emonesp/CT1 > 3935 etc..
// -------------------------------------------------------------------
void mqtt_publish(const char * data)
{
  if (config_flags.mqtt_json)
  {
    // add some data onto the end
    sprintf(input_json+strlen(input_json)-1,
      ",\"freeram\":%lu,\"rssi\":%d}", (unsigned long)ESP.getFreeHeap(), WiFi.RSSI());

    if (!mqttclient.publish(mqtt_topic.c_str(), input_json, false))
    {
      return;
    }
  }
  else
  {
    const char * data_ptr = data;
    char * topic_ptr = mqtt_topic_prefix;
    topic_ptr += sprintf(mqtt_topic_prefix, "%s/%s", mqtt_topic.c_str(), mqtt_feed_prefix.c_str());

    do
    {
      int pos = strcspn(data_ptr, ":");
      strncpy(topic_ptr, data_ptr, pos);
      topic_ptr[pos] = 0;
      data_ptr += pos;
      if (*data_ptr++ == 0) {
        break;
      }

      pos = strcspn(data_ptr, ",");
      strncpy(mqtt_data, data_ptr, pos);
      mqtt_data[pos] = 0;
      data_ptr += pos;

      // send data via mqtt
      //delay(100);
      //DBUGS.printf("%s = %s\r\n", mqtt_topic_prefix, mqtt_data);
      if (!mqttclient.publish(mqtt_topic_prefix, mqtt_data))
      {
        return;
      }
    } while (*data_ptr++ != 0);

    // send esp free ram
    sprintf(mqtt_topic_prefix, "%s/%sfreeram", mqtt_topic.c_str(), mqtt_feed_prefix.c_str());
    sprintf(mqtt_data, "%lu", (unsigned long)ESP.getFreeHeap());
    if (!mqttclient.publish(mqtt_topic_prefix, mqtt_data))
    {
      return;
    }

    // send wifi signal strength
    sprintf(mqtt_topic_prefix, "%s/%srssi", mqtt_topic.c_str(), mqtt_feed_prefix.c_str());
    sprintf(mqtt_data, "%d", WiFi.RSSI());
    if (!mqttclient.publish(mqtt_topic_prefix, mqtt_data))
    {
      return;
    }
  }
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
  if (!mqttclient.connected()) {
    long now = millis();
    // try and reconnect continuously for first 5s then try again once every 10s
    if ( (now < 5000) || ((now - lastMqttReconnectAttempt)  > 10000) ) {
      lastMqttReconnectAttempt = now;
      if (mqtt_connect()) { // Attempt to reconnect
        lastMqttReconnectAttempt = 0;
        mqtt_connection_error_count = 0;
      } else {
        mqtt_connection_error_count ++;
        if (mqtt_connection_error_count > 10) {
#ifdef ESP32
          esp_restart();
#else
          ESP.restart();
#endif
        }
      }
    }
  } else {
    // if MQTT connected
    mqttclient.loop();
  }
}

void mqtt_restart()
{
  if (mqttclient.connected()) {
    mqttclient.disconnect();
  }
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}
