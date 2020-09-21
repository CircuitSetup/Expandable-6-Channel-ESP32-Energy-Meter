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
#include "energy_meter.h"
#include "web_server.h"
#include "config.h"
#include "esp_wifi.h"
#include "mqtt.h"
#include "input.h"
#include "emoncms.h"
#include "ota.h"
#include "debug.h"

AsyncWebServer server(80);          //Create class for Web server
AsyncWebSocket ws("/ws");

bool enableCors = true;

// Event timeouts
unsigned long wifiRestartTime = 0;
unsigned long mqttRestartTime = 0;
unsigned long systemRestartTime = 0;
unsigned long systemRebootTime = 0;

static const char _DUMMY_PASSWORD[] PROGMEM = "_DUMMY_PASSWORD";
#define DUMMY_PASSWORD FPSTR(_DUMMY_PASSWORD)

#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)
String currentfirmware = ESCAPEQUOTE(BUILD_TAG);

void dumpRequest(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_GET) {
    DBUGF("GET");
  } else if (request->method() == HTTP_POST) {
    DBUGF("POST");
  } else if (request->method() == HTTP_DELETE) {
    DBUGF("DELETE");
  } else if (request->method() == HTTP_PUT) {
    DBUGF("PUT");
  } else if (request->method() == HTTP_PATCH) {
    DBUGF("PATCH");
  } else if (request->method() == HTTP_HEAD) {
    DBUGF("HEAD");
  } else if (request->method() == HTTP_OPTIONS) {
    DBUGF("OPTIONS");
  } else {
    DBUGF("UNKNOWN");
  }
  DBUGF(" http://%s%s", request->host().c_str(), request->url().c_str());

  if (request->contentLength()) {
    DBUGF("_CONTENT_TYPE: %s", request->contentType().c_str());
    DBUGF("_CONTENT_LENGTH: %u", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for (i = 0; i < headers; i++) {
    AsyncWebHeader* h = request->getHeader(i);
    DBUGF("_HEADER[%s]: %s", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for (i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if (p->isFile()) {
      DBUGF("_FILE[%s]: %s, size: %u", p->name().c_str(), p->value().c_str(), p->size());
    } else if (p->isPost()) {
      DBUGF("_POST[%s]: %s", p->name().c_str(), p->value().c_str());
    } else {
      DBUGF("_GET[%s]: %s", p->name().c_str(), p->value().c_str());
    }
  }
}

// -------------------------------------------------------------------
// Helper function to perform the standard operations on a request
// -------------------------------------------------------------------
bool requestPreProcess(AsyncWebServerRequest *request, AsyncResponseStream *&response, const char *contentType = "application/json")
{
  dumpRequest(request);

  if (wifi_mode_is_sta() && www_username != "" &&
      false == request->authenticate(www_username.c_str(), www_password.c_str())) {
    request->requestAuthentication(esp_hostname);
    return false;
  }

  response = request->beginResponseStream(String(contentType));
  if (enableCors) {
    response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
  }

  response->addHeader(F("Cache-Control"), F("no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0"));

  return true;
}

// -------------------------------------------------------------------
// Load Home page
// url: /
// -------------------------------------------------------------------
void handleHome(AsyncWebServerRequest *request) {
  if (www_username != ""
      && !request->authenticate(www_username.c_str(),
                                www_password.c_str())
      && wifi_mode_is_sta()) {
    return request->requestAuthentication();
  }

  if (SPIFFS.exists("/home.html")) {
    request->send(SPIFFS, "/home.html");
  } else {
    request->send(200, "text/plain",
                  "/home.html not found, have you flashed the SPIFFS?");
  }
}

// -------------------------------------------------------------------
// Wifi scan /scan not currently used
// url: /scan
//
// First request will return 0 results unless you start scan from somewhere else (loop/setup)
// Do not request more often than 3-5 seconds
// -------------------------------------------------------------------
void handleScan(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response)) {
    return;
  }

  String json = "[";
  int n = WiFi.scanComplete();
  if (n == -2) {
#ifdef ESP32
    WiFi.scanNetworks(true, true); //2nd true handles isHidden on ESP32
#else
    WiFi.scanNetworks(true);
#endif
  } else if (n) {
    for (int i = 0; i < n; ++i) {
      if (i) json += ",";
      json += "{";
      json += "\"rssi\":" + String(WiFi.RSSI(i));
      json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
      json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
      json += ",\"channel\":" + String(WiFi.channel(i));
      json += ",\"secure\":" + String(WiFi.encryptionType(i));
      json += "}";
    }
    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2) {
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  request->send(200, "text/json", json);
}

// -------------------------------------------------------------------
// Handle turning Access point off
// url: /apoff
// -------------------------------------------------------------------
void handleAPOff(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  response->setCode(200);
  response->print("Turning AP Off");
  request->send(response);

  DBUGLN("Turning AP Off");
  systemRebootTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Save selected network to EEPROM and attempt connection
// url: /savenetwork
// -------------------------------------------------------------------
void handleSaveNetwork(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  String qsid = request->arg("ssid");
  String qpass = request->arg("pass");

  if (qpass.equals(DUMMY_PASSWORD)) {
    qpass = epass;
  }

  if (qsid != 0) {
    config_save_wifi(qsid, qpass);

    response->setCode(200);
    response->print("saved");
    wifiRestartTime = millis() + 2000;
  } else {
    response->setCode(400);
    response->print("No SSID");
  }

  request->send(response);
}

// -------------------------------------------------------------------
// Save Emoncms
// url: /saveemoncms
// -------------------------------------------------------------------
void handleSaveEmoncms(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  String apikey = request->arg("apikey");
  if (apikey.equals(DUMMY_PASSWORD)) {
    apikey = emoncms_apikey;
  }

  config_save_emoncms(request->arg("server"),
                      request->arg("path"),
                      request->arg("node"),
                      apikey,
                      request->arg("fingerprint"));

  char tmpStr[200];
  snprintf(tmpStr, sizeof(tmpStr), "Saved: %s %s %s %s %s",
           emoncms_server.c_str(),
           emoncms_path.c_str(),
           emoncms_node.c_str(),
           emoncms_apikey.c_str(),
           emoncms_fingerprint.c_str());
  DBUGLN(tmpStr);

  response->setCode(200);
  response->print(tmpStr);
  request->send(response);
}

// -------------------------------------------------------------------
// Save MQTT Config
// url: /savemqtt
// -------------------------------------------------------------------
void handleSaveMqtt(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  String pass = request->arg("pass");
  if (pass.equals(DUMMY_PASSWORD)) {
    pass = mqtt_pass;
  }

  config_save_mqtt(request->arg("server"),
                   request->arg("topic"),
                   request->arg("prefix"),
                   request->arg("user"),
                   pass,
                   request->arg("json") != "false");

  char tmpStr[200];
  snprintf(tmpStr, sizeof(tmpStr), "Saved: %s %s %s %s %s", mqtt_server.c_str(),
           mqtt_topic.c_str(), mqtt_feed_prefix.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
  DBUGLN(tmpStr);

  response->setCode(200);
  response->print(tmpStr);
  request->send(response);

  // If connected disconnect MQTT to trigger re-connect with new details
  mqttRestartTime = millis();
}

// -------------------------------------------------------------------
// Save Calibration Config
// url: /savecal
// -------------------------------------------------------------------
void handleSaveCal(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  config_save_cal(request);

  response->setCode(200);
  request->send(response);

  // restart the system to load values into energy meter
  systemRestartTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Save the web site user/pass
// url: /saveadmin
// -------------------------------------------------------------------
void handleSaveAdmin(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  String quser = request->arg("user");
  String qpass = request->arg("pass");

  if (qpass.equals(DUMMY_PASSWORD)) {
    qpass = www_password;
  }

  config_save_admin(quser, qpass);

  response->setCode(200);
  response->print("saved");
  request->send(response);
}

// -------------------------------------------------------------------
// Last values on atmega serial
// url: /lastvalues
// -------------------------------------------------------------------
void handleLastValues(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  response->setCode(200);
  response->print(last_datastr);
  request->send(response);
}

// -------------------------------------------------------------------
// Returns status json
// url: /status
// -------------------------------------------------------------------
void handleStatus(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response)) {
    return;
  }

  String s = "{";
  if (wifi_mode_is_sta_only()) {
    s += "\"mode\":\"STA\",";
  } else if (wifi_mode_is_ap_only()) {
    s += "\"mode\":\"AP\",";
  } else if (wifi_mode_is_ap() && wifi_mode_is_sta()) {
    s += "\"mode\":\"STA+AP\",";
  }
  s += "\"networks\":[" + st + "],";
  s += "\"rssi\":[" + rssi + "],";

  s += "\"srssi\":\"" + String(WiFi.RSSI()) + "\",";
  s += "\"ipaddress\":\"" + ipaddress + "\",";
  s += "\"emoncms_connected\":\"" + String(emoncms_connected) + "\",";
  s += "\"packets_sent\":\"" + String(packets_sent) + "\",";
  s += "\"packets_success\":\"" + String(packets_success) + "\",";

  s += "\"mqtt_connected\":\"" + String(mqtt_connected()) + "\",";

  s += "\"free_heap\":\"" + String(ESP.getFreeHeap()) + "\"";

  s += "}";


  response->setCode(200);
  response->print(s);
  request->send(response);
}

// -------------------------------------------------------------------
// Returns EmonESP Config json
// url: /config
// -------------------------------------------------------------------
void handleConfig(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response)) {
    return;
  }

  String dummyPassword = String(DUMMY_PASSWORD);

  String s = "{";
  s += "\"espflash\":\"" + String(ESP.getFlashChipSize()) + "\",";
  s += "\"version\":\"" + currentfirmware + "\",";

  s += "\"ssid\":\"" + esid + "\",";
  s += "\"pass\":\"";
  if (epass != 0) {
    s += dummyPassword;
  }
  s += "\",";
  s += "\"emoncms_server\":\"" + emoncms_server + "\",";
  s += "\"emoncms_path\":\"" + emoncms_path + "\",";
  s += "\"emoncms_node\":\"" + emoncms_node + "\",";
  s += "\"emoncms_apikey\":\"";
  if (emoncms_apikey != 0) {
    s += dummyPassword;
  }
  s += "\",";
  s += "\"emoncms_fingerprint\":\"" + emoncms_fingerprint + "\",";
  s += "\"mqtt_server\":\"" + mqtt_server + "\",";
  s += "\"mqtt_topic\":\"" + mqtt_topic + "\",";
  s += "\"mqtt_feed_prefix\":\"" + mqtt_feed_prefix + "\",";
  s += "\"mqtt_user\":\"" + mqtt_user + "\",";
  s += "\"mqtt_pass\":\"";
  if (mqtt_pass != 0) {
    s += dummyPassword;
  }
  s += "\",";
  s += "\"mqtt_json\":\"";
  if (config_flags.mqtt_json) {
    s += "true";
  } else {
    s += "false";
  }
  s += "\",";
  s += "\"www_username\":\"" + www_username + "\",";
  s += "\"www_password\":\"";
  if (www_password != 0) {
    s += dummyPassword;
  }
  s += "\",";
  s += "\"voltage_cal\":\"" + String(voltage_cal) + "\",";
  s += "\"voltage2_cal\":\"" + String(voltage2_cal) + "\",";
  for (int i = 0; i < NUM_BOARDS; i ++)
  {
    byte tb[] = {1, 2, 4, 0};
    byte gain;

    gain = tb[gain_cal[i] & 0x3];
    s += "\"gain" + String(i*NUM_INPUTS+1) + "_cal\":\"" + gain + "\",";
    gain = tb[(gain_cal[i] >> 2) & 0x3];
    s += "\"gain" + String(i*NUM_INPUTS+2) + "_cal\":\"" + gain + "\",";
    gain = tb[(gain_cal[i] >> 4) & 0x3];
    s += "\"gain" + String(i*NUM_INPUTS+3) + "_cal\":\"" + gain + "\",";
    gain = tb[(gain_cal[i] >> 8) & 0x3];
    s += "\"gain" + String(i*NUM_INPUTS+4) + "_cal\":\"" + gain + "\",";
    gain = tb[(gain_cal[i] >> 10) & 0x3];
    s += "\"gain" + String(i*NUM_INPUTS+5) + "_cal\":\"" + gain + "\",";
    gain = tb[(gain_cal[i] >> 12) & 0x3];
    s += "\"gain" + String(i*NUM_INPUTS+6) + "_cal\":\"" + gain + "\",";
  }
  for (int i = 0; i < NUM_CHANNELS; i ++)
  {
    s += "\"ct" + String(i+1) + "_name\":\"" + ct_name[i] + "\",";
    s += "\"ct" + String(i+1) + "_cal\":\"" + ct_cal[i] + "\",";
    s += "\"cur" + String(i+1) + "_mul\":\"" + cur_mul[i] + "\",";
    s += "\"pow" + String(i+1) + "_mul\":\"" + pow_mul[i] + "\",";
  }
  s += "\"freq_cal\":\"" + String(freq_cal) + "\"";
  s += "}";

  response->setCode(200);
  response->print(s);
  request->send(response);
}

// -------------------------------------------------------------------
// Reset config and reboot
// url: /reset
// -------------------------------------------------------------------
void handleRst(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  config_reset();

#ifdef ESP32
  WiFi.disconnect(false, true);
#else
  WiFi.disconnect();
  ESP.eraseConfig();
#endif

  response->setCode(200);
  response->print("1");
  request->send(response);

  systemRebootTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Restart (Reboot)
// url: /restart
// -------------------------------------------------------------------
void handleRestart(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  response->setCode(200);
  response->print("1");
  request->send(response);

  systemRestartTime = millis() + 1000;
}

// -------------------------------------------------------------------
// Handle test input API
// url /input
// e.g http://192.168.0.75/input?string=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7
// -------------------------------------------------------------------
void handleInput(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  strcpy(input_string, request->arg("string").c_str());

  response->setCode(200);
  response->print(input_string);
  request->send(response);

  DBUGLN(input_string);
}

// -------------------------------------------------------------------
// Check for updates and display current version
// url: /firmware
// -------------------------------------------------------------------
void handleUpdateCheck(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response)) {
    return;
  }

  DBUGLN("Running: " + currentfirmware);
  // Get latest firmware version number
  // BUG/HACK/TODO: This will block, should be done in the loop call
  String latestfirmware = ota_get_latest_version();
  DBUGLN("Latest: " + latestfirmware);
  // Update web interface with firmware version(s)
  String s = "{";
  s += "\"current\":\"" + currentfirmware + "\",";
  s += "\"latest\":\"" + latestfirmware + "\"";
  s += "}";

  response->setCode(200);
  response->print(s);
  request->send(response);
}

