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

/***** CALIBRATION SETTINGS *****/
/* These values are edited in the web interface or energy_meter.h */
/* Values in the web interface take priority */
unsigned short VoltageGain1 = VOLTAGE_GAIN;
unsigned short VoltageGain2 = VOLTAGE_GAIN2;
unsigned short CurrentGainCT1 = CURRENT_GAIN_CT1;
unsigned short CurrentGainCT2 = CURRENT_GAIN_CT2;
unsigned short CurrentGainCT3 = CURRENT_GAIN_CT3;
unsigned short CurrentGainCT4 = CURRENT_GAIN_CT4;
unsigned short CurrentGainCT5 = CURRENT_GAIN_CT5;
unsigned short CurrentGainCT6 = CURRENT_GAIN_CT6;
unsigned short lineFreq = LINE_FREQ;
unsigned short PGAGain = PGA_GAIN;

unsigned long startMillis;
unsigned long currentMillis;
const int period = 1000; //time interval in ms to send data

/****** Chip Select Pins ******/
/*
   Each chip has its own CS pin (2 per board). The main board must be pins 5 and 4.
*/
const int CS1_main = 5;
const int CS2_main = 4;

char result[300];
char measurement[16];

/* Initialize ATM90E32 library for each IC */
ATM90E32 main1{};
ATM90E32 main2{};

/*  Add-on boards have a jumper selection. Jumper bank "CS" are the
    left 3 current channels, and CS2 is the right 3.
  
    When using multiple add-on board - To make it simple, use the default pin assignments,
    unless you have a specific reason to not use a certain CS pin
    by default the jumper pins increment up from left to right on the jumper banks
    ONLY JUMPER ONE PIN PER BANK
*/
#ifdef ADDON_BOARDS
#if NUM_OF_ADDON_BOARDS >= 1 
  const int CS1_addon1 = 0;
  const int CS2_addon1 = 16;

  ATM90E32 AddOn1_1{};
  ATM90E32 AddOn1_2{};
#endif

#if NUM_OF_ADDON_BOARDS >= 2
  const int CS1_addon2 = 2;
  const int CS2_addon2 = 17;

  ATM90E32 AddOn2_1{};
  ATM90E32 AddOn2_2{};
#endif

#if NUM_OF_ADDON_BOARDS >= 3
  const int CS1_addon3 = 12;
  const int CS2_addon3 = 21;

  ATM90E32 AddOn3_1{};
  ATM90E32 AddOn3_2{};
#endif

#if NUM_OF_ADDON_BOARDS >= 4
  const int CS1_addon4 = 13;
  const int CS2_addon4 = 22;

  ATM90E32 AddOn4_1{};
  ATM90E32 AddOn4_2{};
#endif

#if NUM_OF_ADDON_BOARDS >= 5
  const int CS1_addon5 = 14;
  const int CS2_addon5 = 25;

  ATM90E32 AddOn5_1{};
  ATM90E32 AddOn5_2{};
  
#endif
#if NUM_OF_ADDON_BOARDS >= 6
  const int CS1_addon6 = 15;
  const int CS2_addon6 = 26;

  ATM90E32 AddOn6_1{};
  ATM90E32 AddOn6_2{};
