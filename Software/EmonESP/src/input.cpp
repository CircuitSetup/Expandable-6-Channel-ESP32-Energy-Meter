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
#include "input.h"
#include "emoncms.h"
#include "espal.h"

char input_string[MAX_DATA_LEN] = "";
char last_datastr[MAX_DATA_LEN] = "";

boolean input_get(JsonDocument &data)
{
  boolean gotLine = false;
  boolean gotData = false;
  char line[MAX_DATA_LEN];

  // If data from test API e.g `http://<IP-ADDRESS>/input?string=CT1:3935,CT2:325,T1:12.5,T2:16.9,T3:11.2,T4:34.7`
  if(strlen(input_string) > 0) {
    strcpy(line, input_string);
    strcpy(input_string, "");
    gotLine = true;
  }

  if(gotLine) {
    int len = strlen(line);
    if(len > 0) {
      //DEBUG.printf_P(PSTR("Got '%s'\n"), line);
      for(int i = 0; i < len; i++) {
        String name = "";

        // Get the name
        while (i < len && line[i] != ':') {
          name += line[i++];
        }

        if (i++ >= len) {
          break;
        }

        // Get the value
        String value = "";
        while (i < len && line[i] != ','){
          value += line[i++];
        }

        //DBUGVAR(name);
        //DBUGVAR(value);

        if(name.length() > 0 && value.length() > 0) {
          // IMPROVE: check that value is only a number, toDouble() will skip white space and and chars after the number
          data[name] = value.toDouble();
          gotData = true;
        }
      }
    }
  }

  // Append some system info
  if(gotData) {
    data[F("freeram")] = ESPAL.getFreeHeap();
    data[F("srssi")] = WiFi.RSSI();
    data[F("psent")] = packets_sent;
    data[F("psuccess")] = packets_success;

    last_datastr[0] = '\0';
    serializeJson(data, last_datastr);
  }

  return gotData;
}
