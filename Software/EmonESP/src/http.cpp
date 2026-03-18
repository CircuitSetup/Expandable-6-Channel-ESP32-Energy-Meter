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

NetworkClient client;               // Create class for HTTP TCP connections get_http()
NetworkClientSecure client_ssl;     // Create class for HTTPS TCP connections get_https()

static char request[MAX_DATA_LEN+100];

static String execute_http_request(Client &http, const char * host, const char * url)
{
  snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url, host);
  http.print(request);

  unsigned long timeout = millis();
  while (http.available() == 0) {
    if (millis() - timeout > (HTTP_TIMEOUT * 1000)) {
      http.stop();
      return ("Client Timeout");
    }
#ifdef ENABLE_WDT
    feedLoopWDT();
#endif
  }

  while (http.available()) {
    String line = http.readStringUntil('\r');
    DBUGS.println(line);      //debug
    if (line.startsWith("HTTP/1.1 200 OK")) {
      return ("ok");
    }
  }

  return ("error " + String(host));
}

// -------------------------------------------------------------------
// HTTP or HTTPS GET Request
// url: N/A
// -------------------------------------------------------------------

String get_http(const char * host, const char * url, int port, const char * fingerprint) {
  if (fingerprint) {
    if (!client_ssl.connect(host, port, HTTP_TIMEOUT * 1000)) {
      DBUGS.printf("%s:%d\n", host, port);      //debug
      return ("Connection error");
    }
    client_ssl.setTimeout(HTTP_TIMEOUT * 1000);
    if (!client_ssl.verify(fingerprint, host)) {
      client_ssl.stop();
      return ("HTTPS fingerprint no match");
    }
    return execute_http_request(client_ssl, host, url);
  }

  if (!client.connect(host, port, HTTP_TIMEOUT * 1000)) {
    DBUGS.printf("%s:%d\n", host, port);      //debug
    return ("Connection error");
  }
  client.setTimeout(HTTP_TIMEOUT * 1000);
  return execute_http_request(client, host, url);
}
