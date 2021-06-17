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

#include "emonesp.h"
#include "esp_wifi.h"
#include "app_config.h"

DNSServer dnsServer;                  // Create class DNS server, captive portal re-direct
static bool dnsServerStarted = false;
const byte DNS_PORT = 53;

// Access Point SSID, password & IP address. SSID will be softAP_ssid + chipID to make SSID unique
//const char *softAP_ssid = "emonESP";
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
  DBUGLN("Starting AP");

  if (wifi_mode_is_sta()) {
    WiFi.disconnect(true);
  }

  //turn off LED while doing wifi things
  #ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
  #endif

  #ifdef ESP32
  WiFi.persistent(false); //workaround for bug that causes a crash if a connection to wifi is lost and ESP32 has to go into AP mode
  #endif
  //WiFi.enableAP(true); not needed since WiFi.mode(WIFI_AP) calls this
  WiFi.mode(WIFI_AP);
  delay(500); // Without delay the IP address is sometimes blank

  // Create Unique SSID e.g "emonESP_XXXXXX"
  // String softAP_ssid_ID = String(softAP_ssid) + "_" + String(node_id);
  // Pick a random channel out of 1, 6 or 11
  int channel = (random(3) * 5) + 1;
  WiFi.softAP(node_name.c_str(), softAP_password, channel);
  delay(100);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  
  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServerStarted = dnsServer.start(DNS_PORT, "*", apIP);

  #ifdef ENABLE_WDT
  feedLoopWDT();
  #endif

  IPAddress myIP = WiFi.softAPIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
  DBUGF("AP IP Address: ",tmpStr);
  ipaddress = tmpStr;

  apClients = 0;
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
void startClient() {
  DEBUG.print(F("Connecting to SSID: "));
  DEBUG.println(esid.c_str());
  // DEBUG.print(" epass:");
  // DEBUG.println(epass.c_str());

  //turn off LED while doing wifi things
  #ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
  #endif

  WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(8,8,8,8)); 
  delay(10);

  #ifdef ESP8266
  WiFi.hostname(node_name.c_str());
  #else
  WiFi.persistent(false); //workaround for bug that causes a crash if a connection to wifi is lost
  WiFi.setHostname(node_name.c_str());
  #endif

  WiFi.begin(esid.c_str(), epass.c_str());
  WiFi.enableSTA(true);
  delay(100);

  #ifdef ENABLE_WDT
  feedLoopWDT();
  #endif

  WiFi.waitForConnectResult(); //yields until wifi connects or not

  delay(50);
}

static void wifi_start() {
  // 1) If no network configured start up access point
  DBUGVAR(esid);
  if (esid == 0 || esid == "")  {
    startAP();
  }
  // 2) else try and connect to the configured network
  else  {
    startClient();
  }
}

void wifi_onStationModeGotIP(const WiFiEventStationModeGotIP &event) {
  IPAddress myAddress = WiFi.localIP();
  char tmpStr[40];
  sprintf(tmpStr, "%d.%d.%d.%d", myAddress[0], myAddress[1], myAddress[2], myAddress[3]);
  ipaddress = tmpStr;
  DEBUG.print(F("Connected, IP: "));
  DEBUG.println(tmpStr);

  // Copy the connected network and ipaddress to global strings for use in status request
  connected_network = esid;

  // Clear any error state
  client_disconnects = 0;
  client_retry = false;

  if (MDNS.begin(node_name.c_str())) {
    MDNS.addService("http", "tcp", 80);
    #ifdef ESP8266 //uses LEA mDNS
    MDNSResponder::hMDNSService hService = MDNS.addService(NULL, "emonesp", "tcp", 80);
    if(hService) {
      MDNS.addServiceTxt(hService, "node_type", node_type.c_str());
    }
    #endif
  } else {
    DEBUG_PORT.println("Failed to start mDNS");
  }
}

