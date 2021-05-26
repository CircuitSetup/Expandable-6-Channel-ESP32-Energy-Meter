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
#include "esp_wifi.h"
#include "mqtt.h"
#include "app_config.h"
#include "espal.h"

WiFiClient espClient;                 // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT

static long nextMqttReconnectAttempt = 0;
static unsigned long mqttRestartTime = 0;

int clientTimeout = 0;
int i = 0;

// -------------------------------------------------------------------
// MQTT Control callback for WIFI Relay and Sonoff smartplug
// -------------------------------------------------------------------
static void mqtt_msg_callback(char *topic, byte *payload, unsigned int length) {

  String topicstr = String(topic);
  String payloadstr = String((char *)payload);
  payloadstr = payloadstr.substring(0,length);

  DBUGF("Message arrived topic:[%s] payload: [%s]", topic, payload);

  // --------------------------------------------------------------------------
  // State
  // --------------------------------------------------------------------------
  if (topicstr.compareTo(mqtt_topic+"/"+node_name+"/in/ctrlmode")==0) {
    DEBUG.print(F("Status: "));
    if (payloadstr.compareTo("2")==0) {
      ctrl_mode = "Timer";
    } else if (payloadstr.compareTo("1")==0) {
      ctrl_mode = "On";
    } else if (payloadstr.compareTo("0")==0) {
      ctrl_mode = "Off";
    } else if (payloadstr.compareTo("Timer")==0) {
      ctrl_mode = "Timer";
    } else if (payloadstr.compareTo("On")==0) {
      ctrl_mode = "On";
    } else if (payloadstr.compareTo("Off")==0) {
      ctrl_mode = "Off";
    } else {
      ctrl_mode = "Off";
    }
    DEBUG.println(ctrl_mode);
  // --------------------------------------------------------------------------
  // Timer
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo(mqtt_topic+"/"+node_name+"/in/timer")==0) {
    DEBUG.print(F("Timer: "));
    if (payloadstr.length()==9) {
      String tstart = payloadstr.substring(0,4);
      String tstop = payloadstr.substring(5,9);
      timer_start1 = tstart.toInt();
      timer_stop1 = tstop.toInt();
      DEBUG.println(tstart+" "+tstop);
    }
    if (payloadstr.length()==19) {
      String tstart1 = payloadstr.substring(0,4);
      String tstop1 = payloadstr.substring(5,9);
      timer_start1 = tstart1.toInt();
      timer_stop1 = tstop1.toInt();
      String tstart2 = payloadstr.substring(10,14);
      String tstop2 = payloadstr.substring(15,19);
      timer_start2 = tstart2.toInt();
      timer_stop2 = tstop2.toInt();
      DEBUG.println(tstart1+":"+tstop1+" "+tstart2+":"+tstop2);
    }
  // --------------------------------------------------------------------------
  // Vout
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo(mqtt_topic+"/"+node_name+"/in/vout")==0) {
    DEBUG.print(F("Vout: "));
    voltage_output = payloadstr.toInt();
    DEBUG.println(voltage_output);
  // --------------------------------------------------------------------------
  // FlowT
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo(mqtt_topic+"/"+node_name+"/in/flowT")==0) {
    DEBUG.print(F("FlowT: "));
    float flow = payloadstr.toFloat();
    voltage_output = (int) (flow - 7.14)/0.0371;
    DEBUG.println(String(flow)+" vout:"+String(voltage_output));
  // --------------------------------------------------------------------------
  // Return device state
  // --------------------------------------------------------------------------
  } else if (topicstr.compareTo(mqtt_topic+"/"+node_name+"/in/state")==0) {
    DEBUG.println(F("State: "));

    String s = "{";
    s += "\"ip\":\""+ipaddress+"\",";
    // s += "\"time\":\"" + String(getTime()) + "\",";
    s += "\"ctrlmode\":\"" + String(ctrl_mode) + "\",";
    s += "\"timer\":\"" + String(timer_start1)+" "+String(timer_stop1)+" "+String(timer_start2)+" "+String(timer_stop2) + "\",";
    s += "\"vout\":\"" + String(voltage_output) + "\"";
    s += "}";
    mqtt_publish("out/state",s);
  }
}

// -------------------------------------------------------------------
// MQTT Connect
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  DBUGS.println("MQTT Connecting...");

  DEBUG.print(F("MQTT Connecting to..."));
  DEBUG.println(mqtt_server.c_str());

  espClient.setTimeout(MQTT_TIMEOUT);
  mqttclient.setSocketTimeout(MQTT_TIMEOUT);
  mqttclient.setBufferSize(MAX_DATA_LEN + 200);

    String subscribe_topic = mqtt_topic + "/" + node_name + "/in/#";
    mqttclient.subscribe(subscribe_topic.c_str());

  } else {
    DEBUG.print(F("MQTT failed: "));
    DEBUG.println(mqttclient.state());
    return(0);
  }
  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// -------------------------------------------------------------------
void mqtt_publish(String topic_p2, String data)
{
  if(!config_mqtt_enabled() || !mqttclient.connected()) {
    return;
  }

  String topic = mqtt_topic + "/" + node_name + "/" + topic_p2;
  mqttclient.publish(topic.c_str(), data.c_str());
}

// -------------------------------------------------------------------
// Publish to MQTT
// Split up data string into sub topics: e.g
// data = CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// base topic = emon/emonesp
// MQTT Publish: emon/emonesp/CT1 > 3935 etc..
// -------------------------------------------------------------------
void mqtt_publish(JsonDocument &data)
{
  Profile_Start(mqtt_publish);

  if(!config_mqtt_enabled() || !mqttclient.connected()) {
    return;
  }

  JsonObject root = data.as<JsonObject>();
  for (JsonPair kv : root) {
    String topic = mqtt_topic + "/";
    topic += kv.key().c_str();
    String val = kv.value().as<String>();
    mqttclient.publish(topic.c_str(), val.c_str());
  }

  String ram_topic = mqtt_topic + "/" + node_name + "/" + mqtt_feed_prefix + "freeram";
  String free_ram = String(ESP.getFreeHeap());
  mqttclient.publish(ram_topic.c_str(), free_ram.c_str());

  Profile_End(mqtt_publish, 5);
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the WiFi
// -------------------------------------------------------------------
void mqtt_loop()
{
  Profile_Start(mqtt_loop);

  // Do we need to restart MQTT?
  if(mqttRestartTime > 0 && millis() > mqttRestartTime)
  {
    mqttRestartTime = 0;
    if (mqttclient.connected()) {
      DBUGF("Disconnecting MQTT");
      mqttclient.disconnect();
    }
    nextMqttReconnectAttempt = 0;
  }

  if(config_mqtt_enabled())
  {
    if (!mqttclient.connected()) {
      long now = millis();
      // try and reconnect every x seconds
      if (now > nextMqttReconnectAttempt) {
        nextMqttReconnectAttempt = now + MQTT_CONNECT_TIMEOUT;
        mqtt_connect(); // Attempt to reconnect
      }
    } else {
      // if MQTT connected
      mqttclient.loop();
    }
  }

  Profile_End(mqtt_loop, 5);
}

void mqtt_restart()
{
  // If connected disconnect MQTT to trigger re-connect with new details
  mqttRestartTime = millis();
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}
