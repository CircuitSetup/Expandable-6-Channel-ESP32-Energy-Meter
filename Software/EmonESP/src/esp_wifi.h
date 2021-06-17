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

#ifndef _EMONESP_ESP_WIFI_H
#define _EMONESP_ESP_WIFI_H

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>              // Resolve URL for update server etc.
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>              // Resolve URL for update server etc.
#endif
#include <DNSServer.h>                // Required for captive portal


#ifdef ESP32
enum WiFiDisconnectReason
{
    WIFI_DISCONNECT_REASON_UNSPECIFIED              = 1,
    WIFI_DISCONNECT_REASON_AUTH_EXPIRE              = 2,
    WIFI_DISCONNECT_REASON_AUTH_LEAVE               = 3,
    WIFI_DISCONNECT_REASON_ASSOC_EXPIRE             = 4,
    WIFI_DISCONNECT_REASON_ASSOC_TOOMANY            = 5,
    WIFI_DISCONNECT_REASON_NOT_AUTHED               = 6,
    WIFI_DISCONNECT_REASON_NOT_ASSOCED              = 7,
    WIFI_DISCONNECT_REASON_ASSOC_LEAVE              = 8,
    WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED         = 9,
    WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD      = 10,  /* 11h */
    WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD     = 11,  /* 11h */
    WIFI_DISCONNECT_REASON_IE_INVALID               = 13,  /* 11i */
    WIFI_DISCONNECT_REASON_MIC_FAILURE              = 14,  /* 11i */
    WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,  /* 11i */
    WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,  /* 11i */
    WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS       = 17,  /* 11i */
    WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID     = 18,  /* 11i */
    WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID  = 19,  /* 11i */
    WIFI_DISCONNECT_REASON_AKMP_INVALID             = 20,  /* 11i */
    WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION    = 21,  /* 11i */
    WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP       = 22,  /* 11i */
    WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED       = 23,  /* 11i */
    WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED    = 24,  /* 11i */

    WIFI_DISCONNECT_REASON_BEACON_TIMEOUT           = 200,
    WIFI_DISCONNECT_REASON_NO_AP_FOUND              = 201,
    WIFI_DISCONNECT_REASON_AUTH_FAIL                = 202,
    WIFI_DISCONNECT_REASON_ASSOC_FAIL               = 203,
    WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT        = 204,
};

struct WiFiEventStationModeConnected
{
    String ssid;
    uint8_t bssid[6];
    uint8_t channel;
};

struct WiFiEventStationModeDisconnected
{
    String ssid;
    uint8_t bssid[6];
    WiFiDisconnectReason reason;
};

struct WiFiEventStationModeGotIP
{
    IPAddress ip;
    IPAddress mask;
    IPAddress gw;
};

struct WiFiEventSoftAPModeStationConnected
{
    uint8_t mac[6];
    uint8_t aid;
};

struct WiFiEventSoftAPModeStationDisconnected
{
    uint8_t mac[6];
    uint8_t aid;
};

#endif

// Last discovered WiFi access points
extern String st;
extern String rssi;

// Network state
extern String ipaddress;

extern void wifi_setup();
extern void wifi_loop();

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

#endif // _EMONESP_ESP_WIFI_H
