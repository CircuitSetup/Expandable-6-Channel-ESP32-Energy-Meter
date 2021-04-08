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

#ifndef _EMONESP_H
#define _EMONESP_H

// -------------------------------------------------------------------
// General support code used by all modules
// -------------------------------------------------------------------

#include "debug.h"

#ifndef ENABLE_WDT
//#define ENABLE_WDT //will cause issues for Soft AP
#endif

#ifndef MAX_DATA_LEN
#define MAX_DATA_LEN 4096
#endif

#ifndef HTTP_TIMEOUT
#define HTTP_TIMEOUT 4
#endif

#ifndef ENABLE_WEB_OTA
//#define ENABLE_WEB_OTA
#endif

#endif // _EMONESP_H
