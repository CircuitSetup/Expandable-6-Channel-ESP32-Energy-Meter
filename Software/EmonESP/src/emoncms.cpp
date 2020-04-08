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
#include "emoncms.h"
#include "config.h"
#include "http.h"
#include "wifi_local.h"

//EMONCMS SERVER strings
const char* e_url = "/input/post.json?json=";
boolean emoncms_connected = false;

unsigned long packets_sent = 0;
unsigned long packets_success = 0;
unsigned long emoncms_connection_error_count = 0;

void emoncms_publish(String data)
{
  // We now create a URL for server data upload
  String url = emoncms_path.c_str();
  url += e_url;
  url += "{";
  // Copy across, data length
  for (int i = 0; i < data.length(); ++i) {
    url += data[i];
  }
  url += ",psent:";
  url += packets_sent;
  url += ",psuccess:";
  url += packets_success;
  url += ",freeram:";
  url += String(ESP.getFreeHeap());
  url += "}&node=";
  url += emoncms_node;
  url += "&apikey=";
  url += emoncms_apikey;

  DBUGS.println(url); delay(10);
  packets_sent++;

  // Send data to Emoncms server
  String result = "";
  if (emoncms_fingerprint != 0) {
    // HTTPS on port 443 if HTTPS fingerprint is present
    DBUGS.println("HTTPS Enabled"); delay(10);
    result = get_https(emoncms_fingerprint.c_str(), emoncms_server.c_str(), url, 443);
  } else {
    // Plain HTTP if other emoncms server e.g EmonPi
    DBUGS.println("Plain old HTTP"); delay(10);
    result = get_http(emoncms_server.c_str(), url);
  }
  if (result == "ok") {
    packets_success++;
    emoncms_connected = true;
    emoncms_connection_error_count = 0;
  }
  else {
    emoncms_connected = false;
    DBUGS.print("Emoncms error: ");
    DBUGS.println(result);
    emoncms_connection_error_count ++;
    if (emoncms_connection_error_count > 30) {
#ifdef ESP32
      esp_restart();
#else
      ESP.restart();
#endif
    }
  }
}
