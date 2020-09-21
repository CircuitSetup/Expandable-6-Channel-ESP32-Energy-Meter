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
#include "esp_wifi.h"
#include "http.h"

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
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
      {
        type = "filesystem";
        SPIFFS.end(); // unmount filesystem
      }
      DBUGS.println("Start updating " + type);
    })
    .onEnd([]() {
      DBUGS.println("\nOTA Update Complete");
      if (ArduinoOTA.getCommand() == U_SPIFFS)
        SPIFFS.begin();
    })
    #ifdef WIFI_LED
    .onProgress([](unsigned int pos, unsigned int size) {
      static int state = LOW;
      state = !state;
      digitalWrite(WIFI_LED, state);
    })
    #endif
    .onError([](ota_error_t error) {
      DBUGS.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) DBUGS.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) DBUGS.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) DBUGS.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) DBUGS.println("Receive Failed");
      else if (error == OTA_END_ERROR) DBUGS.println("End Failed");
    });

  ArduinoOTA.begin();
}

void ota_loop()
{
#ifdef ENABLE_WDT
  feedLoopWDT();
#endif
  ArduinoOTA.handle();
}

String ota_get_latest_version()
{
  // Get latest firmware version number
  return get_http(u_host, u_url);
}

t_httpUpdate_return ota_http_update()
{
  WiFiClient client;
  #ifdef ENABLE_WDT
    feedLoopWDT();
  #endif
  SPIFFS.end(); // unmount filesystem
  t_httpUpdate_return ret = httpUpdate.update(client,"http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  SPIFFS.begin(); //mount-file system
  return ret;
}
