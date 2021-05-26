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
 
#include "autoauth.h"
#include "emonesp.h"
#include "app_config.h"
#include "http.h"
#include <Arduino.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
unsigned int localUdpPort = 5005;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets

byte mqtt_auth_transfer_flag = 0;
unsigned long last_auth_request_attempt = 0;
 
void auth_request() {

    DEBUG.println(F("Fetching MQTT Auth"));
    DEBUG.println(mqtt_server.c_str());

    // Fetch emoncms mqtt broker credentials
    String url = F("/emoncms/device/auth/request.json");
    String result = "";
    String mqtt_username = "";
    String mqtt_password = "";
    String mqtt_basetopic = "";
    String mqtt_portnum = "";
    int stringpart = 0;
    
    // This needs to be done with an encrypted request otherwise credentials are shared as plain text
    result = get_http(mqtt_server.c_str(), url);
    if (result != F("request registered")) {
        for (int i=0; i<result.length(); i++) {
            char c = result[i];
            if (c==':') { 
                stringpart++; 
            } else {
                if (stringpart==0) mqtt_username += c;
                if (stringpart==1) mqtt_password += c;
                if (stringpart==2) mqtt_basetopic += c;
                if (stringpart==3) mqtt_portnum += c;
            }
        }
        // Only save if we received 3 setting parts (0,1,2)
        if (stringpart==2) {
            mqtt_auth_transfer_flag = 2;
            config_save_mqtt(true, mqtt_server.c_str(), mqtt_port, mqtt_basetopic, "", mqtt_username, mqtt_password);
            DEBUG.println(F("MQTT Settings:")); DEBUG.println(result);
        }
        
        if (stringpart==3) {
            mqtt_auth_transfer_flag = 2;
            config_save_mqtt(true, mqtt_server.c_str() ,mqtt_portnum.toInt(), mqtt_basetopic, "", mqtt_username, mqtt_password);
            DEBUG.println(F("MQTT Settings:")); DEBUG.println(result);
        }
    }

    delay(100);
}

void auth_setup() {
  Udp.begin(localUdpPort);
}

void auth_loop() {
    // UDP Broadcast receive 
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    DBUGF("Received %d bytes from %s, port %d", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    DBUGF("UDP packet contents: %s", incomingPacket);

    if (strcmp(incomingPacket,"emonpi.local")==0) {
      if (mqtt_server!=Udp.remoteIP().toString().c_str()) {
        config_save_mqtt_server(Udp.remoteIP().toString().c_str());        
        DBUGF("MQTT Server Updated");
        mqtt_auth_transfer_flag = 1;
        auth_request();
        // ---------------------------------------------------------------------------------------------------
      }
    }
  }

  if ((millis()-last_auth_request_attempt)>10000) {
      last_auth_request_attempt = millis();

      if (mqtt_auth_transfer_flag==1) {
          auth_request();
      }
  }
}
