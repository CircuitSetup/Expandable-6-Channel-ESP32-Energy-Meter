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
#include "wifi.h"
#include "config.h"

/*
DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
const byte DNS_PORT = 53;
*/

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
const char *softAP_ssid = "emonESP";
const char *softAP_password = "";
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
int apClients = 0;

//set this to false if you do not want the ESP to go into SoftAP mode
//when the connection to the previously configured main AP is lost.
//By default it will try to reconnect 3 times within 30 seconds after
//the connection to wifi is lost, turn on the soft AP, and then
//try to reconnect to the main AP every 5 min.
bool startAPonWifiDisconnect = true;

// hostname for mDNS. Should work at least on windows. Try http://emonesp.local
const char *esp_hostname = "emonesp";

#ifdef WIFI_LED
int wifiLedState = !WIFI_LED_ON_STATE;
unsigned long wifiLedTimeOut = millis();
#endif

// Wifi Network Strings
String connected_network = "";
String status_string = "";
String ipaddress = "";

int client_disconnects = 0;
bool client_retry = false;
unsigned long client_retry_time = 0;

unsigned long Timer;
String st, rssi;

int wifiButtonState = HIGH;
unsigned long wifiButtonTimeOut = millis();
bool apMessage = false;


// -------------------------------------------------------------------
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
void startAP() {
  DBUGS.println("Starting AP");

  wifi_disconnect();

  //turn off LED while doing wifi things
#ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
#endif

  wifi_scan();
  delay(100);
  WiFi.enableAP(true);
  delay(100);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
  String softAP_ssid_ID =
#ifdef ESP32
    String(softAP_ssid) + "_" + String((uint32_t)ESP.getEfuseMac());
#else
    String(softAP_ssid) + "_" + String(ESP.getChipId());
#endif

  // Pick a random channel out of 1, 6 or 11
  int channel = (random(3) * 5) + 1;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password, channel);
  delay(500); // Without delay the IP address is sometimes blank

  // Setup the DNS server redirecting all the domains to the apIP
  /*dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  */

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  DBUGS.print("AP IP Address: ");
  DBUGS.println(tmpStr);
  ipaddress = tmpStr;

  apClients = 0;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  DBUGS.print("Connecting to SSID: ");
  DBUGS.println(esid.c_str());
  // DBUGS.print(" epass:");
  // DBUGS.println(epass.c_str());

  //turn off LED while doing wifi things
#ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
#endif

  WiFi.enableSTA(true);
  delay(100);
  WiFi.begin(esid.c_str(), epass.c_str());
  WiFi.waitForConnectResult(); //yields until wifi connects or not
#ifdef ESP32
  WiFi.setHostname(esp_hostname);
#else
  WiFi.hostname(esp_hostname);
#endif

  delay(50);
}

static void wifi_start()
{
  // 1) If no network configured start up access point
  if (esid == 0 || esid == "")  {
    startAP();
  }
  // 2) else try and connect to the configured network
  else  {
    startClient();
  }
}

#ifdef ESP32

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_WIFI_READY:
      DBUGS.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      DBUGS.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      DBUGS.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      DBUGS.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      DBUGS.print("Connected to SSID: ");
      DBUGS.println(esid.c_str());
      client_disconnects = 0;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      DBUGS.println("Disconnected from WiFi access point");
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      DBUGS.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP: {
        IPAddress myAddress = WiFi.localIP();
        char tmpStr[40];
        sprintf(tmpStr, "%d.%d.%d.%d", myAddress[0], myAddress[1], myAddress[2], myAddress[3]);
        ipaddress = tmpStr;
        DBUGS.print("EmonESP IP: ");
        DBUGS.println(tmpStr);

        // Copy the connected network and ipaddress to global strings for use in status request
        connected_network = esid;

        // Clear any error state
        client_disconnects = 0;
        client_retry = false;
      }
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      DBUGS.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      DBUGS.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      DBUGS.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      DBUGS.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      DBUGS.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      DBUGS.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      DBUGS.println("WiFi access point stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      DBUGS.println("Client connected");
      apClients++;
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      DBUGS.println("Client disconnected");
      apClients--;
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      DBUGS.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      DBUGS.println("Received probe request");
      break;
    default:
      break;
  }
}

#else //ESP8266
void wifi_onStationModeGotIP(const WiFiEventStationModeGotIP &event)
{
  IPAddress myAddress = WiFi.localIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myAddress[0], myAddress[1], myAddress[2], myAddress[3]);
  ipaddress = tmpStr;
  DBUGS.print("Connected, IP: ");
  DBUGS.println(tmpStr);

  // Copy the connected network and ipaddress to global strings for use in status request
  connected_network = esid;

  // Clear any error state
  client_disconnects = 0;
  client_retry = false;
}

