/*
   -------------------------------------------------------------------
   EmonESP Serial to Emoncms gateway
   -------------------------------------------------------------------
   Created for use with the CircuitSetup.us Split Phase Energy Meter by jdeglavina

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

#include "energy_meter.h"
#include "emonesp.h"
#include "emoncms.h"
#include "input.h"
#include "config.h"
#include <WiFi.h>
#include "mqtt.h"
#include "json.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32.h>

#ifdef ENABLE_OLED_DISPLAY
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_DC     0
#define OLED_CS     16
#define OLED_RESET  2 //17
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);
#endif

unsigned long startMillis;
unsigned long currentMillis;
const int period = 1000; //time interval in ms to send data

/****** Chip Select Pins ******/
/*
   Each chip has its own CS pin (2 per board). The main board must be pins 5 and 4.
*/
const int CS1[NUM_BOARDS] = { 5, 0, 27, 2, 13, 14, 15 };
const int CS2[NUM_BOARDS] = { 4, 16, 17, 21, 22, 25, 26 };

/* Initialize ATM90E32 library for each IC */
ATM90E32 sensor_ic1[NUM_BOARDS]{};
ATM90E32 sensor_ic2[NUM_BOARDS]{};

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void energy_meter_setup() {
  int i;

  /*Initialise the ATM90E32 & Pass CS pin and calibrations to its library */
  Serial.println("Start ATM90E32");
  for (i = 0; i < NUM_BOARDS; i ++)
  {
    sensor_ic1[i].begin(CS1[i], freq_cal, gain_cal[i] & 0xFF, voltage_cal, ct_cal[i*NUM_INPUTS+0], ct_cal[i*NUM_INPUTS+1], ct_cal[i*NUM_INPUTS+2]);
    sensor_ic2[i].begin(CS2[i], freq_cal, gain_cal[i] >> 8, voltage2_cal, ct_cal[i*NUM_INPUTS+3], ct_cal[i*NUM_INPUTS+4], ct_cal[i*NUM_INPUTS+5]);
    delay(200);
  }

#ifdef ENABLE_OLED_DISPLAY
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("OLED allocation failed"));
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting up...");
  display.display();
