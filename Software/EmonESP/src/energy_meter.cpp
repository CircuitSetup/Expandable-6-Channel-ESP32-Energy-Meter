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
#include "board_profile.h"

// for ATM90E32 energy meter
#include <SPI.h>
#include <ATM90E32.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

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
   Each chip has its own CS pin (2 per board). These are selected by the
   compile-time board profile to match the host board and adapter wiring.
*/
static_assert(NUM_BOARDS == BOARD_PROFILE_NUM_BOARDS, "NUM_BOARDS must match board profile CS arrays");
const int CS1[NUM_BOARDS] = {
  BOARD_PROFILE_CS1_PINS[0],
  BOARD_PROFILE_CS1_PINS[1],
  BOARD_PROFILE_CS1_PINS[2],
  BOARD_PROFILE_CS1_PINS[3],
  BOARD_PROFILE_CS1_PINS[4],
  BOARD_PROFILE_CS1_PINS[5],
  BOARD_PROFILE_CS1_PINS[6]
};
const int CS2[NUM_BOARDS] = {
  BOARD_PROFILE_CS2_PINS[0],
  BOARD_PROFILE_CS2_PINS[1],
  BOARD_PROFILE_CS2_PINS[2],
  BOARD_PROFILE_CS2_PINS[3],
  BOARD_PROFILE_CS2_PINS[4],
  BOARD_PROFILE_CS2_PINS[5],
  BOARD_PROFILE_CS2_PINS[6]
};

/* Initialize ATM90E32 library for each IC */
ATM90E32 sensor_ic1[NUM_BOARDS]{};
ATM90E32 sensor_ic2[NUM_BOARDS]{};
static energy_meter_snapshot_t g_snapshot{};

static bool append_to_buffer(char *buffer, size_t buffer_size, size_t &offset, const char *format, ...)
{
  if (offset >= buffer_size) {
    return false;
  }

  va_list args;
  va_start(args, format);
  const int written = vsnprintf(buffer + offset, buffer_size - offset, format, args);
  va_end(args);

  if (written < 0) {
    return false;
  }

  if (static_cast<size_t>(written) >= (buffer_size - offset)) {
    offset = buffer_size - 1;
    buffer[offset] = '\0';
    return false;
  }

  offset += static_cast<size_t>(written);
  return true;
}

static bool meter_ic_is_online(ATM90E32 &sensor)
{
  const unsigned short sys0 = sensor.GetSysStatus0();
  return sys0 != 0 && sys0 != 0xFFFF;
}

static void clear_snapshot(energy_meter_snapshot_t &snapshot)
{
  memset(&snapshot, 0, sizeof(snapshot));
#ifdef THREE_PHASE
  snapshot.three_phase = true;
#else
  snapshot.three_phase = false;
#endif
}

bool energy_meter_snapshot_valid()
{
  return g_snapshot.valid;
}

