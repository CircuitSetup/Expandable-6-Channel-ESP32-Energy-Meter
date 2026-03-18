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
#include "esp_wifi.h"
#include "config.h"

#include <Network.h>

#if BOARD_PROFILE_SUPPORTS_ETHERNET
#include <ETH.h>
#include <SPI.h>
#endif

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

// set this to false if you do not want the ESP to go into SoftAP mode when the
// connection to the previously configured main AP is lost.
bool startAPonWifiDisconnect = BOARD_PROFILE_HAS_WIFI_STA;

// hostname for mDNS. Should work at least on windows. Try http://emonesp.local
const char *esp_hostname = "emonesp";

#ifdef WIFI_LED
int wifiLedState = !WIFI_LED_ON_STATE;
unsigned long wifiLedTimeOut = millis();
#endif

// Network Strings
String connected_network = "";
String ipaddress = "";

int client_disconnects = 0;
bool client_retry = false;
unsigned long client_retry_time = 0;

String st, rssi;

int wifiButtonState = HIGH;
unsigned long wifiButtonTimeOut = millis();
bool apMessage = false;

static bool mdnsStarted = false;
static bool ethernetLinkUp = false;
static bool ethernetHasIp = false;

#if BOARD_PROFILE_SUPPORTS_ETHERNET
static SPIClass ethernetSPI(HSPI);
#endif

static String ipToString(IPAddress address)
{
  char tmpStr[40];
  snprintf(tmpStr, sizeof(tmpStr), "%u.%u.%u.%u", address[0], address[1], address[2], address[3]);
  return String(tmpStr);
}

bool wifi_transport_is_ethernet()
{
  return BOARD_PROFILE_SUPPORTS_ETHERNET;
}

const char *wifi_transport_type()
{
  return wifi_transport_is_ethernet() ? "ethernet" : "wifi";
}

bool wifi_is_client_configured()
{
  return BOARD_PROFILE_HAS_WIFI_STA && WiFi.SSID() != "";
}

bool wifi_mode_is_sta()
{
  return BOARD_PROFILE_HAS_WIFI_STA && (WIFI_STA == (WiFi.getMode() & WIFI_STA));
}

bool wifi_mode_is_sta_only()
{
  return wifi_mode_is_sta() && !wifi_mode_is_ap() && !wifi_mode_is_eth();
}

bool wifi_mode_is_ap()
{
  return WIFI_AP == (WiFi.getMode() & WIFI_AP);
}

bool wifi_mode_is_ap_only()
{
  return wifi_mode_is_ap() && !wifi_client_connected();
}

bool wifi_mode_is_eth()
{
  return BOARD_PROFILE_SUPPORTS_ETHERNET && ethernetHasIp;
}

bool wifi_mode_is_eth_only()
{
  return wifi_mode_is_eth() && !wifi_mode_is_ap();
}

bool wifi_mode_is_eth_ap()
{
  return wifi_mode_is_eth() && wifi_mode_is_ap();
}

bool wifi_client_connected()
{
  if (wifi_mode_is_eth()) {
    return true;
  }
  return BOARD_PROFILE_HAS_WIFI_STA && WiFi.isConnected() && wifi_mode_is_sta();
}

bool wifi_link_up()
{
  if (BOARD_PROFILE_SUPPORTS_ETHERNET) {
    return ethernetLinkUp || ethernetHasIp;
  }
  return BOARD_PROFILE_HAS_WIFI_STA && WiFi.isConnected();
}

bool wifi_scan_supported()
{
  return BOARD_PROFILE_HAS_WIFI_STA;
}

bool wifi_should_authenticate_requests()
{
  return wifi_client_connected();
}

int wifi_signal_strength()
{
  return (BOARD_PROFILE_HAS_WIFI_STA && WiFi.isConnected()) ? WiFi.RSSI() : 0;
}

String wifi_mode_string()
{
  if (wifi_mode_is_eth_ap()) {
    return "ETH+AP";
  }
  if (wifi_mode_is_eth_only()) {
    return "ETH";
  }
  if (wifi_mode_is_sta() && wifi_mode_is_ap()) {
    return "STA+AP";
  }
  if (wifi_mode_is_sta_only()) {
    return "STA";
  }
  if (wifi_mode_is_ap()) {
    return "AP";
  }
  return "ERR";
}

static void updateActiveIpAddress()
{
  if (wifi_mode_is_eth()) {
#if BOARD_PROFILE_SUPPORTS_ETHERNET
    ipaddress = ipToString(ETH.localIP());
#endif
    return;
  }

  if (BOARD_PROFILE_HAS_WIFI_STA && WiFi.isConnected() && wifi_mode_is_sta()) {
    ipaddress = ipToString(WiFi.localIP());
    return;
  }

  if (wifi_mode_is_ap()) {
    ipaddress = ipToString(WiFi.softAPIP());
    return;
  }

  ipaddress = "";
}

static void ensureMdnsStarted()
{
  if (!mdnsStarted && MDNS.begin(esp_hostname)) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }

