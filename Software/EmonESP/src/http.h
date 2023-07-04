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

#ifndef _EMONESP_HTTP_H
#define _EMONESP_HTTP_H

// -------------------------------------------------------------------
// HTTP(S) support functions
// -------------------------------------------------------------------

#include <Arduino.h>
#include <Print.h>
#ifdef ESP32
//#include <WiFiClient.h>         // http GET request
//#include <WiFiClientSecure.h>   // Secure https GET request
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>
#endif

/*
#ifdef ESP32
// -------------------------------------------------------------------
// HTTP or HTTPS GET Request
// url: N/A
// -------------------------------------------------------------------
extern String get_http(const char * host, String url, int port=80, const char * fingerprint=NULL);

#elif defined(ESP8266)
*/
// -------------------------------------------------------------------
// HTTPS SECURE GET Request
// url: N/A
// -------------------------------------------------------------------
extern String get_https(const char* fingerprint, const char* host, String &path, int httpsPort);

// -------------------------------------------------------------------
// HTTP GET Request
// url: N/A
// -------------------------------------------------------------------
extern String get_http(const char* host, String &path);
//#endif

#endif // _EMONESP_HTTP_H