const energy_meter_snapshot_t & energy_meter_get_snapshot()
{
  return g_snapshot;
}

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void energy_meter_setup() {
  int i;

  SPI.begin(
    BOARD_PROFILE_METER_SPI_SCK,
    BOARD_PROFILE_METER_SPI_MISO,
    BOARD_PROFILE_METER_SPI_MOSI,
    -1
  );

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
  int i;
  energy_meter_snapshot_t snapshot;
  clear_snapshot(snapshot);
  char result[MAX_DATA_LEN] = "";
  size_t result_offset = 0;
  bool haveAnyChannel = false;

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

  const bool primary_board_valid = meter_ic_is_online(sensor_ic1[0]) && meter_ic_is_online(sensor_ic2[0]);

  if (primary_board_valid)
  {
#ifdef THREE_PHASE
    snapshot.voltage_l1_v = sensor_ic1[0].GetLineVoltageA();
    snapshot.voltage_l2_v = sensor_ic1[3].GetLineVoltageA();
    snapshot.voltage_l3_v = sensor_ic1[5].GetLineVoltageA();
#else
    snapshot.voltage_l1_v = sensor_ic1[0].GetLineVoltageA();
    snapshot.voltage_l2_v = sensor_ic2[0].GetLineVoltageA();
#endif
    snapshot.frequency_hz = sensor_ic1[0].GetFrequency();
    snapshot.temperature_c = sensor_ic1[0].GetTemperature();
  }

  Serial.println("Temp:" + String(snapshot.temperature_c) + "C");
  Serial.println("Freq:" + String(snapshot.frequency_hz) + "Hz");
#ifdef THREE_PHASE
  Serial.println("V1:" + String(snapshot.voltage_l1_v) + "V   V2:" + String(snapshot.voltage_l2_v) + "V   V3:" + String(snapshot.voltage_l3_v) + "V");
#else
  Serial.println("V1:" + String(snapshot.voltage_l1_v) + "V   V2:" + String(snapshot.voltage_l2_v) + "V");
#endif

  if (primary_board_valid) {
    append_to_buffer(result, sizeof(result), result_offset, "temp:%.1f", snapshot.temperature_c);
    append_to_buffer(result, sizeof(result), result_offset, ",freq:%.2f", snapshot.frequency_hz);
    append_to_buffer(result, sizeof(result), result_offset, ",V1:%.2f", snapshot.voltage_l1_v);
    append_to_buffer(result, sizeof(result), result_offset, ",V2:%.2f", snapshot.voltage_l2_v);
#ifdef THREE_PHASE
    append_to_buffer(result, sizeof(result), result_offset, ",V3:%.2f", snapshot.voltage_l3_v);
#endif
  }

  for (i = 0; i < NUM_BOARDS; i ++)
  {
    if (!meter_ic_is_online(sensor_ic1[i]) || !meter_ic_is_online(sensor_ic2[i]))
    {
      /* Print error message if we can't talk to the master board */
      if (i == 0) DBUGS.println("Error: Not receiving data from the energy meter - check your connections");
      /* If no response, go to next board */
      continue;
    }

    const float currentCT[NUM_INPUTS] = {
      static_cast<float>(sensor_ic1[i].GetLineCurrentA()),
      static_cast<float>(sensor_ic1[i].GetLineCurrentB()),
      static_cast<float>(sensor_ic1[i].GetLineCurrentC()),
      static_cast<float>(sensor_ic2[i].GetLineCurrentA()),
      static_cast<float>(sensor_ic2[i].GetLineCurrentB()),
      static_cast<float>(sensor_ic2[i].GetLineCurrentC())
    };
    const float realPowerCT[NUM_INPUTS] = {
      static_cast<float>(sensor_ic1[i].GetActivePowerA()),
      static_cast<float>(sensor_ic1[i].GetActivePowerB()),
      static_cast<float>(sensor_ic1[i].GetActivePowerC()),
      static_cast<float>(sensor_ic2[i].GetActivePowerA()),
      static_cast<float>(sensor_ic2[i].GetActivePowerB()),
      static_cast<float>(sensor_ic2[i].GetActivePowerC())
    };
    const float reactivePowerCT[NUM_INPUTS] = {
      static_cast<float>(sensor_ic1[i].GetReactivePowerA()),
      static_cast<float>(sensor_ic1[i].GetReactivePowerB()),
      static_cast<float>(sensor_ic1[i].GetReactivePowerC()),
      static_cast<float>(sensor_ic2[i].GetReactivePowerA()),
      static_cast<float>(sensor_ic2[i].GetReactivePowerB()),
      static_cast<float>(sensor_ic2[i].GetReactivePowerC())
    };
    const float apparentPowerCT[NUM_INPUTS] = {
      static_cast<float>(sensor_ic1[i].GetApparentPowerA()),
      static_cast<float>(sensor_ic1[i].GetApparentPowerB()),
      static_cast<float>(sensor_ic1[i].GetApparentPowerC()),
      static_cast<float>(sensor_ic2[i].GetApparentPowerA()),
      static_cast<float>(sensor_ic2[i].GetApparentPowerB()),
      static_cast<float>(sensor_ic2[i].GetApparentPowerC())
    };
    const float powerFactorCT[NUM_INPUTS] = {
      static_cast<float>(sensor_ic1[i].GetPowerFactorA()),
      static_cast<float>(sensor_ic1[i].GetPowerFactorB()),
      static_cast<float>(sensor_ic1[i].GetPowerFactorC()),
      static_cast<float>(sensor_ic2[i].GetPowerFactorA()),
      static_cast<float>(sensor_ic2[i].GetPowerFactorB()),
      static_cast<float>(sensor_ic2[i].GetPowerFactorC())
    };
    const float phaseAngleCT[NUM_INPUTS] = {
      static_cast<float>(sensor_ic1[i].GetPhaseA()),
      static_cast<float>(sensor_ic1[i].GetPhaseB()),
      static_cast<float>(sensor_ic1[i].GetPhaseC()),
      static_cast<float>(sensor_ic2[i].GetPhaseA()),
      static_cast<float>(sensor_ic2[i].GetPhaseB()),
      static_cast<float>(sensor_ic2[i].GetPhaseC())
    };

    for (int j = 0; j < NUM_INPUTS; j ++)
    {
      const int channelIndex = i*NUM_INPUTS+j;
      const float scale = pow_mul[channelIndex] * cur_mul[channelIndex];
      /* determine if negative - current registers are not signed, so this is an easy way to tell */
      float current = currentCT[j];
      if (realPowerCT[j] < 0) {
        current *= -1;
      }

      /* flip sign of power factor if current multiplier is negative */
      float powerFactor = powerFactorCT[j];
      float phaseAngle = phaseAngleCT[j];
      if (cur_mul[channelIndex] < 0) {
        powerFactor *= -1;
        phaseAngle *= -1;
      }

      /* scale current and power using multipliers */
      current *= cur_mul[channelIndex];
      const float realPower = realPowerCT[j] * scale;
      const float reactivePower = reactivePowerCT[j] * scale;

      /* apparent power is always positive */
      const float apparentPower = apparentPowerCT[j] * fabsf(scale);

      energy_channel_snapshot_t &channel = snapshot.channels[channelIndex];
      channel.valid = true;
      channel.current_a = current;
      channel.power_w = realPower;
      channel.power_factor = powerFactor;
      channel.apparent_power_va = apparentPower;
      channel.reactive_power_var = reactivePower;
      channel.phase_angle_deg = phaseAngle;

      snapshot.total_power_w += realPower;
      snapshot.total_apparent_power_va += apparentPower;
      snapshot.total_reactive_power_var += reactivePower;
      haveAnyChannel = true;

      Serial.println("I" + String(i) + "_CT" + String(j+1) + ": " + String(current) + "A");

      const char *prefix = result_offset ? "," : "";
      append_to_buffer(result, sizeof(result), result_offset, "%sCT%d:%.4f", prefix, channelIndex + 1, current);
      append_to_buffer(result, sizeof(result), result_offset, ",PF%d:%.3f", channelIndex + 1, powerFactor);
      append_to_buffer(result, sizeof(result), result_offset, ",W%d:%.2f", channelIndex + 1, realPower);
      append_to_buffer(result, sizeof(result), result_offset, ",VA%d:%.2f", channelIndex + 1, apparentPower);
    }
    Serial.println("");
  }

  snapshot.valid = haveAnyChannel || primary_board_valid;
  if (fabsf(snapshot.total_apparent_power_va) > 0.0001f) {
    snapshot.total_power_factor = snapshot.total_power_w / snapshot.total_apparent_power_va;
  }
  g_snapshot = snapshot;

  if (result[0] != '\0') {
    input_set(result, true);
  }

#ifdef ENABLE_OLED_DISPLAY
  /* Write meter data to the display */
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("V1:" + String(snapshot.voltage_l1_v) + "V");
  display.println("V2:" + String(snapshot.voltage_l2_v) + "V");
  display.println("CT1:" + String(snapshot.channels[0].current_a) + "A");
  display.println("CT2:" + String(snapshot.channels[1].current_a) + "A");
  display.println("CT3:" + String(snapshot.channels[2].current_a) + "A");
  display.println("CT4:" + String(snapshot.channels[3].current_a) + "A");
  display.println("CT5:" + String(snapshot.channels[4].current_a) + "A");
  display.println("CT6:" + String(snapshot.channels[5].current_a) + "A");
  display.println("Total W:" + String(snapshot.total_power_w) + "W");
  /*
    display.println("Freq: " + String(freq) + "Hz");
    display.println("PF: " + String(powerFactor));
    display.println("Chip Temp: " + String(temp) + "C");
  */
  display.display();
#endif
}
