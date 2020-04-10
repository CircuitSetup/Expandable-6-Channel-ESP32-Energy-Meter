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
#include "http.h"

#include <Print.h>


#ifdef ESP32
#include <HTTPClient.h>
#else
#error Platform not supported
#endif


WiFiClientSecure client;        // Create class for HTTPS TCP connections get_https()
HTTPClient http;                // Create class for HTTP TCP connections get_http()

// -------------------------------------------------------------------
// HTTPS SECURE GET Request
// url: N/A
// -------------------------------------------------------------------

String
get_https(const char *fingerprint, const char *host, String url,
          int httpsPort) {
  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, httpsPort)) {
    DBUGS.print(host + httpsPort);      //debug
    return ("Connection error");
  }
#ifndef ESP32
#warning HTTPS verification not enabled
  if (client.verify(fingerprint, host)) {
#endif
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host +
                 "\r\n" + "Connection: close\r\n\r\n");
    // Handle wait for reply and timeout
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        client.stop();
        return ("Client Timeout");
      }
    }
    // Handle message receive
    while (client.available()) {
      String line = client.readStringUntil('\r');
      DBUGS.println(line);      //debug
      if (line.startsWith("HTTP/1.1 200 OK")) {
        return ("ok");
      }
    }
#ifndef ESP32
  } else {
    return ("HTTPS fingerprint no match");
  }
#endif
  return ("error " + String(host));
}

// -------------------------------------------------------------------
// HTTP GET Request
// url: N/A
// -------------------------------------------------------------------
String
get_http(const char *host, String url) {
  http.begin(String("http://") + host + String(url));
  int httpCode = http.GET();
  if ((httpCode > 0) && (httpCode == HTTP_CODE_OK)) {
    String payload = http.getString();
    DBUGS.println(payload);
    http.end();
    return (payload);
  } else {
    http.end();
    return ("server error: " + String(httpCode));
  }
}                               // end http_get
