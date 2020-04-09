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
#include "wifi_local.h"

WiFiClient espClient;                 // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT

long lastMqttReconnectAttempt = 0;
int clientTimeout = 0;
int i = 0;


// -------------------------------------------------------------------
// MQTT Connect
// Called only when MQTT server field is populated
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  mqttclient.setServer(mqtt_server.c_str(), 1883);
  DBUGS.println("MQTT Connecting...");
  DBUGS.println(mqttclient.state());

#ifdef ESP32
  String strID = String((uint32_t)ESP.getEfuseMac());
#else
  String strID = String(ESP.getChipId());
#endif

  if (mqtt_user.length() == 0) {
    //allows for anonymous connection 
    if (mqttclient.connect(strID.c_str())) {  // Attempt to connect
      DBUGS.println("MQTT connected");
      mqttclient.publish(mqtt_topic.c_str(), "connected"); // Once connected, publish an announcement..
    } else {
      DBUGS.print("MQTT failed: ");
      DBUGS.println(mqttclient.state());
      return (0);
    }
    
  } else {
    if (mqttclient.connect(strID.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {  // Attempt to connect
      DBUGS.println("MQTT connected");
      mqttclient.publish(mqtt_topic.c_str(), "connected"); // Once connected, publish an announcement..
    } else {
      DBUGS.print("MQTT failed: ");
      DBUGS.println(mqttclient.state());
      return (0);
    }
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
void mqtt_publish(String data)
{
  String mqtt_data = "";
  String topic = mqtt_topic + "/" + mqtt_feed_prefix;
  int i = 0;
  while (int (data[i]) != 0)
  {
    // Construct MQTT topic e.g. <base_topic>/CT1 e.g. emonesp/CT1
    while (data[i] != ':') {
      topic += data[i];
      i++;
      if (int(data[i]) == 0) {
        break;
      }
    }
    i++;
    // Construct data string to publish to above topic
    while (data[i] != ',') {
      mqtt_data += data[i];
      i++;
      if (int(data[i]) == 0) {
        break;
      }
    }
    // send data via mqtt
    //delay(100);
    //DBUGS.printf("%s = %s\r\n", topic.c_str(), mqtt_data.c_str());
    mqttclient.publish(topic.c_str(), mqtt_data.c_str());
    topic = mqtt_topic + "/" + mqtt_feed_prefix;
    mqtt_data = "";
    i++;
    if (int(data[i]) == 0) break;
  }

  // send esp free ram
  String ram_topic = mqtt_topic + "/" + mqtt_feed_prefix + "freeram";
  String free_ram = String(ESP.getFreeHeap());
  mqttclient.publish(ram_topic.c_str(), free_ram.c_str());

  // send wifi signal strength
  long rssi = WiFi.RSSI();
  String rssi_S = String(rssi);
  String rssi_topic = mqtt_topic + "/" + mqtt_feed_prefix + "rssi";
  mqttclient.publish(rssi_topic.c_str(), rssi_S.c_str());

    // send ip
  String ip_topic = mqtt_topic + "/" + mqtt_feed_prefix + "ip";
  String ip_add = WiFi.localIP().toString();
  mqttclient.publish(ip_topic.c_str(), ip_add.c_str());

  // send Uptime
  String uptime_topic = mqtt_topic + "/" + mqtt_feed_prefix + "uptime";
  String uptime = String(millis());
  mqttclient.publish(uptime_topic.c_str(), uptime.c_str());

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