void wifi_onStationModeDisconnected(const WiFiEventStationModeDisconnected &event) {
  DBUG("WiFi dissconnected: ");
  DBUGLN(
  WIFI_DISCONNECT_REASON_UNSPECIFIED == event.reason ? F("WIFI_DISCONNECT_REASON_UNSPECIFIED") :
  WIFI_DISCONNECT_REASON_AUTH_EXPIRE == event.reason ? F("WIFI_DISCONNECT_REASON_AUTH_EXPIRE") :
  WIFI_DISCONNECT_REASON_AUTH_LEAVE == event.reason ? F("WIFI_DISCONNECT_REASON_AUTH_LEAVE") :
  WIFI_DISCONNECT_REASON_ASSOC_EXPIRE == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_EXPIRE") :
  WIFI_DISCONNECT_REASON_ASSOC_TOOMANY == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_TOOMANY") :
  WIFI_DISCONNECT_REASON_NOT_AUTHED == event.reason ? F("WIFI_DISCONNECT_REASON_NOT_AUTHED") :
  WIFI_DISCONNECT_REASON_NOT_ASSOCED == event.reason ? F("WIFI_DISCONNECT_REASON_NOT_ASSOCED") :
  WIFI_DISCONNECT_REASON_ASSOC_LEAVE == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_LEAVE") :
  WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_NOT_AUTHED") :
  WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD == event.reason ? F("WIFI_DISCONNECT_REASON_DISASSOC_PWRCAP_BAD") :
  WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD == event.reason ? F("WIFI_DISCONNECT_REASON_DISASSOC_SUPCHAN_BAD") :
  WIFI_DISCONNECT_REASON_IE_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_IE_INVALID") :
  WIFI_DISCONNECT_REASON_MIC_FAILURE == event.reason ? F("WIFI_DISCONNECT_REASON_MIC_FAILURE") :
  WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_4WAY_HANDSHAKE_TIMEOUT") :
  WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_GROUP_KEY_UPDATE_TIMEOUT") :
  WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS == event.reason ? F("WIFI_DISCONNECT_REASON_IE_IN_4WAY_DIFFERS") :
  WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_GROUP_CIPHER_INVALID") :
  WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_PAIRWISE_CIPHER_INVALID") :
  WIFI_DISCONNECT_REASON_AKMP_INVALID == event.reason ? F("WIFI_DISCONNECT_REASON_AKMP_INVALID") :
  WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION == event.reason ? F("WIFI_DISCONNECT_REASON_UNSUPP_RSN_IE_VERSION") :
  WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP == event.reason ? F("WIFI_DISCONNECT_REASON_INVALID_RSN_IE_CAP") :
  WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED == event.reason ? F("WIFI_DISCONNECT_REASON_802_1X_AUTH_FAILED") :
  WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED == event.reason ? F("WIFI_DISCONNECT_REASON_CIPHER_SUITE_REJECTED") :
  WIFI_DISCONNECT_REASON_BEACON_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_BEACON_TIMEOUT") :
  WIFI_DISCONNECT_REASON_NO_AP_FOUND == event.reason ? F("WIFI_DISCONNECT_REASON_NO_AP_FOUND") :
  WIFI_DISCONNECT_REASON_AUTH_FAIL == event.reason ? F("WIFI_DISCONNECT_REASON_AUTH_FAIL") :
  WIFI_DISCONNECT_REASON_ASSOC_FAIL == event.reason ? F("WIFI_DISCONNECT_REASON_ASSOC_FAIL") :
  WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT == event.reason ? F("WIFI_DISCONNECT_REASON_HANDSHAKE_TIMEOUT") :
  F("UNKNOWN"));

  client_disconnects++;
  MDNS.end();
}

#ifdef ESP32
void wifi_onStationModeConnected(const WiFiEventStationModeConnected &event) {
  DBUGF("Connected to %s", event.ssid.c_str());
}

void wifi_onAPModeStationConnected(const WiFiEventSoftAPModeStationConnected &event) {
  apClients++;
};

void wifi_onAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &event) {
  apClients--;
};

