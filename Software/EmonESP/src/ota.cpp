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

#include "app_config.h"
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
//const char* u_host = "217.9.195.227";
//const char* u_url = "/esp/firmware.php";

void ota_setup()
{
  // Start local OTA update server
  ArduinoOTA.setHostname(node_name.c_str());
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
  //String url = u_url;
  //return get_http(u_host, url);
}

#ifdef ESP8266
t_httpUpdate_return ota_http_update()
{
  t_httpUpdate_return ret = ESPhttpUpdate.update("http://" + String(u_host) + String(u_url) + "?tag=" + currentfirmware);
  return ret;
}
#endif
