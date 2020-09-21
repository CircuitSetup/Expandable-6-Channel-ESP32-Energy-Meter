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
#ifndef _EMONESP_WEB_SERVER_H
#define _EMONESP_WEB_SERVER_H

#include <FS.h>                       // SPIFFS file-system: store web server html, CSS etc.
#include <Arduino.h>
#ifdef ESP32
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>  //https://github.com/me-no-dev/AsyncTCP
#elif defined(ESP8266)
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h> //https://github.com/me-no-dev/ESPAsyncWebServer


extern AsyncWebServer server;
extern AsyncWebSocket ws;
extern String currentfirmware;

extern void web_server_setup();
extern void web_server_loop();

extern void dumpRequest(AsyncWebServerRequest *request);

#endif // _EMONESP_WEB_SERVER_H
