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

#ifndef _EMONESP_INPUT_H
#define _EMONESP_INPUT_H

#include <Arduino.h>

// -------------------------------------------------------------------
// Support for reading input
// -------------------------------------------------------------------

extern char last_datastr[MAX_DATA_LEN];
extern char input_string[MAX_DATA_LEN];

// -------------------------------------------------------------------
// Set the latest input payload and note whether it came from the
// onboard ATM90E32 polling path or an external/manual source.
// -------------------------------------------------------------------
extern void input_set(const char * data, bool from_energy_meter);

// -------------------------------------------------------------------
// Return true when the last successful input_get() returned data that
// originated from the ATM90E32 polling path.
// -------------------------------------------------------------------
extern bool input_last_data_from_energy_meter();

// -------------------------------------------------------------------
// Read input sent via the web_server or serial.
//
// data: if true is returned data will be updated with the new line of
//       input
// -------------------------------------------------------------------
extern boolean input_get(char * data);

#endif // _EMONESP_INPUT_H