// -------------------------------------------------------------------
// Update firmware
// url: /update
// -------------------------------------------------------------------
void handleUpdate(AsyncWebServerRequest *request) {
  // BUG/HACK/TODO: This will block, should be done in the loop call
#ifdef ENABLE_WEB_OTA
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/plain")) {
    return;
  }

  DBUGLN("UPDATING...");
  delay(500);

  t_httpUpdate_return ret = ota_http_update();

  int retCode = 400;
  String str = "Error";
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      str = "Update failed error (";
      str += httpUpdate.getLastError();
      str += "): ";
      str += httpUpdate.getLastErrorString();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      str = "No update, running latest firmware";
      break;
    case HTTP_UPDATE_OK:
      retCode = 200;
      str = "Update done!";
      break;
  }
  response->setCode(retCode);
  response->print(str);
  request->send(response);

  DBUGLN(str);
#endif
}

// -------------------------------------------------------------------
// Update firmware
// url: /update
// -------------------------------------------------------------------
void handleUpdateGet(AsyncWebServerRequest *request) {
  AsyncResponseStream *response;
  if (false == requestPreProcess(request, response, "text/html")) {
    return;
  }

  response->setCode(200);
  response->print(
    F("<html><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='update' accept='.bin'><input type='submit' value='Update Firmware'></form></html>"));
  request->send(response);
}