void WiFiEvent(WiFiEvent_t event, system_event_info_t info) {
  DBUG("Got Network event: ");
  DBUGLN(
  SYSTEM_EVENT_WIFI_READY == event ? F("SYSTEM_EVENT_WIFI_READY") :
  SYSTEM_EVENT_SCAN_DONE == event ? F("SYSTEM_EVENT_SCAN_DONE") :
  SYSTEM_EVENT_STA_START == event ? F("SYSTEM_EVENT_STA_START") :
  SYSTEM_EVENT_STA_STOP == event ? F("SYSTEM_EVENT_STA_STOP") :
  SYSTEM_EVENT_STA_CONNECTED == event ? F("SYSTEM_EVENT_STA_CONNECTED") :
  SYSTEM_EVENT_STA_DISCONNECTED == event ? F("SYSTEM_EVENT_STA_DISCONNECTED") :
  SYSTEM_EVENT_STA_AUTHMODE_CHANGE == event ? F("SYSTEM_EVENT_STA_AUTHMODE_CHANGE") :
  SYSTEM_EVENT_STA_GOT_IP == event ? F("SYSTEM_EVENT_STA_GOT_IP") :
  SYSTEM_EVENT_STA_LOST_IP == event ? F("SYSTEM_EVENT_STA_LOST_IP") :
  SYSTEM_EVENT_STA_WPS_ER_SUCCESS == event ? F("SYSTEM_EVENT_STA_WPS_ER_SUCCESS") :
  SYSTEM_EVENT_STA_WPS_ER_FAILED == event ? F("SYSTEM_EVENT_STA_WPS_ER_FAILED") :
  SYSTEM_EVENT_STA_WPS_ER_TIMEOUT == event ? F("SYSTEM_EVENT_STA_WPS_ER_TIMEOUT") :
  SYSTEM_EVENT_STA_WPS_ER_PIN == event ? F("SYSTEM_EVENT_STA_WPS_ER_PIN") :
  SYSTEM_EVENT_AP_START == event ? F("SYSTEM_EVENT_AP_START") :
  SYSTEM_EVENT_AP_STOP == event ? F("SYSTEM_EVENT_AP_STOP") :
  SYSTEM_EVENT_AP_STACONNECTED == event ? F("SYSTEM_EVENT_AP_STACONNECTED") :
  SYSTEM_EVENT_AP_STADISCONNECTED == event ? F("SYSTEM_EVENT_AP_STADISCONNECTED") :
  SYSTEM_EVENT_AP_STAIPASSIGNED == event ? F("SYSTEM_EVENT_AP_STAIPASSIGNED") :
  SYSTEM_EVENT_AP_PROBEREQRECVED == event ? F("SYSTEM_EVENT_AP_PROBEREQRECVED") :
  SYSTEM_EVENT_GOT_IP6 == event ? F("SYSTEM_EVENT_GOT_IP6") :
  SYSTEM_EVENT_ETH_START == event ? F("SYSTEM_EVENT_ETH_START") :
  SYSTEM_EVENT_ETH_STOP == event ? F("SYSTEM_EVENT_ETH_STOP") :
  SYSTEM_EVENT_ETH_CONNECTED == event ? F("SYSTEM_EVENT_ETH_CONNECTED") :
  SYSTEM_EVENT_ETH_DISCONNECTED == event ? F("SYSTEM_EVENT_ETH_DISCONNECTED") :
  SYSTEM_EVENT_ETH_GOT_IP == event ? F("SYSTEM_EVENT_ETH_GOT_IP") :
  F("UNKNOWN"));

  switch (event) {
    case SYSTEM_EVENT_WIFI_READY:
      DEBUG.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      DEBUG.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
    {
      DEBUG.println("WiFi client started");
      if(WiFi.setHostname(node_name.c_str())) {
        DBUGF("Set host name to %s", WiFi.getHostname());
      } else {
        DBUGF("Setting host name failed: %s", node_name.c_str());
      }
    } break;
    case SYSTEM_EVENT_STA_STOP:
      DEBUG.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
    {
      auto& src = info.connected;
      WiFiEventStationModeConnected dst;
      dst.ssid = String(reinterpret_cast<char*>(src.ssid));
      memcpy(dst.bssid, src.bssid, 6);
      dst.channel = src.channel;
      wifi_onStationModeConnected(dst);
    } break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    {
      auto& src = info.disconnected;
      WiFiEventStationModeDisconnected dst;
      dst.ssid = String(reinterpret_cast<char*>(src.ssid));
      memcpy(dst.bssid, src.bssid, 6);
      dst.reason = static_cast<WiFiDisconnectReason>(src.reason);
      wifi_onStationModeDisconnected(dst);
    } break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      DEBUG.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
    {
      auto& src = info.got_ip.ip_info;
      WiFiEventStationModeGotIP dst;
      dst.ip = src.ip.addr;
      dst.mask = src.netmask.addr;
      dst.gw = src.gw.addr;
      wifi_onStationModeGotIP(dst);
    } break;
    case SYSTEM_EVENT_STA_LOST_IP:
      DEBUG.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      DEBUG.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      DEBUG.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      DEBUG.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      DEBUG.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
    {
      if(WiFi.softAPsetHostname(esp_hostname)) {
        DBUGF("Set host name to %s", WiFi.softAPgetHostname());
      } else {
        DBUGF("Setting host name failed: %s", esp_hostname);
      }
    } break;
    case SYSTEM_EVENT_AP_STOP:
      DEBUG.println("WiFi access point stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
    {
      auto& src = info.sta_connected;
      WiFiEventSoftAPModeStationConnected dst;
      memcpy(dst.mac, src.mac, 6);
      dst.aid = src.aid;
      wifi_onAPModeStationConnected(dst);
    } break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    {
      auto& src = info.sta_disconnected;
      WiFiEventSoftAPModeStationDisconnected dst;
      memcpy(dst.mac, src.mac, 6);
      dst.aid = src.aid;
      wifi_onAPModeStationDisconnected(dst);
    } break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      DEBUG.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      DEBUG.println("Received probe request");
      break;
    default:
      break;
  }
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
  static auto _onStationModeConnected = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &event) { DBUGF("Connected to %s", event.ssid.c_str()); });
  static auto _onStationModeGotIP = WiFi.onStationModeGotIP(wifi_onStationModeGotIP);
  static auto _onStationModeDisconnected = WiFi.onStationModeDisconnected(wifi_onStationModeDisconnected);
  static auto _onSoftAPModeStationConnected = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected &event) {
    apClients++;
  });
  static auto _onSoftAPModeStationDisconnected = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected &event) {
    apClients--;
  });
