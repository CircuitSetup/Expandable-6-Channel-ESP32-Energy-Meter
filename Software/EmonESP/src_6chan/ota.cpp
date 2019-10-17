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
#include "ota.h"
#include "web_server.h"
#include "wifi.h"
#include "http.h"

#include <FS.h>

#include <ArduinoOTA.h>               // local OTA update from Arduino IDE
#ifdef ESP32
#include <Update.h>        // remote OTA update from server
#elif defined(ESP8266)
#include <ESP8266httpUpdate.h>        // remote OTA update from server
#endif


// -------------------------------------------------------------------
//OTA UPDATE SETTINGS
// -------------------------------------------------------------------
//UPDATE SERVER strings and interfers for upate server
// Array of strings Used to check firmware version
const char* u_host = "192.168.1.124";
const char* u_url = "/esp/firmware.php";

extern const char *esp_hostname;

void ota_setup()
{
  // Start local OTA update server
  ArduinoOTA.setHostname(esp_hostname);
  ArduinoOTA.begin();
#ifdef WIFI_LED
  ArduinoOTA.onProgress([](unsigned int pos, unsigned int size) {
    static int state = LOW;
    state = !state;
    digitalWrite(WIFI_LED, state);
  });
#endif
}

void ota_loop()
{
  ArduinoOTA.handle();
}

String ota_get_latest_version()
{
  // Get latest firmware version number
  String url = u_url;
  return get_http(u_host, url);
}

#ifdef ESP8266
t_httpUpdate_return ota_http_update()
{
  SPIFFS.end(); // unmount filesystem
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  SPIFFS.begin(); //mount-file system
  return ret;
}
#endif
