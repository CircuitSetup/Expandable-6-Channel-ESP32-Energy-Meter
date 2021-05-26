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

#ifndef _EMONESP_H
#define _EMONESP_H

#include <Arduino.h>
#include "debug.h"
#include "profile.h"

String getTime();

void setTimeOffset();

//#define ENABLE_WDT

#ifndef MAX_DATA_LEN
#define MAX_DATA_LEN 4096
#endif

#ifndef WIFI_LED
#define WIFI_LED 2
#endif

#ifdef WIFI_LED

#ifndef WIFI_LED_ON_STATE
#define WIFI_LED_ON_STATE LOW
#endif

//the time the LED actually stays on
#ifndef WIFI_LED_ON_TIME
#define WIFI_LED_ON_TIME 50
#endif

//time the LED is off in AP mode...
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

#ifndef WIFI_BUTTON
#define WIFI_BUTTON 3
#endif

#ifndef WIFI_BUTTON_PRESSED_STATE
#define WIFI_BUTTON_PRESSED_STATE LOW
#endif

#ifndef CONTROL_PIN
#define CONTROL_PIN 13
#endif

#ifndef CONTROL_PIN_ON_STATE
#define CONTROL_PIN_ON_STATE HIGH
#endif

#ifndef NODE_TYPE
#define NODE_TYPE "emonesp"
#endif

#ifndef NODE_DESCRIPTION
#define NODE_DESCRIPTION "WiFi EmonCMS Link for CircuitSetup Energy Meters"
#endif

#endif // _EMONESP_H
