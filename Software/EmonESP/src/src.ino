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

#ifdef ENABLE_WDT
#include <esp_task_wdt.h>
#endif
#include "emonesp.h"
#include "app_config.h"
#include "esp_wifi.h"
#include "web_server.h"
#include "ota.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"
#include "http.h"
#include "autoauth.h"
#include <NTPClient.h>
#include "energy_meter.h"
// See energy meter specific configuration in energy_meter.h

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"europe.pool.ntp.org",time_offset,60000);
unsigned long last_ctrl_update = 0;
unsigned long last_pushbtn_check = 0;
bool pushbtn_action = 0;
bool pushbtn_state = 0;
bool last_pushbtn_state = 0;

static uint32_t last_mem = 0;
static uint32_t start_mem = 0;
static unsigned long mem_info_update = 0;

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {

  #ifdef ENABLE_WDT
  enableLoopWDT();
  #endif

  debug_setup();

  DEBUG.println();
  DEBUG.println();
  DEBUG.print("EmonESP ");
  DEBUG.println(node_name.c_str());
  DEBUG.println("Firmware: " + currentfirmware);
  DEBUG.printf("Free: %d\n", ESP.getFreeHeap());

  DBUG("Node type: ");
  DBUGLN(node_type);

  // Read saved settings from the config
  config_load_settings();
  timeClient.setTimeOffset(time_offset);

  DBUGF("After config_load_settings: %d", ESP.getFreeHeap());

  DBUG("Node name: ");
  DBUGLN(node_name);

  // ---------------------------------------------------------
  // pin setup
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, !WIFI_LED_ON_STATE);

  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, !CONTROL_PIN_ON_STATE);

  // custom: analog output pin
  #ifdef VOLTAGE_OUT_PIN
  pinMode(4, OUTPUT);
  #endif
  // ---------------------------------------------------------

  // Initial LED on
  led_flash(3000, 100);

  // Initialise the WiFi
  wifi_setup();
  DBUGF("After wifi_setup: %d", ESP.getFreeHeap());
  led_flash(50, 50);

  // Bring up the web server
  web_server_setup();
  DBUGF("After web_server_setup: %d", ESP.getFreeHeap());
  led_flash(50, 50);

  // Start the OTA update systems
  ota_setup();
  DBUGF("After ota_setup: %d", ESP.getFreeHeap());

  // Start auto auth
  auth_setup();
  DBUGF("After auth_setup: %d", ESP.getFreeHeap());

    // Time
  timeClient.begin();
  DBUGF("After timeClient.begin: %d", ESP.getFreeHeap());

  energy_meter_setup();

  #ifdef ENABLE_WDT
  DEBUG.println("Watchdog timer is enabled.");
  feedLoopWDT();
  #endif

  delay(100);

  start_mem = last_mem = ESP.getFreeHeap();
} // end setup

void led_flash(int ton, int toff) {
  digitalWrite(WIFI_LED, WIFI_LED_ON_STATE);
  delay(ton);
  digitalWrite(WIFI_LED, WIFI_LED_ON_STATE);
  delay(toff);
}

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop()
{
  #ifdef ENABLE_WDT
  feedLoopWDT();
  #endif

  if (millis() > mem_info_update) {
    mem_info_update = millis() + 2000;
    uint32_t current = ESP.getFreeHeap();
    int32_t diff = (int32_t)(last_mem - current);
    if(diff != 0) {
      DEBUG.printf("Free memory %u - diff %d %d\n", current, diff, start_mem - current);
      last_mem = current;
    }
  }

  ota_loop();
  web_server_loop();
  wifi_loop();
  timeClient.update();
  energy_meter_loop();

  StaticJsonDocument<512> data;
  boolean gotInput = input_get(data);

  if (wifi_client_connected()) {
    mqtt_loop();
    if(gotInput) {
      emoncms_publish(data);
      event_send(data);
    }
  }

  auth_loop();

  // --------------------------------------------------------------
  // CONTROL UPDATE
  // --------------------------------------------------------------
  if ((millis()-last_ctrl_update)>1000 || ctrl_update) {
    last_ctrl_update = millis();
    ctrl_update = 0;
    ctrl_state = 0; // default off

    // 1. Timer
    int timenow = timeClient.getHours()*100+timeClient.getMinutes();
    
    if (timer_stop1>=timer_start1 && (timenow>=timer_start1 && timenow<timer_stop1)) ctrl_state = 1;
    if (timer_stop2>=timer_start2 && (timenow>=timer_start2 && timenow<timer_stop2)) ctrl_state = 1;

    if (timer_stop1<timer_start1 && (timenow>=timer_start1 || timenow<timer_stop1)) ctrl_state = 1;
    if (timer_stop2<timer_start2 && (timenow>=timer_start2 || timenow<timer_stop2)) ctrl_state = 1;    

    // 2. On/Off
    if (ctrl_mode=="On") ctrl_state = 1;
    if (ctrl_mode=="Off") ctrl_state = 0;

    // 3. Apply
    if (ctrl_state) {
      // ON
      digitalWrite(CONTROL_PIN, CONTROL_PIN_ON_STATE);
    } else {
      digitalWrite(CONTROL_PIN, !CONTROL_PIN_ON_STATE);
    }

    #ifdef VOLTAGE_OUT_PIN
    analogWrite(VOLTAGE_OUT_PIN, voltage_output);
    #endif
  }
  
  // --------------------------------------------------------------
  if ((millis()-last_pushbtn_check)>100) {
    last_pushbtn_check = millis();

    last_pushbtn_state = pushbtn_state;
    pushbtn_state = !digitalRead(0);

    if (pushbtn_state && last_pushbtn_state && !pushbtn_action) {
      pushbtn_action = 1;
      if (ctrl_mode=="On") ctrl_mode = "Off"; else ctrl_mode = "On";
      if (mqtt_server!=0) mqtt_publish("out/ctrlmode",String(ctrl_mode));

    }
    if (!pushbtn_state && !last_pushbtn_state) pushbtn_action = 0;
  }

} // end loop

String getTime() {
    return timeClient.getFormattedTime();
}

void setTimeOffset() {
    timeClient.setTimeOffset(time_offset);
}

void event_send(String &json)
{
  StaticJsonDocument<512> event;
  deserializeJson(event, json);
  event_send(event);
}

void event_send(JsonDocument &event)
{
  #ifdef ENABLE_DEBUG
  serializeJson(event, DEBUG_PORT);
  DBUGLN("");
  #endif
  web_server_event(event);
  mqtt_publish(event);
}