#if BOARD_PROFILE_SUPPORTS_ETHERNET
  if (mdnsStarted && wifi_mode_is_eth()) {
    MDNS.enableWorkstation(ESP_IF_ETH);
  }
#endif
}

// -------------------------------------------------------------------
// Start Access Point
// Access point is used for wifi network selection
// -------------------------------------------------------------------
static void startAP()
{
  DBUGS.println("Starting AP");

  if (BOARD_PROFILE_HAS_WIFI_STA) {
    wifi_disconnect();
  }

#ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
#endif

  wifi_scan();
  delay(100);
  WiFi.enableAP(true);
  delay(100);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  // Create Unique SSID e.g "emonESP_XXXXXX"
  String softAP_ssid_ID = String(softAP_ssid) + "_" + String((uint32_t)ESP.getEfuseMac());

  // Pick a random channel out of 1, 6 or 11
  int channel = (random(3) * 5) + 1;
  WiFi.softAP(softAP_ssid_ID.c_str(), softAP_password, channel);
  delay(200); // Without delay the IP address is sometimes blank

  /*
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  */

  updateActiveIpAddress();
  DBUGS.print("AP IP Address: ");
  DBUGS.println(ipaddress);

  apClients = 0;
  ensureMdnsStarted();
}

// -------------------------------------------------------------------
// Start Client, attempt to connect to Wifi network
// -------------------------------------------------------------------
static void startClient()
{
  if (!BOARD_PROFILE_HAS_WIFI_STA) {
    return;
  }

  DBUGS.print("Connecting to SSID: ");
  DBUGS.println(esid.c_str());

#ifdef WIFI_LED
  digitalWrite(WIFI_LED, LOW);
#endif

  WiFi.enableSTA(true);
  delay(100);
  WiFi.setHostname(esp_hostname);
  WiFi.begin(esid.c_str(), epass.c_str());
  WiFi.waitForConnectResult(); // yields until wifi connects or not

  delay(50);
}

#if BOARD_PROFILE_SUPPORTS_ETHERNET
static void startEthernet()
{
  DBUGS.println("Starting W5500 Ethernet");
  ethernetLinkUp = false;
  ethernetHasIp = false;

  ethernetSPI.begin(
    BOARD_PROFILE_ETH_SPI_SCK,
    BOARD_PROFILE_ETH_SPI_MISO,
    BOARD_PROFILE_ETH_SPI_MOSI,
    BOARD_PROFILE_ETH_CS
  );

  ETH.setHostname(esp_hostname);
  if (!ETH.begin(
        ETH_PHY_W5500,
        1,
        BOARD_PROFILE_ETH_CS,
        BOARD_PROFILE_ETH_INT,
        BOARD_PROFILE_ETH_RST,
        ethernetSPI,
        BOARD_PROFILE_ETH_SPI_FREQ_MHZ)) {
    DBUGS.println("Failed to start W5500 Ethernet");
  }
}
#endif

static void wifi_start()
{
  if (BOARD_PROFILE_SUPPORTS_ETHERNET) {
    startAP();
#if BOARD_PROFILE_SUPPORTS_ETHERNET
    startEthernet();
#endif
    return;
  }

  if (!wifi_is_client_configured()) {
    startAP();
  } else {
    startClient();
  }
}

static void onNetworkEvent(arduino_event_id_t event)
{
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      DBUGS.println("WiFi interface ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      DBUGS.println("Completed scan for access points");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      DBUGS.println("WiFi client started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      DBUGS.println("WiFi client stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      DBUGS.print("Connected to SSID: ");
      DBUGS.println(esid.c_str());
      client_disconnects = 0;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      DBUGS.println("Disconnected from WiFi access point");
      connected_network = "";
      updateActiveIpAddress();
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      connected_network = esid;
      updateActiveIpAddress();
      DBUGS.print("EmonESP IP: ");
      DBUGS.println(ipaddress);
      client_disconnects = 0;
      client_retry = false;
      ensureMdnsStarted();
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      DBUGS.println("Lost IP address and IP address is reset to 0");
      updateActiveIpAddress();
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      DBUGS.println("WiFi access point started");
      updateActiveIpAddress();
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      DBUGS.println("WiFi access point stopped");
      apClients = 0;
      updateActiveIpAddress();
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      DBUGS.println("Client connected");
      apClients++;
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      DBUGS.println("Client disconnected");
      if (apClients > 0) {
        apClients--;
      }
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      DBUGS.println("Assigned IP address to client");
      break;
#if BOARD_PROFILE_SUPPORTS_ETHERNET
    case ARDUINO_EVENT_ETH_START:
      DBUGS.println("Ethernet started");
      ETH.setHostname(esp_hostname);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      DBUGS.println("Ethernet link up");
      ethernetLinkUp = true;
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      DBUGS.println("Ethernet got IP");
      ethernetLinkUp = true;
      ethernetHasIp = true;
      updateActiveIpAddress();
      DBUGS.print("Ethernet IP: ");
      DBUGS.println(ipaddress);
      ensureMdnsStarted();
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      DBUGS.println("Ethernet link down");
      ethernetLinkUp = false;
      ethernetHasIp = false;
      updateActiveIpAddress();
      break;
    case ARDUINO_EVENT_ETH_STOP:
      DBUGS.println("Ethernet stopped");
      ethernetLinkUp = false;
      ethernetHasIp = false;
      updateActiveIpAddress();
      break;
#endif
    default:
      break;
  }
}

void wifi_setup()
{
#ifdef WIFI_LED
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, wifiLedState);
#endif

  randomSeed((uint32_t)esp_random());

  Network.onEvent(onNetworkEvent);
  wifi_start();
  ensureMdnsStarted();
}

