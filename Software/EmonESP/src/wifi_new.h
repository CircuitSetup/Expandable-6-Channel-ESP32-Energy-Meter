#ifndef _EMONESP_WIFI_NEW_H
#define _EMONESP_WIFI_NEW_H

#ifdef ESP32
  #include <esp_wifi.h>
  #include <WiFi.h>
  #include <WiFiClient.h>

  #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

  #define LED_ON      HIGH
  #define LED_OFF     LOW  
#else
  #include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
  //needed for library
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>  

  #define ESP_getChipId()   (ESP.getChipId())

  #define LED_ON      LOW
  #define LED_OFF     HIGH
#endif

#endif // _EMONESP_WIFI_NEW_H

// SSID and PW for Config Portal
String ssid = "ESP_" + String(ESP_getChipId(), HEX);
const char* password = "";
bool isConnected = false;
String ipaddress = "";
String isAPMode = "";

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h> 
#define USE_AVAILABLE_PAGES     false

#include <ESP_WiFiManager.h>              //https://github.com/khoih-prog/ESP_WiFiManager

// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;
const char* esp_hn = "EmonESP";

void wifi_setup() 
{
  unsigned long startedAt = millis();

  //Local intialization. Once its business is done, there is no need to keep it around
  // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  //ESP_WiFiManager ESP_wifiManager;
  // Use this to personalize DHCP hostname (RFC952 conformed)
  ESP_WiFiManager ESP_wifiManager(esp_hn);
  
  ESP_wifiManager.setMinimumSignalQuality(-1);
  // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
  ESP_wifiManager.setRemoveDuplicateAPs(true);

  // We can't use WiFi.SSID() in ESP32as it's only valid after connected. 
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
  // Have to create a new function to store in EEPROM/SPIFFS for this purpose
  esid = ESP_wifiManager.WiFi_SSID();
  epass = ESP_wifiManager.WiFi_Pass();
  
  //Remove this line if you do not want to see WiFi password printed
  DBUGS.println("Stored: SSID = " + esid + ", Pass = " + epass);

  // SSID to uppercase 
  ssid.toUpperCase();
  
  if (esid == "")
  {
    DBUGS.println("We haven't got any access point credentials, so get them now");   
     
    
    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!ESP_wifiManager.startConfigPortal(ssid.c_str(), password)) {
      DBUGS.println("Not connected to WiFi but continuing anyway.");
      isConnected = false;
    } 
    else {
      DBUGS.println("WiFi connected...yeey :)");
      isConnected = true;    
    }
  }

  
  #define WIFI_CONNECT_TIMEOUT        30000L
  #define WHILE_LOOP_DELAY            200L
  #define WHILE_LOOP_STEPS            (WIFI_CONNECT_TIMEOUT / ( 3 * WHILE_LOOP_DELAY ))
  
  startedAt = millis();
  
  while ( (WiFi.status() != WL_CONNECTED) && (millis() - startedAt < WIFI_CONNECT_TIMEOUT ) )
  {   
    WiFi.mode(WIFI_STA);
    WiFi.persistent (true);
    // We start by connecting to a WiFi network
  
    DBUGS.println("Connecting to " + esid);
    
  
    WiFi.begin(esid.c_str(), epass.c_str());

    int i = 0;
    while((!WiFi.status() || WiFi.status() >= WL_DISCONNECTED) && i++ < WHILE_LOOP_STEPS)
    {
      delay(WHILE_LOOP_DELAY);
    }    
  }

  DBUGS.println("After waiting ");
  DBUGS.println((millis()- startedAt) / 1000);
  DBUGS.println(" secs more in setup(), connection result is ");

  if (WiFi.status() == WL_CONNECTED)
  {
    DBUGS.println("connected. Local IP: ");
    DBUGS.println(WiFi.localIP());
    ipaddress = (WiFi.localIP()).toString();
    isConnected = true;
    isAPMode = WiFi.getMode();
  }
  else
    DBUGS.println(ESP_wifiManager.getStatus(WiFi.status()));
    isConnected = false;
    isAPMode = WiFi.getMode();

}

void heartBeatPrint(void)
{
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print("H");        // H means connected to WiFi
  else
    Serial.print("F");        // F means not connected to WiFi
  
  if (num == 80) 
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0) 
  {
    Serial.print(" ");
  }
} 

void check_status()
{
  static ulong checkstatus_timeout = 0;

  #define HEARTBEAT_INTERVAL    10000L
  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = millis() + HEARTBEAT_INTERVAL;
  }
}

extern void wifi_loop() 
{
    //Local intialization. Once its business is done, there is no need to keep it around
    ESP_WiFiManager ESP_wifiManager;
    
    //Check if there is stored WiFi router/password credentials.
    //If not found, device will remain in configuration mode until switched off via webserver.
    DBUGS.println("Opening configuration portal. ");
    esid = ESP_wifiManager.WiFi_SSID();
    if (esid != "")
    {
      ESP_wifiManager.setConfigPortalTimeout(60); //If no access point name has been previously entered disable timeout.
      DBUGS.println("Got stored Credentials. Timeout 60s");
    }
    else
      DBUGS.println("No stored Credentials. No timeout");
    
    //it starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!ESP_wifiManager.startConfigPortal(ssid.c_str(), password)) 
    {
      DBUGS.println("Not connected to WiFi but continuing anyway.");
    } 
    else 
    {
      //if you get here you have connected to the WiFi
      DBUGS.println("connected...yeey :)");
    }
    isAPMode = WiFi.getMode();
    DBUGS.println("APMode: " + isAPMode);

  }