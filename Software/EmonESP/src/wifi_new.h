#ifndef _EMONESP_WIFI_H
#define _EMONESP_WIFI_H
#include <Arduino.h>
#ifdef ESP32
  #include <esp_wifi.h>
  #include <WiFi.h>
  #include <WiFiClient.h>

  #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

  #define LED_ON      HIGH
  #define LED_OFF     LOW  
#endif
extern String esid;
extern String epass;
extern String ipaddress;
// mDNS hostname
extern const char *esp_hostname;
// SSID and PW for Config Portal
// String ssid = "ESP_" + String(ESP_getChipId(), HEX);
extern const char* password;
extern bool isConnected;
extern String ssid;


// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h> 
#define USE_AVAILABLE_PAGES     false

#include <ESP_WiFiManager.h>              //https://github.com/khoih-prog/ESP_WiFiManager

// Indicates whether ESP has WiFi credentials saved from previous session
extern bool initialConfig;



// Last discovered WiFi access points
extern String st;
extern String rssi;

// Network state
extern String ipaddress;

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
bool wifi_mode_is_sta(){if((WiFi.getMode() & WIFI_STA) == WIFI_STA){return true;}return false;}         
bool wifi_mode_is_sta_only(){if(WiFi.getMode() == WIFI_STA){return true;}return false;}//       (WiFi.getMode() == WIFI_STA)
bool wifi_mode_is_ap(){if((WiFi.getMode() & WIFI_AP) == WIFI_AP){return true;}return false;}//             ((WiFi.getMode() & WIFI_AP) == WIFI_AP)

// Performing a scan enables STA so we end up in AP+STA mode so treat AP+STA with no
// ssid set as AP only
bool wifi_mode_is_ap_only(){if((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() && !wifi_is_client_configured() == WIFI_AP_STA)){return true;}return false;}// ((WiFi.getMode() == WIFI_AP) || (WiFi.getMode() && !wifi_is_client_configured()) == WIFI_AP_STA))

// }        ((WiFi.getMode() == WIFI_AP) || 
//                                        (WiFi.getMode() && !wifi_is_client_configured() == WIFI_AP_STA))

#endif // _EMONESP_WIFI_NEW_H