#endif

  startMillis = millis();  //initial start time
} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void energy_meter_loop()
{
  int i, j = 0;

  char *result = input_string;
  input_json.remove(0);

  /*get the current "time" (actually the number of milliseconds since the program started)*/
  currentMillis = millis();

  if (startMillis == 0) {
    startMillis = currentMillis;
    return;
  }
  if (currentMillis - startMillis < period)  //test whether the period has elapsed
  {
    return;
  }
  startMillis = currentMillis;

  /*Repeatedly fetch some values from the ATM90E32 */
  #ifdef THREE_PHASE
  float temp, freq, voltage1, voltage2, voltage3, voltageCT[NUM_INPUTS], currentCT[NUM_INPUTS],
        realPowerCT[NUM_INPUTS], vaPowerCT[NUM_INPUTS], powerFactorCT[NUM_INPUTS];
  #else
  float temp, freq, voltage1, voltage2, voltageCT[NUM_INPUTS], currentCT[NUM_INPUTS],
        realPowerCT[NUM_INPUTS], vaPowerCT[NUM_INPUTS], powerFactorCT[NUM_INPUTS];
  #endif


  unsigned short sys0 = sensor_ic1[0].GetSysStatus0();  //EMMState0
  unsigned short sys1 = sensor_ic1[0].GetSysStatus1();  //EMMState1
  unsigned short en0 = sensor_ic1[0].GetMeterStatus0(); //EMMIntState0
  unsigned short en1 = sensor_ic1[0].GetMeterStatus1(); //EMMInsState1

  unsigned short sys0_2 = sensor_ic2[0].GetSysStatus0();
  unsigned short sys1_2 = sensor_ic2[0].GetSysStatus1();
  unsigned short en0_2 = sensor_ic2[0].GetMeterStatus0();
  unsigned short en1_2 = sensor_ic2[0].GetMeterStatus1();

  Serial.println("Sys Status 1: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
  Serial.println("Meter Status 1: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
  Serial.println("Sys Status 2: S0:0x" + String(sys0_2, HEX) + " S1:0x" + String(sys1_2, HEX));
  Serial.println("Meter Status 2: E0:0x" + String(en0_2, HEX) + " E1:0x" + String(en1_2, HEX));
  delay(10);

  /* only 1 voltage channel is used on each IC */
  /* modified for 3 phase voltage output */
  #ifdef THREE_PHASE
  voltage1 = sensor_ic1[0].GetLineVoltageA();
  voltage2 = sensor_ic1[3].GetLineVoltageA();
  voltage3 = sensor_ic1[5].GetLineVoltageA();
  #else
  voltage1 = sensor_ic1[0].GetLineVoltageA();
  voltage2 = sensor_ic2[0].GetLineVoltageA();
  #endif

  freq = sensor_ic1[0].GetFrequency();
  temp = sensor_ic1[0].GetTemperature();

  Serial.println("Temp:" + String(temp) + "C");
  Serial.println("Freq:" + String(freq) + "Hz");
  
  #ifdef THREE_PHASE
   /* modified for 3 phase voltage output */
  Serial.println("V1:" + String(voltage1) + "V   V2:" + String(voltage2) + "V   V3:" + String(voltage3) + "V");
  #else
  Serial.println("V1:" + String(voltage1) + "V   V2:" + String(voltage2) + "V");
  #endif

  int length = snprintf(result, MAX_DATA_LEN, "temp:%.1f,freq:%.2f,V1:%.2f,V2:%.2f",
                        temp, freq, voltage1, voltage2);
  if (length < 0 || length >= MAX_DATA_LEN) {
    result[0] = '\0';
    return;
  }
#ifdef THREE_PHASE
  const int extra = snprintf(result + length, MAX_DATA_LEN - length, ",V3:%.2f", voltage3);
  if (extra < 0 || extra >= MAX_DATA_LEN - length) {
    result[0] = '\0';
    return;
  }
  length += extra;
#endif

  char fields[256];
  bool jsonReady = config_flags.mqtt_json;
  bool firstSensor = true;
  if (jsonReady) {
    const int count = snprintf(fields, sizeof(fields), "{\"temp\":%.1f,\"freq\":%.2f,\"sensors\":[", temp, freq);
    jsonReady = isfinite(temp) && isfinite(freq) && count >= 0 && count < sizeof(fields) &&
                input_json.concat(fields);
  }

  for (i = 0; i < NUM_BOARDS; i ++)
  {
    unsigned short sys0_1 = sensor_ic1[i].GetSysStatus0();  //EMMState0
    unsigned short sys0_2 = sensor_ic2[i].GetSysStatus0();
    if (sys0_1 == 65535 || sys0_1 == 0 || sys0_2 == 65535 || sys0_2 == 0)
    {
      /* Print error message if we can't talk to the master board */
      if (i == 0) DBUGS.println("Error: Not receiving data from the energy meter - check your connections");
      /* If no response, go to next board */
      continue;
    }

    /* get current readings from each IC */
    voltageCT[0] = sensor_ic1[i].GetLineVoltageA();
    voltageCT[1] = sensor_ic1[i].GetLineVoltageB();
    voltageCT[2] = sensor_ic1[i].GetLineVoltageC();
    voltageCT[3] = sensor_ic2[i].GetLineVoltageA();
    voltageCT[4] = sensor_ic2[i].GetLineVoltageB();
    voltageCT[5] = sensor_ic2[i].GetLineVoltageC();

    currentCT[0] = sensor_ic1[i].GetLineCurrentA();
    currentCT[1] = sensor_ic1[i].GetLineCurrentB();
    currentCT[2] = sensor_ic1[i].GetLineCurrentC();
    currentCT[3] = sensor_ic2[i].GetLineCurrentA();
    currentCT[4] = sensor_ic2[i].GetLineCurrentB();
    currentCT[5] = sensor_ic2[i].GetLineCurrentC();

    realPowerCT[0] = sensor_ic1[i].GetActivePowerA();
    realPowerCT[1] = sensor_ic1[i].GetActivePowerB();
    realPowerCT[2] = sensor_ic1[i].GetActivePowerC();
    realPowerCT[3] = sensor_ic2[i].GetActivePowerA();
    realPowerCT[4] = sensor_ic2[i].GetActivePowerB();
    realPowerCT[5] = sensor_ic2[i].GetActivePowerC();

    vaPowerCT[0] = sensor_ic1[i].GetApparentPowerA();
    vaPowerCT[1] = sensor_ic1[i].GetApparentPowerB();
    vaPowerCT[2] = sensor_ic1[i].GetApparentPowerC();
    vaPowerCT[3] = sensor_ic2[i].GetApparentPowerA();
    vaPowerCT[4] = sensor_ic2[i].GetApparentPowerB();
    vaPowerCT[5] = sensor_ic2[i].GetApparentPowerC();

    powerFactorCT[0] = sensor_ic1[i].GetPowerFactorA();
    powerFactorCT[1] = sensor_ic1[i].GetPowerFactorB();
    powerFactorCT[2] = sensor_ic1[i].GetPowerFactorC();
    powerFactorCT[3] = sensor_ic2[i].GetPowerFactorA();
    powerFactorCT[4] = sensor_ic2[i].GetPowerFactorB();
    powerFactorCT[5] = sensor_ic2[i].GetPowerFactorC();

    for (j = 0; j < NUM_INPUTS; j ++)
    {
      /* determine if negative - current registers are not signed, so this is an easy way to tell */
      if (realPowerCT[j] < 0) currentCT[j] *= -1;

      /* flip sign of power factor if current multiplier is negative */
      if (cur_mul[i*NUM_INPUTS+j] < 0) powerFactorCT[j] *= -1;

      /* scale current and power using multipliers */
      currentCT[j] *=  cur_mul[i*NUM_INPUTS+j];
      realPowerCT[j] *= pow_mul[i*NUM_INPUTS+j] * cur_mul[i*NUM_INPUTS+j];

      /* apparent power is always positive */
      vaPowerCT[j] *= fabs(pow_mul[i*NUM_INPUTS+j] * cur_mul[i*NUM_INPUTS+j]);

      Serial.println("I" + String(i) + "_CT" + String(j+1) + ": " + String(currentCT[j]) + "A");

      const int channel = i * NUM_INPUTS + j + 1;
      const int count = snprintf(result + length, MAX_DATA_LEN - length,
                                 ",CT%d:%.4f,PF%d:%.3f,W%d:%.2f,VA%d:%.2f",
                                 channel, currentCT[j], channel, powerFactorCT[j],
                                 channel, realPowerCT[j], channel, vaPowerCT[j]);
      if (count < 0 || count >= MAX_DATA_LEN - length) {
        result[0] = '\0';
        input_json.remove(0);
        return;
      }
      length += count;

      if (jsonReady) {
        const int prefix = snprintf(fields, sizeof(fields), "%s{\"ct\":%d,\"name\":\"",
                                    firstSensor ? "" : ",", channel);
        jsonReady = prefix >= 0 && prefix < sizeof(fields) && input_json.concat(fields) &&
                    input_json.concat(jsonEscape(ct_name[channel - 1]));
        const int values = snprintf(fields, sizeof(fields),
                                    "\",\"w\":%.2f,\"a\":%.4f,\"pf\":%.3f,\"va\":%.2f,\"v\":%.2f}",
                                    realPowerCT[j], currentCT[j], powerFactorCT[j], vaPowerCT[j], voltageCT[j]);
        jsonReady = jsonReady && isfinite(realPowerCT[j]) && isfinite(currentCT[j]) &&
                    isfinite(powerFactorCT[j]) && isfinite(vaPowerCT[j]) && isfinite(voltageCT[j]) &&
                    values >= 0 && values < sizeof(fields) && input_json.concat(fields);
        firstSensor = false;
      }
    }
    Serial.println("");
  }
  if (jsonReady) {
    const int count = snprintf(fields, sizeof(fields), "],\"freeram\":%lu,\"rssi\":%d}",
                               (unsigned long)ESP.getFreeHeap(), WiFi.RSSI());
    jsonReady = count >= 0 && count < sizeof(fields) && input_json.concat(fields);
  }
  if (!jsonReady) input_json.remove(0);

#ifdef ENABLE_OLED_DISPLAY
  /* Write meter data to the display */
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("V1:" + String(voltage1) + "V");
  display.println("V2:" + String(voltage2) + "V");
  display.println("CT1:" + String(currentCT1) + "A");
  display.println("CT2:" + String(currentCT2) + "A");
  display.println("CT3:" + String(currentCT3) + "A");
  display.println("CT4:" + String(currentCT4) + "A");
  display.println("CT5:" + String(currentCT5) + "A");
  display.println("CT6:" + String(currentCT6) + "A");
  display.println("Total W:" + String(totalWatts) + "W");
  /*
    display.println("Freq: " + String(freq) + "Hz");
    display.println("PF: " + String(powerFactor));
    display.println("Chip Temp: " + String(temp) + "C");
  */
  display.display();
#endif
}