void wifi_onStationModeDisconnected(const WiFiEventStationModeDisconnected &event)
{
  DBUGF("WiFi disconnected: %s",
        WIFI_DISCONNECT_REASON_UNSPECIFIED == event.reason ? "WIFI_DISCONNECT_REASON_UNSPECIFIED" :
        WIFI_DISCONNECT_REASON_AUTH_EXPIRE == event.reason ? "WIFI_DISCONNECT_REASON_AUTH_EXPIRE" :
        WIFI_DISCONNECT_REASON_AUTH_LEAVE == event.reason ? "WIFI_DISCONNECT_REASON_AUTH_LEAVE" :
        WIFI_DISCONNECT_REASON_ASSOC_EXPIRE == event.reason ? "WIFI_DISCONNECT_REASON_ASSOC_EXPIRE" :
        WIFI_DISCONNECT_REASON_ASSOC_TOOMANY == event.reason ? "WIFI_DISCONNECT_REASON_ASSOC_TOOMANY" :
        WIFI_DISCONNECT_REASON_NOT_AUTHED == event.reason ? "WIFI_DISCONNECT_REASON_NOT_AUTHED" :
        WIFI_DISCONNECT_REASON_NOT_ASSOCED == event.reason ? "WIFI_DISCONNECT_REASON_NOT_ASSOCED" :
        WIFI_DISCONNECT_REASON_ASSOC_LEAVE == event.reason ? "WIFI_DISCONNECT_REASON_ASSOC_LEAVE" :
        WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED == event.reason ? "WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED" :
        WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD == event.reason ? "WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD" :
        WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD == event.reason ? "WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD" :
        WIFI_DISCONNECT_REASON_IE_INVALID == event.reason ? "WIFI_DISCONNECT_REASON_IE_INVALID" :
        WIFI_DISCONNECT_REASON_MIC_FAILURE == event.reason ? "WIFI_DISCONNECT_REASON_MIC_FAILURE" :
        WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT == event.reason ? "WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT" :
        WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT == event.reason ? "WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT" :
        WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS == event.reason ? "WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS" :
        WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID == event.reason ? "WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID" :
        WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID == event.reason ? "WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID" :
        WIFI_DISCONNECT_REASON_AKMP_INVALID == event.reason ? "WIFI_DISCONNECT_REASON_AKMP_INVALID" :
        WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION == event.reason ? "WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION" :
        WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP == event.reason ? "WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP" :
        WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED == event.reason ? "WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED" :
        WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED == event.reason ? "WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED" :
        WIFI_DISCONNECT_REASON_BEACON_TIMEOUT == event.reason ? "WIFI_DISCONNECT_REASON_BEACON_TIMEOUT" :
        WIFI_DISCONNECT_REASON_NO_AP_FOUND == event.reason ? "WIFI_DISCONNECT_REASON_NO_AP_FOUND" :
        WIFI_DISCONNECT_REASON_AUTH_FAIL == event.reason ? "WIFI_DISCONNECT_REASON_AUTH_FAIL" :
        WIFI_DISCONNECT_REASON_ASSOC_FAIL == event.reason ? "WIFI_DISCONNECT_REASON_ASSOC_FAIL" :
        WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT == event.reason ? "WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT" :
        "UNKNOWN");

  client_disconnects++;
}

#endif

void wifi_setup() {

#ifdef WIFI_LED
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  randomSeed(analogRead(0));

  // If we have an SSID configured at this point we have likely
  // been running another firmware, clear the results
  /*
    if(wifi_is_client_configured()) {
      WiFi.persistent(true);

      #ifdef ESP32
      WiFi.disconnect(false,true);
      #else
      WiFi.disconnect();
      ESP.eraseConfig();
      #endif
    }

    // Stop the WiFi module
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
  */
#ifdef ESP32
  WiFi.onEvent(WiFiEvent);

#else
  static auto _onStationModeConnected = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected & event) {
    DBUGF("Connected to %s", event.ssid.c_str());
  });
  static auto _onStationModeGotIP = WiFi.onStationModeGotIP(wifi_onStationModeGotIP);
  static auto _onStationModeDisconnected = WiFi.onStationModeDisconnected(wifi_onStationModeDisconnected);
  static auto _onSoftAPModeStationConnected = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected & event) {
    apClients++;
  });
  static auto _onSoftAPModeStationDisconnected = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected & event) {
    apClients--;
  });
#endif

  wifi_start();

  if (MDNS.begin(esp_hostname)) {
    MDNS.addService("http", "tcp", 80);
  }
}

