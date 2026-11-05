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
#include <WiFi.h>
#include "energy_meter.h"

WiFiClient espClient;                 // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT

unsigned long lastMqttReconnectAttempt = 0;
int clientTimeout = 0;
int i = 0;

String input_json;

static char mqtt_topic_prefix[128] = "";
static char mqtt_data[64] = "";

// -------------------------------------------------------------------
// MQTT Connect
// Called only when MQTT server field is populated
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  DBUGS.println("MQTT Connecting...");

  if (espClient.connect(mqtt_server.c_str(), 1883, MQTT_TIMEOUT * 1000) != 1)
  {
     DBUGS.println("MQTT connect timeout.");
     return (0);
  }

  espClient.setTimeout(MQTT_TIMEOUT);
  mqttclient.setSocketTimeout(MQTT_TIMEOUT);
  mqttclient.setBufferSize(MAX_DATA_LEN + 200);

  String strID = String((uint32_t)(ESP.getEfuseMac() >> 16), HEX);

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
  if (config_flags.mqtt_json) {
    if (input_json.length() == 0) return;
    const size_t packetSize = input_json.length() + mqtt_topic.length() + 7;
    if (packetSize > UINT16_MAX) return;
    if (packetSize > mqttclient.getBufferSize() && !mqttclient.setBufferSize(packetSize)) return;
    mqttclient.publish(mqtt_topic.c_str(), input_json.c_str(), false);
    return;
  }

  const int prefixLength = snprintf(mqtt_topic_prefix, sizeof(mqtt_topic_prefix), "%s/%s",
                                   mqtt_topic.c_str(), mqtt_feed_prefix.c_str());
  if (prefixLength < 0 || prefixLength >= sizeof(mqtt_topic_prefix)) return;
  char *suffix = mqtt_topic_prefix + prefixLength;
  const size_t suffixSize = sizeof(mqtt_topic_prefix) - prefixLength;
  if (suffixSize <= strlen("freeram")) return;

  while (*data) {
    const size_t keyLength = strcspn(data, ":,");
    if (data[keyLength] != ':' || keyLength == 0 || keyLength >= suffixSize) return;
    const char *value = data + keyLength + 1;
    const size_t valueLength = strcspn(value, ",");
    if (valueLength >= sizeof(mqtt_data)) return;

    memcpy(suffix, data, keyLength);
    suffix[keyLength] = '\0';
    memcpy(mqtt_data, value, valueLength);
    mqtt_data[valueLength] = '\0';
    if (!mqttclient.publish(mqtt_topic_prefix, mqtt_data)) return;

    data = value + valueLength;
    if (*data) ++data;
  }

  strcpy(suffix, "freeram");
  snprintf(mqtt_data, sizeof(mqtt_data), "%lu", (unsigned long)ESP.getFreeHeap());
  if (!mqttclient.publish(mqtt_topic_prefix, mqtt_data)) return;

  strcpy(suffix, "rssi");
  snprintf(mqtt_data, sizeof(mqtt_data), "%d", WiFi.RSSI());
  mqttclient.publish(mqtt_topic_prefix, mqtt_data);
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
  if (!mqttclient.connected()) {
    const unsigned long now = millis();
    // try and reconnect continuously for first 5s then try again once every 10s
    if ( (now < 5000) || ((now - lastMqttReconnectAttempt)  > 10000) ) {
      lastMqttReconnectAttempt = now;
      if (mqtt_connect()) lastMqttReconnectAttempt = 0;
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
