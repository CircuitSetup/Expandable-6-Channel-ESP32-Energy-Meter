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
#include "input.h"

char input_string[MAX_DATA_LEN] = "";
char last_datastr[MAX_DATA_LEN] = "";

boolean input_get(char * data)
{
  boolean gotData = false;

  // If data from test API e.g `http://<IP-ADDRESS>/input?string=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7`
  if (strlen(input_string) > 0) {
    strcpy(data, input_string);
    strcpy(input_string, "");
    gotData = true;
  }
#ifdef USE_SERIAL_INPUT
  // If data received on serial
  else if (Serial.available()) {
    // Could check for string integrity here
    data = Serial.readStringUntil('\n');
    gotData = true;
  }
#endif

  if (gotData)
  {
    // Get rid of any whitespace, newlines etc
    //data.trim();

    if (strlen(data) > 0) {
//      DBUGS.printf("Got '%s'\n", data.c_str());
      strcpy(last_datastr, data);
    } else {
      gotData = false;
    }
  }

  return gotData;
}