void wifi_loop()
{
  bool isApOnly = wifi_mode_is_ap_only();
  bool isUplinkConnected = wifi_client_connected();

#ifdef WIFI_LED
  if ((isApOnly || !isUplinkConnected) && millis() > wifiLedTimeOut) {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);

    if (wifiLedState) {
      wifiLedTimeOut = millis() + WIFI_LED_ON_TIME;
    } else {
      int ledTime = isApOnly ? (0 == apClients ? WIFI_LED_AP_TIME : WIFI_LED_AP_CONNECTED_TIME) : WIFI_LED_STA_CONNECTING_TIME;
      wifiLedTimeOut = millis() + ledTime;
    }
  }

  if ((!isApOnly && isUplinkConnected) && millis() > wifiLedTimeOut) {
    wifiLedState = !wifiLedState;
    digitalWrite(WIFI_LED, wifiLedState);

    if (wifiLedState) {
      wifiLedTimeOut = millis() + WIFI_LED_ON_TIME;
    } else {
      wifiLedTimeOut = millis() + WIFI_LED_STA_CONNECTED_TIME;
    }
  }
#endif

  if (BOARD_PROFILE_ENABLE_BUTTON_ACTIONS) {
#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
    digitalWrite(WIFI_BUTTON, HIGH);
    pinMode(WIFI_BUTTON, INPUT_PULLUP);
#else
    pinMode(WIFI_BUTTON, INPUT_PULLUP);
#endif

    // Pressing the boot button for 5 seconds will turn on AP mode, 10 seconds will factory reset
    int button = digitalRead(WIFI_BUTTON);

#if defined(WIFI_LED) && WIFI_BUTTON == WIFI_LED
    pinMode(WIFI_BUTTON, OUTPUT);
    digitalWrite(WIFI_LED, wifiLedState);
#endif

    if (wifiButtonState != button) {
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

    if (LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_FACTORY_RESET_TIMEOUT) {
      DBUGS.println("Factory reset");
      delay(1000);

      config_reset();

      WiFi.disconnect(false, true);
      delay(50);
      esp_restart();
    } else if (!apMessage && LOW == wifiButtonState && millis() > wifiButtonTimeOut + WIFI_BUTTON_AP_TIMEOUT) {
      DBUGS.println("Access point");
      apMessage = true;
    }
  }

  if (startAPonWifiDisconnect && BOARD_PROFILE_HAS_WIFI_STA) {
    while (wifi_mode_is_sta_only() && !WiFi.isConnected()) {
      client_disconnects++; // set to 0 when connection to AP is made

      // If we have failed to connect 3 times, turn on the AP
      if (client_disconnects > 2) {
        DBUGS.println("Start AP if WiFi can not reconnect to AP");
        startAP();
        client_retry = true;
        client_retry_time = millis() + WIFI_CLIENT_RETRY_TIMEOUT;
        client_disconnects = 0;
      } else {
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

  // was causing ESP to crash in SoftAP mode
  // if (isApOnly) dnsServer.processNextRequest(); // Captive portal DNS redirect
}

void wifi_scan()
{
  st = "";
  rssi = "";

  if (!wifi_scan_supported()) {
    return;
  }

  int n = WiFi.scanNetworks();
  DBUGS.print(n);
  DBUGS.println(" networks found");

  for (int i = 0; i < n; ++i) {
    st += "\"" + WiFi.SSID(i) + "\"";
    rssi += "\"" + String(WiFi.RSSI(i)) + "\"";
    if (i < n - 1) {
      st += ",";
      rssi += ",";
    }
  }
}

void wifi_restart()
{
  DBUGS.println("Network restart called");
  wifi_disconnect();
  delay(50);
  wifi_start();
}

void wifi_disconnect()
{
  if (BOARD_PROFILE_HAS_WIFI_STA && wifi_mode_is_sta()) {
    DBUGS.println("WiFi disconnect called");
    WiFi.persistent(false);
    delay(50);
    WiFi.disconnect();
  }
}

void wifi_turn_off_ap()
{
  if (wifi_mode_is_ap()) {
    DBUGS.println("WiFi turn off AP called");
    WiFi.softAPdisconnect(true);
    apClients = 0;
    updateActiveIpAddress();
    // dnsServer.stop();
  }
}

void wifi_turn_on_ap()
{
  DBUGF("WiFi turn on AP called %d", WiFi.getMode());
  if (!wifi_mode_is_ap()) {
    startAP();
  }
}