#endif

  wifi_start();

  client_retry_time = millis();
}

void wifi_loop()
{
  Profile_Start(wifi_loop);

  bool isClientOnly = wifi_mode_is_sta_only();
  bool isApOnly = wifi_mode_is_ap_only();

  // flash the LED according to what state wifi is in
  // if AP mode & disconnected - blink every 2 seconds
  // if AP mode & someone is connected - blink fast
  // if Client mode - slow blink every 4 seconds

#ifdef WIFI_LED
  //if AP mode only
  if ((isApOnly || !WiFi.isConnected()) && millis() > wifiLedTimeOut) {
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
  //if connected to wifi
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

  // Pressing the WIFI_BUTTON for 5 seconds will turn on AP mode, 10 seconds will factory reset
  int button = digitalRead(WIFI_BUTTON);

#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
  pinMode(WIFI_BUTTON, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  //DBUGF("%lu %d %d", millis() - wifiButtonTimeOut, button, wifiButtonState);
  if (wifiButtonState != button) {
    wifiButtonState = button;
    if (LOW == button) {
      DEBUG.println("Button pressed");
      wifiButtonTimeOut = millis();
      apMessage = false;
    } else {
      DEBUG.println("Button released");
      if (millis() > wifiButtonTimeOut + WIFI_BUTTON_AP_TIMEOUT) {
        wifi_turn_on_ap();
      }
    }
  }

  if (LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_FACTORY_RESET_TIMEOUT) {
    DEBUG.println("Factory reset");
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
  else if (false == apMessage && LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_AP_TIMEOUT) {
    DEBUG.println("Access point");
    apMessage = true;
  }

  // Manage state while connecting
  if (startAPonWifiDisconnect) {
    if (isClientOnly && !WiFi.isConnected()) {
      // If we have failed to connect 3 times, turn on the AP
      if (client_disconnects > 2) {
        DEBUG.println("Start AP if WiFi can not reconnect to AP");
        startAP();
        client_retry = true;
        client_retry_time = millis() + WIFI_CLIENT_RETRY_TIMEOUT;
      }
      else {
        // wait 10 seconds and retry
#ifdef ENABLE_WDT
        // so watchdog (hard coded to 5 seconds) is not triggered by delay
        if (WIFI_CLIENT_DISCONNECT_RETRY >= 5000) {
          int disconnect_retry;
          int dr_div;
          int i = 0;
          dr_div = WIFI_CLIENT_DISCONNECT_RETRY/1000;
          disconnect_retry = WIFI_CLIENT_DISCONNECT_RETRY/dr_div;
          DEBUG.print("disconnect retry time: ");
          DEBUG.println(disconnect_retry);
          while (i < dr_div) {
            delay(disconnect_retry);
            feedLoopWDT();
            i++;
          }
        }
        else {
#endif
          delay(WIFI_CLIENT_DISCONNECT_RETRY);
#ifdef ENABLE_WDT
        }
#endif
        wifi_restart();
      }
#ifdef ENABLE_WDT
      feedLoopWDT();
#endif
    }
  }

  // Remain in AP mode if no one is connected for 5 Minutes before resetting
  if (isApOnly && 0 == apClients && client_retry && millis() > client_retry_time) {
    DEBUG.println("Try to connect to client again - resetting");
    wifi_turn_off_ap();
    delay(50);
#ifdef ESP32
    esp_restart();
#else
    ESP.reset();
#endif
  }

  if(dnsServerStarted) {
    dnsServer.processNextRequest(); // Captive portal DNS re-dierct
  }

  Profile_End(wifi_loop, 5);
}

void wifi_restart() {
  DEBUG.println("WiFi restart called");
  wifi_disconnect();
  delay(50);
  wifi_start();
}

void wifi_disconnect() {
  if (wifi_mode_is_sta()) {
    DEBUG.println("WiFi disconnect called");
    WiFi.persistent(false);
    delay(50);
    WiFi.disconnect();
  }
}

void wifi_turn_off_ap() {
  if (wifi_mode_is_ap())  {
    DEBUG.println("WiFi turn off AP called");
    WiFi.softAPdisconnect();
    //#ifdef ESP8266
    dnsServer.stop();
    dnsServerStarted = false;
    //#endif
  }
}

void wifi_turn_on_ap() {
  DBUGF("WiFi turn on AP called %d", WiFi.getMode());
  if (!wifi_mode_is_ap()) {
    startAP();
  }
}

bool wifi_client_connected() {
  return WiFi.isConnected() && (WIFI_STA == (WiFi.getMode() & WIFI_STA));
}
