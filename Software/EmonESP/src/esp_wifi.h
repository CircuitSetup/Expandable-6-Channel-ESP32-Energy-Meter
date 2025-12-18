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

#ifndef _EMONESP_ESP_WIFI_H
#define _EMONESP_ESP_WIFI_H

#include <Arduino.h>

#include <WiFi.h>
#include <ESPmDNS.h>              // Resolve URL for update server etc.

//was causing ESP to crash in AP mode
//#include <DNSServer.h>                // Required for captive portal

#ifndef WIFI_LED
//#define WIFI_LED 2
#endif

#ifdef WIFI_LED

#ifndef WIFI_LED_ON_STATE
#define WIFI_LED_ON_STATE LOW
#endif

//the time the LED actually stays on
#ifndef WIFI_LED_ON_TIME
#define WIFI_LED_ON_TIME 50
#endif

//times the LED is off...
#ifndef WIFI_LED_AP_TIME
#define WIFI_LED_AP_TIME 2000
#endif

#ifndef WIFI_LED_AP_CONNECTED_TIME
#define WIFI_LED_AP_CONNECTED_TIME 1000
#endif

#ifndef WIFI_LED_STA_CONNECTING_TIME
#define WIFI_LED_STA_CONNECTING_TIME 500
#endif

#ifndef WIFI_LED_STA_CONNECTED_TIME
#define WIFI_LED_STA_CONNECTED_TIME 4000
#endif

#endif

#ifndef WIFI_BUTTON
#define WIFI_BUTTON 3
#endif

#ifndef WIFI_BUTTON_AP_TIMEOUT
#define WIFI_BUTTON_AP_TIMEOUT              (5 * 1000)
#endif

#ifndef WIFI_BUTTON_FACTORY_RESET_TIMEOUT
#define WIFI_BUTTON_FACTORY_RESET_TIMEOUT   (10 * 1000)
#endif

#ifndef WIFI_CLIENT_DISCONNECT_RETRY
#define WIFI_CLIENT_DISCONNECT_RETRY         (10 * 1000)
#endif

#ifndef WIFI_CLIENT_RETRY_TIMEOUT
#define WIFI_CLIENT_RETRY_TIMEOUT           (5 * 60 * 1000) //5 min
#endif


// Last discovered WiFi access points
extern String st;
extern String rssi;

// Network state
extern String ipaddress;

// mDNS hostname
extern const char *esp_hostname;

extern void wifi_setup();
extern void wifi_loop();
extern void wifi_scan();

extern void wifi_restart();
extern void wifi_disconnect();

extern void wifi_turn_off_ap();
extern void wifi_turn_on_ap();
extern bool wifi_client_connected();

#define wifi_is_client_configured()   (WiFi.SSID() != "")

// Wifi mode
#define wifi_mode_is_sta()            (WIFI_STA == (WiFi.getMode() & WIFI_STA))
#define wifi_mode_is_sta_only()       (WIFI_STA == WiFi.getMode())
#define wifi_mode_is_ap()             (WIFI_AP == (WiFi.getMode() & WIFI_AP))

// Performing a scan enables STA so we end up in AP+STA mode so treat AP+STA with no
// ssid set as AP only
#define wifi_mode_is_ap_only()        ((WIFI_AP == WiFi.getMode()) || \
                                       (WIFI_AP_STA == WiFi.getMode() && !wifi_is_client_configured()))

#endif // _EMONESP_WIFI_H