void wifi_loop()
{
  bool isClient = wifi_mode_is_sta();
  bool isClientOnly = wifi_mode_is_sta_only();
  bool isAp = wifi_mode_is_ap();
  bool isApOnly = wifi_mode_is_ap_only();

  // flash the LED according to what state wifi is in
  // if AP mode & disconnected - blink every 2 seconds
  // if AP mode & someone is connected - blink fast
  // if Client mode - slow blink every 4 seconds

#ifdef WIFI_LED
  if ((isApOnly || !WiFi.isConnected()) && millis() > wifiLedTimeOut)  {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);

    if (wifiLedState) {
      wifiLedTimeOut = millis() + WIFI_LED_ON_TIME;
    }
    else {
      int ledTime = isApOnly ? (0 == apClients ? WIFI_LED_AP_TIME : WIFI_LED_AP_CONNECTED_TIME) : WIFI_LED_STA_CONNECTING_TIME;
      wifiLedTimeOut = millis() + ledTime;
    }
  }
  if ((isClientOnly || WiFi.isConnected()) && millis() > wifiLedTimeOut)  {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);

    if (wifiLedState) {
      wifiLedTimeOut = millis() + WIFI_LED_ON_TIME;
    }
    else {
      int ledTime = WIFI_LED_STA_CONNECTED_TIME;
      wifiLedTimeOut = millis() + ledTime;
    }
  }
#endif

#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
  digitalWrite(WIFI_BUTTON, HIGH);
  pinMode(WIFI_BUTTON, INPUT_PULLUP);
#endif

  // Pressing the boot button for 5 seconds will turn on AP mode, 10 seconds will factory reset
  int button = digitalRead(WIFI_BUTTON);

#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
  pinMode(WIFI_BUTTON, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  //DBUGF("%lu %d %d", millis() - wifiButtonTimeOut, button, wifiButtonState);
  if (wifiButtonState != button)
  {
    wifiButtonState = button;
    if (LOW == button) {
      DBUGS.println("Button pressed");
      wifiButtonTimeOut = millis();
      apMessage = false;
    } else {
      DBUGS.println("Button released");
      if (millis() > wifiButtonTimeOut + WIFI_BUTTON_AP_TIMEOUT) {
        wifi_turn_on_ap();
      }
    }
  }

  if (LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_FACTORY_RESET_TIMEOUT)
  {
    DBUGS.println("Factory reset");
    delay(1000);

    config_reset();

#ifdef ESP32
    WiFi.disconnect(false, true);
    delay(50);
    esp_restart();
#else
    WiFi.disconnect();
    ESP.eraseConfig();
    delay(50);
    ESP.reset();
#endif
  }
  else if (false == apMessage && LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_AP_TIMEOUT)
  {
    DBUGS.println("Access point");
    apMessage = true;
  }

  // Manage state while connecting

  if (startAPonWifiDisconnect) {
    while (wifi_mode_is_sta_only() && !wifi_mode_is_ap_only() && !WiFi.isConnected())
    {
      client_disconnects++; //set to 0 when connection to AP is made

      // If we have failed to connect 3 times, turn on the AP
      if (client_disconnects > 2) {
        DBUGS.println("Start AP if WiFi can not reconnect to AP");
        startAP();
        client_retry = true;
        client_retry_time = millis() + WIFI_CLIENT_RETRY_TIMEOUT;
        client_disconnects = 0;
      }
      else {
        // wait 10 seconds and retry
        delay(WIFI_CLIENT_DISCONNECT_RETRY);
        wifi_restart();
      }
#ifdef ENABLE_WDT
      feedLoopWDT();
#endif
    }
  }



  // Remain in AP mode if no one is connected for 5 Minutes before resetting
  if (isApOnly && 0 == apClients && client_retry && millis() > client_retry_time) {
    DBUGS.println("Try to connect to client again - resetting");
    delay(50);
    wifi_turn_off_ap();
    delay(50);
    wifi_restart();
  }

  //was causing ESP to crash in SoftAP mode
  //if (isApOnly) dnsServer.processNextRequest(); // Captive portal DNS re-dierct
}

void wifi_scan() {
  int n = WiFi.scanNetworks();
  DBUGS.print(n);
  DBUGS.println(" networks found");
  st = "";
  rssi = "";
  for (int i = 0; i < n; ++i) {
    st += "\"" + WiFi.SSID(i) + "\"";
    rssi += "\"" + String(WiFi.RSSI(i)) + "\"";
    if (i < n - 1)
      st += ",";
    if (i < n - 1)
      rssi += ",";
  }
}

void wifi_restart() {
  DBUGS.println("WiFi restart called");
  wifi_disconnect();
  delay(50);
  wifi_start();
}

void wifi_disconnect() {
  if (wifi_mode_is_sta()) {
    DBUGS.println("WiFi disconnect called");
    WiFi.persistent(false);
    delay(50);
    WiFi.disconnect();
  }
}

void wifi_turn_off_ap() {
  if (wifi_mode_is_ap())  {
    DBUGS.println("WiFi turn off AP called");
    WiFi.softAPdisconnect();
    //dnsServer.stop();
  }
}

void wifi_turn_on_ap() {
  DBUGF("WiFi turn on AP called %d", WiFi.getMode());
  if (!wifi_mode_is_ap()) {
    startAP();
  }
}

bool wifi_client_connected()
{
  return WiFi.isConnected() && (WIFI_STA == (WiFi.getMode() & WIFI_STA));
}
