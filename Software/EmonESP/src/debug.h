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

#ifndef __DEBUG_H
#define __DEBUG_H

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif

#ifndef DEBUG_PORT
#define DEBUG_PORT Serial
#endif

#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)

#ifdef ENABLE_DEBUG
#define DBUGS               DEBUG_PORT
#define DEBUG_BEGIN(speed)  DEBUG_PORT.begin(speed)
#define DBUGF(format, ...)  DEBUG_PORT.printf(format "\n", ##__VA_ARGS__)
#define DBUG(...)           DEBUG_PORT.print(__VA_ARGS__)
#define DBUGLN(...)         DEBUG_PORT.println(__VA_ARGS__)
#else
#define DBUGS               DEBUG_PORT
#define DEBUG_BEGIN(speed)
#define DBUGF(...)
#define DBUG(...)
#define DBUGLN(...)
#endif // DEBUG

#endif // __DEBUG_H