void handleUpdatePost(AsyncWebServerRequest *request) {
  bool shouldReboot = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "Update Complete. Rebooting in 20 seconds." : "Update FAIL");
  response->addHeader("Refresh", "20");
  response->addHeader("Location", "/");
  request->send(response);

  if (shouldReboot) {
    systemRestartTime = millis() + 1000;
  }
}

void handleUpdateUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

  if (!index) {
    DBUGF("Update Start: %s\n", filename.c_str());
#ifdef ESP32
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > 0) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
#ifdef ENABLE_DEBUG
      Update.printError(DEBUG_PORT);
#endif
    }
#elif defined(ESP8266)
    Update.runAsync(true);
    if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
#ifdef ENABLE_DEBUG
      Update.printError(DEBUG_PORT);
#endif
    }
#endif
  }

  if (Update.write(data, len) != len) {
#ifdef ENABLE_DEBUG
    Update.printError(DEBUG_PORT);
#endif
  }

  if (final) {
    if (Update.end(true)) {
      DBUGF("Update Success: %uB\n", index + len);
    } else {
#ifdef ENABLE_DEBUG
      Update.printError(DEBUG_PORT);
#endif
    }
  }
}


void handleNotFound(AsyncWebServerRequest *request)
{
  DBUG("NOT_FOUND: ");
  dumpRequest(request);

  if (wifi_mode_is_ap_only()) {
    // Redirect to the home page in AP mode (for the captive portal)
    AsyncResponseStream *response = request->beginResponseStream(String("text/html"));

    String url = F("http://");
    url += ipaddress;

    String s = F("<html><body><a href=\"");
    s += url;
    s += F("\">EmonESP</a></body></html>");

    response->setCode(301);
    response->addHeader(F("Location"), url);
    response->print(s);
    request->send(response);
  } else {
    request->send(404);
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {
    DBUGF("ws[%s][%u] connect", server->url(), client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT) {
    DBUGF("ws[%s][%u] disconnect: %u", server->url(), client->id());
  } else if(type == WS_EVT_ERROR) {
    DBUGF("ws[%s][%u] error(%u): %s", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG) {
    DBUGF("ws[%s][%u] pong[%u]: %s", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      DBUGF("ws[%s][%u] %s-message[%u]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", len);
    } else {
      // TODO: handle messages that are comprised of multiple frames or the frame is split into multiple packets
    }
  }
}

void web_server_setup()
{
  SPIFFS.begin(); // mount the fs

  // Add the Web Socket server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Setup the static files
  server.serveStatic("/", SPIFFS, "/")
  .setDefaultFile("index.html")
  .setAuthentication(www_username.c_str(), www_password.c_str());

  // Start server & server root html /
  server.on("/", handleHome);

  // Handle HTTP web interface button presses
  server.on("/generate_204", handleHome);  //Android captive portal. Maybe not needed. Might be handled by notFound
  server.on("/fwlink", handleHome);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound
  server.on("/status", handleStatus);

  server.on("/config", handleConfig);

  server.on("/savenetwork", handleSaveNetwork);
  server.on("/saveemoncms", handleSaveEmoncms);
  server.on("/savemqtt", handleSaveMqtt);
  server.on("/savecal", handleSaveCal);
  server.on("/saveadmin", handleSaveAdmin);

  server.on("/reset", handleRst);
  server.on("/restart", handleRestart);

  server.on("/scan", handleScan);
  server.on("/apoff", handleAPOff);
  server.on("/input", handleInput);
  server.on("/lastvalues", handleLastValues);

  // Simple Firmware Update Form
  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleUpdateGet(request);
  });
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    handleUpdatePost(request);
  },
  [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data,
     size_t len, bool final) {
    handleUpdateUpload(request, filename, index, data, len, final);
  });

  server.on("/firmware", handleUpdateCheck);
  server.on("/update", handleUpdate);

  server.onNotFound(handleNotFound);
  server.begin();
}

void web_server_loop() {
  // Do we need to restart the WiFi?
  if (wifiRestartTime > 0 && millis() > wifiRestartTime) {
    wifiRestartTime = 0;
    wifi_restart();
  }

  // Do we need to restart MQTT?
  if (mqttRestartTime > 0 && millis() > mqttRestartTime) {
    mqttRestartTime = 0;
    mqtt_restart();
  }

  // Do we need to restart the system?
  if (systemRestartTime > 0 && millis() > systemRestartTime) {
    systemRestartTime = 0;
    wifi_disconnect();
#ifdef ESP32
    esp_restart();
#else
    ESP.restart();
#endif
  }

  // Do we need to reboot the system?
  if (systemRebootTime > 0 && millis() > systemRebootTime) {
    systemRebootTime = 0;
    wifi_disconnect();
#ifdef ESP32
    esp_restart();
#else
    ESP.reset();
#endif
  }

  //clean up any stray web-sockets
  ws.cleanupClients();
}