#endif
#endif

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void energy_meter_setup() {

  /*Get values from web interface and assign them if populated*/
  if (voltage_cal.toInt() > 0) VoltageGain1 = voltage_cal.toInt();
  if (voltage2_cal.toInt() > 0) VoltageGain2 = voltage2_cal.toInt();
  if (ct1_cal.toInt() > 0) CurrentGainCT1 = ct1_cal.toInt();
  if (ct2_cal.toInt() > 0) CurrentGainCT2 = ct2_cal.toInt();
  if (ct3_cal.toInt() > 0) CurrentGainCT3 = ct3_cal.toInt();
  if (ct4_cal.toInt() > 0) CurrentGainCT4 = ct4_cal.toInt();
  if (ct5_cal.toInt() > 0) CurrentGainCT5 = ct5_cal.toInt();
  if (ct6_cal.toInt() > 0) CurrentGainCT6 = ct6_cal.toInt();
  if (freq_cal.toInt() > 0) lineFreq = freq_cal.toInt();
  if (gain_cal.toInt() > 0) PGAGain = gain_cal.toInt();

  /*Initialise the ATM90E32 & Pass CS pin and calibrations to its library */
  Serial.println("Start ATM90E32");
  main1.begin(CS1_main, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
  main2.begin(CS2_main, lineFreq, PGAGain, VoltageGain2, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  delay(500);

  #ifdef ADDON_BOARDS
  #if NUM_OF_ADDON_BOARDS >= 1
    AddOn1_1.begin(CS1_addon1, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    AddOn1_2.begin(CS2_addon1, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  #endif
  #if NUM_OF_ADDON_BOARDS >= 2
    AddOn2_1.begin(CS1_addon2, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    AddOn2_2.begin(CS2_addon2, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  #endif
  #if NUM_OF_ADDON_BOARDS >= 3
    AddOn3_1.begin(CS1_addon3, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    AddOn3_2.begin(CS2_addon3, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  #endif
  #if NUM_OF_ADDON_BOARDS >= 4
    AddOn4_1.begin(CS1_addon4, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    AddOn4_2.begin(CS2_addon4, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  #endif
  #if NUM_OF_ADDON_BOARDS >= 5
    AddOn5_1.begin(CS1_addon5, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    AddOn5_2.begin(CS2_addon5, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  #endif
  #if NUM_OF_ADDON_BOARDS >= 6
    AddOn6_1.begin(CS1_addon6, lineFreq, PGAGain, VoltageGain1, CurrentGainCT1, CurrentGainCT2, CurrentGainCT3);
    AddOn6_2.begin(CS2_addon6, lineFreq, PGAGain, VoltageGain1, CurrentGainCT4, CurrentGainCT5, CurrentGainCT6);
  #endif
  delay(500);
#endif

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
    float voltage1, voltage2, currentCT1, currentCT2, currentCT3, currentCT4, currentCT5, currentCT6, current1, current2, totalCurrent, temp, freq, totalWatts;
#ifdef JP9_JP11_SET
#ifdef EXPORT_METERING_VALS
    float powerFactor1, powerFactor2, avgPF, fundPower, harPower, reactPower, appPower;
#endif
#endif

#ifdef ADDON_BOARDS
#if NUM_OF_ADDON_BOARDS >= 1
    float AddOn1_V1, AddOn1_V2, AddOn1_CT1, AddOn1_CT2, AddOn1_CT3, AddOn1_CT4, AddOn1_CT5, AddOn1_CT6, AddOn1_current1, AddOn1_current2, AddOn1_totalCurrent, AddOn1_totalWatts;
#endif
#if NUM_OF_ADDON_BOARDS >= 2
    float AddOn2_V1, AddOn2_V2, AddOn2_CT1, AddOn2_CT2, AddOn2_CT3, AddOn2_CT4, AddOn2_CT5, AddOn2_CT6, AddOn2_current1, AddOn2_current2, AddOn2_totalCurrent, AddOn2_totalWatts;
#endif
#if NUM_OF_ADDON_BOARDS >= 3
    float AddOn3_V1, AddOn3_V2, AddOn3_CT1, AddOn3_CT2, AddOn3_CT3, AddOn3_CT4, AddOn3_CT5, AddOn3_CT6, AddOn3_current1, AddOn3_current2, AddOn3_totalCurrent, AddOn3_totalWatts;
#endif
#if NUM_OF_ADDON_BOARDS >= 4
    float AddOn4_V1, AddOn4_V2, AddOn4_CT1, AddOn4_CT2, AddOn4_CT3, AddOn4_CT4, AddOn4_CT5, AddOn4_CT6, AddOn4_current1, AddOn4_current2, AddOn4_totalCurrent, AddOn4_totalWatts;
#endif
#if NUM_OF_ADDON_BOARDS >= 5
    float AddOn5_V1, AddOn5_V2, AddOn5_CT1, AddOn5_CT2, AddOn5_CT3, AddOn5_CT4, AddOn5_CT5, AddOn5_CT6, AddOn5_current1, AddOn5_current2, AddOn5_totalCurrent, AddOn5_totalWatts;
#endif
#if NUM_OF_ADDON_BOARDS >= 6
    float AddOn6_V1, AddOn6_V2, AddOn6_CT1, AddOn6_CT2, AddOn6_CT3, AddOn6_CT4, AddOn6_CT5, AddOn6_CT6, AddOn6_current1, AddOn6_current2, AddOn6_totalCurrent, AddOn6_totalWatts;
#endif
#endif


    unsigned short sys0 = main1.GetSysStatus0();  //EMMState0
    unsigned short sys1 = main1.GetSysStatus1();  //EMMState1
    unsigned short en0 = main1.GetMeterStatus0(); //EMMIntState0
    unsigned short en1 = main1.GetMeterStatus1(); //EMMInsState1

    unsigned short sys0_2 = main2.GetSysStatus0();
    unsigned short sys1_2 = main2.GetSysStatus1();
    unsigned short en0_2 = main2.GetMeterStatus0();
    unsigned short en1_2 = main2.GetMeterStatus1();

    // Serial.println("Sys Status 1: S0:0x" + String(sys0, HEX) + " S1:0x" + String(sys1, HEX));
    // Serial.println("Meter Status 1: E0:0x" + String(en0, HEX) + " E1:0x" + String(en1, HEX));
    // Serial.println("Sys Status 2: S0:0x" + String(sys0_2, HEX) + " S1:0x" + String(sys1_2, HEX));
    // Serial.println("Meter Status 2: E0:0x" + String(en0_2, HEX) + " E1:0x" + String(en1_2, HEX));
    delay(10);
    
    /*if true the MCU is not getting data from the energy meter */
    if (sys0 == 65535 || sys0 == 0);// DBUGS.println("Error: Not receiving data from the energy meter - check your connections");

    /* only 1 voltage channel is used on each IC */
    voltage1 = main1.GetLineVoltageA();
    voltage2 = main2.GetLineVoltageA();

    /* get current readings from each IC and add them up */
    currentCT1 = main1.GetLineCurrentA();
    currentCT2 = main1.GetLineCurrentB();
    currentCT3 = main1.GetLineCurrentC();
    currentCT4 = main2.GetLineCurrentA();
    currentCT5 = main2.GetLineCurrentB();
    currentCT6 = main2.GetLineCurrentC();

    /* determine if negative - current registers are not signed, so this is an easy way to tell */
    if (main1.GetActivePowerA() < 0) currentCT1 *= -1;
    if (main1.GetActivePowerB() < 0) currentCT2 *= -1;
    if (main1.GetActivePowerC() < 0) currentCT3 *= -1;
    if (main2.GetActivePowerA() < 0) currentCT4 *= -1;
    if (main2.GetActivePowerB() < 0) currentCT5 *= -1;
    if (main2.GetActivePowerC() < 0) currentCT6 *= -1;
    
    current1 = currentCT1 + currentCT2 + currentCT3;
    current2 = currentCT4 + currentCT5 + currentCT6;
    totalCurrent = current1 + current2;

    #ifdef JP9_JP11_SET
    #ifdef EXPORT_METERING_VALS
    powerFactor1 = main1.GetTotalPowerFactor();
    powerFactor2 = main2.GetTotalPowerFactor();
    avgPF = (powerFactor1 + powerFactor2)/2;
    
    fundPower = main1.GetTotalActiveFundPower() + main2.GetTotalActiveFundPower();
    harPower = main1.GetTotalActiveHarPower() + main2.GetTotalActiveHarPower();
    reactPower = main1.GetTotalReactivePower() + main2.GetTotalReactivePower();
    appPower = main1.GetTotalApparentPower() + main2.GetTotalApparentPower();
    #endif
    #endif

    temp = main1.GetTemperature();
    freq = main1.GetFrequency();
    
    #ifdef JP9_JP11_SET
    totalWatts = main1.GetTotalActivePower() + main2.GetTotalActivePower();
    #else
    totalWatts = (voltage1 * current1) + (voltage2 * current2);
    #endif

    // Serial.println("V1:" + String(voltage1) + "V   V2:" + String(voltage2) + "V");
    // Serial.println("I1:" + String(currentCT1) + "A   I6:" + String(currentCT6) + "A");
    // Serial.println("I2:" + String(currentCT2) + "A   I5:" + String(currentCT5) + "A");
    // Serial.println("I3:" + String(currentCT3) + "A   I4:" + String(currentCT4) + "A");
    #ifdef ADDON_BOARDS
    Serial.println("Total Current:" + String(totalCurrent) + "A");
    Serial.println("Total Power:" + String(totalWatts) + "W");
    #endif
    
    #ifdef JP9_JP11_SET
    #ifdef EXPORT_METERING_VALS
    // Serial.println("Power Factor:" + String(avgPF));
    // Serial.println("Fundamental Power:" + String(fundPower) + "W");
    // Serial.println("Harmonic Power:" + String(harPower) + "W");
    // Serial.println("Reactive Power:" + String(reactPower) + "var");
    // Serial.println("Apparent Power:" + String(appPower) + "VA");
    #endif
    #endif
    Serial.println("");


    /* get current from add-on boards as needed */
#ifdef ADDON_BOARDS
    #if NUM_OF_ADDON_BOARDS >= 1
      AddOn1_V1 = AddOn1_1.GetLineVoltageA();
      AddOn1_V2 = AddOn1_2.GetLineVoltageA();
      AddOn1_CT1 = AddOn1_1.GetLineCurrentA();
      AddOn1_CT2 = AddOn1_1.GetLineCurrentB();
      AddOn1_CT3 = AddOn1_1.GetLineCurrentC();
      AddOn1_CT4 = AddOn1_2.GetLineCurrentA();
      AddOn1_CT5 = AddOn1_2.GetLineCurrentB();
      AddOn1_CT6 = AddOn1_2.GetLineCurrentC();
      AddOn1_current1 = AddOn1_CT1 + AddOn1_CT2 + AddOn1_CT3;
      AddOn1_current2 = AddOn1_CT4 + AddOn1_CT5 + AddOn1_CT6;
      AddOn1_totalCurrent = AddOn1_current1 + AddOn1_current2;
      totalCurrent += AddOn1_totalCurrent;
      
      #ifdef JP9_JP11_SET
      AddOn1_totalWatts = AddOn1_1.GetTotalActivePower() + AddOn1_2.GetTotalActivePower();
      #else
      AddOn1_totalWatts = (AddOn1_V1 * AddOn1_current1) + (AddOn1_V2 * AddOn1_current2);
      #endif
      totalWatts += AddOn1_totalWatts;
      
      Serial.println("V1-1:" + String(AddOn1_V1) + "V   V1-2:" + String(AddOn1_V2) + "V");
      Serial.println("I1_1:" + String(AddOn1_CT1) + "A   I1_6:" + String(AddOn1_CT6) + "A");
      Serial.println("I1_2:" + String(AddOn1_CT2) + "A   I1_5:" + String(AddOn1_CT5) + "A");
      Serial.println("I1_3:" + String(AddOn1_CT3) + "A   I1_4:" + String(AddOn1_CT4) + "A");
      Serial.println("AO1 Current:" + String(AddOn1_totalCurrent) + "A");
      Serial.println("AO1 Power:" + String(AddOn1_totalWatts) + "W");
      Serial.println("");
    #endif
      
    #if NUM_OF_ADDON_BOARDS >= 2
      AddOn2_V1 = AddOn2_1.GetLineVoltageA();
      AddOn2_V2 = AddOn2_2.GetLineVoltageA();
      AddOn2_CT1 = AddOn2_1.GetLineCurrentA();
      AddOn2_CT2 = AddOn2_1.GetLineCurrentB();
      AddOn2_CT3 = AddOn2_1.GetLineCurrentC();
      AddOn2_CT4 = AddOn2_2.GetLineCurrentA();
      AddOn2_CT5 = AddOn2_2.GetLineCurrentB();
      AddOn2_CT6 = AddOn2_2.GetLineCurrentC();
      AddOn2_current1 = AddOn2_CT1 + AddOn2_CT2 + AddOn2_CT3;
      AddOn2_current2 = AddOn2_CT4 + AddOn2_CT5 + AddOn2_CT6;
      AddOn2_totalCurrent = AddOn2_current1 + AddOn2_current2;
      totalCurrent += AddOn2_totalCurrent;
      
      #ifdef JP9_JP11_SET
      AddOn2_totalWatts += AddOn2_1.GetTotalActivePower() + AddOn2_2.GetTotalActivePower();
      #else
      AddOn2_totalWatts += (AddOn2_V1 * AddOn2_current1) + (AddOn2_V2 * AddOn2_current2);
      #endif
      totalWatts += AddOn2_totalWatts;

      Serial.println("V2-1:" + String(AddOn2_V1) + "V   V2-2:" + String(AddOn2_V2) + "V");
      Serial.println("I2_1:" + String(AddOn2_CT1) + "A   I2_6:" + String(AddOn2_CT6) + "A");
      Serial.println("I2_2:" + String(AddOn2_CT2) + "A   I2_5:" + String(AddOn2_CT5) + "A");
      Serial.println("I2_3:" + String(AddOn2_CT3) + "A   I2_4:" + String(AddOn2_CT4) + "A");
      Serial.println("AO2 Current:" + String(AddOn2_totalCurrent) + "A");
      Serial.println("AO2 Power:" + String(AddOn2_totalWatts) + "W");
      Serial.println("");
    #endif
    #if NUM_OF_ADDON_BOARDS >= 3
      AddOn3_V1 = AddOn3_1.GetLineVoltageA();
      AddOn3_V2 = AddOn3_2.GetLineVoltageA();
      AddOn3_CT1 = AddOn3_1.GetLineCurrentA();
      AddOn3_CT2 = AddOn3_1.GetLineCurrentB();
      AddOn3_CT3 = AddOn3_1.GetLineCurrentC();
      AddOn3_CT4 = AddOn3_2.GetLineCurrentA();
      AddOn3_CT5 = AddOn3_2.GetLineCurrentB();
      AddOn3_CT6 = AddOn3_2.GetLineCurrentC();
      AddOn3_current1 = AddOn3_CT1 + AddOn3_CT2 + AddOn3_CT3;
      AddOn3_current2 = AddOn3_CT4 + AddOn3_CT5 + AddOn3_CT6;
      AddOn3_totalCurrent = AddOn3_current1 + AddOn3_current2;
      totalCurrent += AddOn3_totalCurrent;
      
      #ifdef JP9_JP11_SET
      AddOn3_totalWatts += AddOn3_1.GetTotalActivePower() + AddOn3_2.GetTotalActivePower();
      #else
      AddOn3_totalWatts += (AddOn3_V1 * AddOn3_current1) + (AddOn3_V2 * AddOn3_current2);
      #endif
      totalWatts += AddOn3_totalWatts;

      Serial.println("V3-1:" + String(AddOn3_V1) + "V   V3-2:" + String(AddOn3_V2) + "V");
      Serial.println("I3_1:" + String(AddOn3_CT1) + "A   I3_6:" + String(AddOn3_CT6) + "A");
      Serial.println("I3_2:" + String(AddOn3_CT2) + "A   I3_5:" + String(AddOn3_CT5) + "A");
      Serial.println("I3_3:" + String(AddOn3_CT3) + "A   I3_4:" + String(AddOn3_CT4) + "A");
      Serial.println("AO3 Current:" + String(AddOn3_totalCurrent) + "A");
      Serial.println("AO3 Power:" + String(AddOn3_totalWatts) + "W");
      Serial.println("");
    #endif
    #if NUM_OF_ADDON_BOARDS >= 4
      AddOn4_V1 = AddOn4_1.GetLineVoltageA();
      AddOn4_V2 = AddOn4_2.GetLineVoltageA();
      AddOn4_CT1 = AddOn4_1.GetLineCurrentA();
      AddOn4_CT2 = AddOn4_1.GetLineCurrentB();
      AddOn4_CT3 = AddOn4_1.GetLineCurrentC();
      AddOn4_CT4 = AddOn4_2.GetLineCurrentA();
      AddOn4_CT5 = AddOn4_2.GetLineCurrentB();
      AddOn4_CT6 = AddOn4_2.GetLineCurrentC();
      AddOn4_current1 = AddOn4_CT1 + AddOn4_CT2 + AddOn4_CT3;
      AddOn4_current2 = AddOn4_CT4 + AddOn4_CT5 + AddOn4_CT6;
      AddOn4_totalCurrent = AddOn4_current1 + AddOn4_current2;
      totalCurrent += AddOn4_totalCurrent;
      
      #ifdef JP9_JP11_SET
      AddOn4_totalWatts += AddOn4_1.GetTotalActivePower() + AddOn4_2.GetTotalActivePower();
      #else
      AddOn4_totalWatts += (AddOn4_V1 * AddOn3_current1) + (AddOn4_V2 * AddOn3_current2);
      #endif
      totalWatts += AddOn4_totalWatts;

      Serial.println("V4-1:" + String(AddOn4_V1) + "V   V4-2:" + String(AddOn4_V2) + "V");
      Serial.println("I4_1:" + String(AddOn4_CT1) + "A   I4_6:" + String(AddOn4_CT6) + "A");
      Serial.println("I4_2:" + String(AddOn4_CT2) + "A   I4_5:" + String(AddOn4_CT5) + "A");
      Serial.println("I4_3:" + String(AddOn4_CT3) + "A   I4_4:" + String(AddOn4_CT4) + "A");
      Serial.println("AO4 Current:" + String(AddOn4_totalCurrent) + "A");
      Serial.println("AO4 Power:" + String(AddOn4_totalWatts) + "W");
      Serial.println("");
    #endif
    #if NUM_OF_ADDON_BOARDS >= 5
      AddOn5_V1 = AddOn5_1.GetLineVoltageA();
      AddOn5_V2 = AddOn5_2.GetLineVoltageA();
      AddOn5_CT1 = AddOn5_1.GetLineCurrentA();
      AddOn5_CT2 = AddOn5_1.GetLineCurrentB();
      AddOn5_CT3 = AddOn5_1.GetLineCurrentC();
      AddOn5_CT4 = AddOn5_2.GetLineCurrentA();
      AddOn5_CT5 = AddOn5_2.GetLineCurrentB();
      AddOn5_CT6 = AddOn5_2.GetLineCurrentC();
      AddOn5_current1 = AddOn5_CT1 + AddOn5_CT2 + AddOn5_CT3;
      AddOn5_current2 = AddOn5_CT4 + AddOn5_CT5 + AddOn5_CT6;
      AddOn5_totalCurrent = AddOn5_current1 + AddOn5_current2;
      totalCurrent += AddOn5_totalCurrent;
      
      #ifdef JP9_JP11_SET
      AddOn5_totalWatts += AddOn5_1.GetTotalActivePower() + AddOn5_2.GetTotalActivePower();
      #else
      AddOn5_totalWatts += (AddOn5_V1 * AddOn5_current1) + (AddOn5_V2 * AddOn5_current2);
      #endif
      totalWatts += AddOn5_totalWatts;

      Serial.println("V5-1:" + String(AddOn5_V1) + "V   V5-2:" + String(AddOn5_V2) + "V");
      Serial.println("I5_1:" + String(AddOn5_CT1) + "A   I5_6:" + String(AddOn5_CT6) + "A");
      Serial.println("I5_2:" + String(AddOn5_CT2) + "A   I5_5:" + String(AddOn5_CT5) + "A");
      Serial.println("I5_3:" + String(AddOn5_CT3) + "A   I5_4:" + String(AddOn5_CT4) + "A");
      Serial.println("AO5 Current:" + String(AddOn5_totalCurrent) + "A");
      Serial.println("AO5 Power:" + String(AddOn5_totalWatts) + "W");
      Serial.println("");
    #endif
    #if NUM_OF_ADDON_BOARDS >= 6
      AddOn6_V1 = AddOn6_1.GetLineVoltageA();
      AddOn6_V2 = AddOn6_2.GetLineVoltageA();
      AddOn6_CT1 = AddOn6_1.GetLineCurrentA();
      AddOn6_CT2 = AddOn6_1.GetLineCurrentB();
      AddOn6_CT3 = AddOn6_1.GetLineCurrentC();
      AddOn6_CT4 = AddOn6_2.GetLineCurrentA();
      AddOn6_CT5 = AddOn6_2.GetLineCurrentB();
      AddOn6_CT6 = AddOn6_2.GetLineCurrentC();
      AddOn6_current1 = AddOn6_CT1 + AddOn6_CT2 + AddOn6_CT3;
      AddOn6_current2 = AddOn6_CT4 + AddOn6_CT5 + AddOn6_CT6;
      AddOn6_totalCurrent = AddOn6_current1 + AddOn6_current2;
      totalCurrent += AddOn6_totalCurrent;
      
      #ifdef JP9_JP11_SET
      AddOn6_totalWatts += AddOn6_1.GetTotalActivePower() + AddOn6_2.GetTotalActivePower();
      #else
      AddOn6_totalWatts += (AddOn6_V1 * AddOn6_current1) + (AddOn6_V2 * AddOn6_current2);
      #endif
      totalWatts += AddOn6_totalWatts;

      Serial.println("V6-1:" + String(AddOn6_V1) + "V   V6-2:" + String(AddOn6_V2) + "V");
      Serial.println("I6_1:" + String(AddOn6_CT1) + "A   I6_6:" + String(AddOn6_CT6) + "A");
      Serial.println("I6_2:" + String(AddOn6_CT2) + "A   I6_5:" + String(AddOn6_CT5) + "A");
      Serial.println("I6_3:" + String(AddOn6_CT3) + "A   I6_4:" + String(AddOn6_CT4) + "A");
      Serial.println("AO6 Current:" + String(AddOn6_totalCurrent) + "A");
      Serial.println("AO6 Power:" + String(AddOn6_totalWatts) + "W");
      Serial.println("");
    #endif
#endif
    // Serial.println("Total Current:" + String(totalCurrent) + "A");
    // Serial.println("Total Power:" + String(totalWatts)+ "W");
    // Serial.println("Temp:" + String(temp) + "C");
    // Serial.println("Freq:" + String(freq) + "Hz");


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

  /* Default values are passed to EmonCMS - these can be changed out for anything
     in the ATM90E32 library
  */
  strcpy(result, "");
  
  strcat(result, "V1:");
  dtostrf(voltage1, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",V2:");
  dtostrf(voltage2, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",CT1:");
  dtostrf(currentCT1, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",CT2:");
  dtostrf(currentCT2, 2, 4, measurement);
  strcat(result, measurement);
  
  strcat(result, ",CT3:");
  dtostrf(currentCT3, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",CT4:");
  dtostrf(currentCT4, 2, 4, measurement);
  strcat(result, measurement);
  
  strcat(result, ",CT5:");
  dtostrf(currentCT5, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",CT6:");
  dtostrf(currentCT6, 2, 4, measurement);
  strcat(result, measurement);

#ifdef EXPORT_METERING_VALS
  strcat(result, ",PF:");
  dtostrf(avgPF, 2, 2, measurement);
  strcat(result, measurement);
  
  strcat(result, ",FundPow:");
  dtostrf(fundPower, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",HarPow:");
  dtostrf(harPower, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",ReactPow:");
  dtostrf(reactPower, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",AppPow:");
  dtostrf(appPower, 2, 4, measurement);
  strcat(result, measurement);
#endif

  strcat(result, ",totI:");
  dtostrf(totalCurrent, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",W:");
  dtostrf(totalWatts, 2, 4, measurement);
  strcat(result, measurement);

  strcat(result, ",temp:");
  dtostrf(temp, 2, 2, measurement);
  strcat(result, measurement);

  strcat(result, ",freq:");
  dtostrf(freq, 2, 2, measurement);
  strcat(result, measurement);

  //DBUGS.println(result);

  input_string = result;

}
