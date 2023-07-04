#ifndef __DEBUG_H
#define __DEBUG_H

#undef DEBUG_PORT
#define DEBUG_PORT SerialDebug

#include "MicroDebug.h"
#include "StreamSpy.h"

extern StreamSpy SerialDebug;

extern void debug_setup();

#endif // __DEBUG_H
