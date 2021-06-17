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

#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_EMONCMS)
#undef ENABLE_DEBUG
#endif

#include "emonesp.h"
#include "emoncms.h"
#include "app_config.h"
#include "http.h"
#include "input.h"
#include "event.h"
#include "urlencode.h"

boolean emoncms_connected = false;
boolean emoncms_updated = false;

unsigned long packets_sent = 0;
unsigned long packets_success = 0;
unsigned long emoncms_connection_error_count = 0;

const char *e_url = "/input/post?";

static void emoncms_result(bool success, String message) {
  StaticJsonDocument<128> event;

  if(success) {
    packets_success++;
    emoncms_connection_error_count = 0;
  } else {
    if(++emoncms_connection_error_count > 30) {
      #ifdef ESP32
            esp_restart();
      #else
            ESP.restart();
      #endif
    }
  }

  if(emoncms_connected != success) {
    emoncms_connected = success;
    event[F("emoncms_connected")] = (int)emoncms_connected;
    event[F("emoncms_message")] = message.substring(0, 64);
    event_send(event);
  }
}

void emoncms_publish(JsonDocument &data) {
  Profile_Start(emoncms_publish);

  if (config_emoncms_enabled() && emoncms_apikey != 0) {
    String url = emoncms_path.c_str();
    String json;
    serializeJson(data, json);
    url += e_url;
    url += F("fulljson=");
    url += urlencode(json);
    url += F("&node=");
    url += emoncms_node;
    url += F("&apikey=");
    url += emoncms_apikey;

    DBUGVAR(url);

    packets_sent++;

    // Send data to Emoncms server
    String result = "";
    if (emoncms_fingerprint != 0) {
      // HTTPS on port 443 if HTTPS fingerprint is present
      DBUGLN(F("HTTPS Enabled"));
      //#ifdef ESP32
      //result = get_http(emoncms_server.c_str(), url, 443, emoncms_fingerprint.c_str());
      //#else
      result = get_https(emoncms_fingerprint.c_str(), emoncms_server.c_str(), url, 443);
      //#endif
    } else {
      // Plain HTTP if other emoncms server e.g EmonPi
      DBUGLN(F("Plain old HTTP"));
      result = get_http(emoncms_server.c_str(), url);
    }

    const size_t capacity = JSON_OBJECT_SIZE(2) + result.length();
    DynamicJsonDocument doc(capacity);
    if(DeserializationError::Code::Ok == deserializeJson(doc, result.c_str(), result.length())) {
      DBUGLN(F("Got JSON"));
      bool success = doc[F("success")]; // true
      emoncms_result(success, doc[F("message")]);
    } else if (result == F("ok")) {
      emoncms_result(true, result);
    } else {
      DEBUG.print(F("Emoncms error: "));
      DEBUG.println(result);
      emoncms_result(false, result);
    }
  } else {
    if(emoncms_connected) {
      emoncms_result(false, String("Disabled"));
    }
  }

  Profile_End(emoncms_publish, 10);
}
