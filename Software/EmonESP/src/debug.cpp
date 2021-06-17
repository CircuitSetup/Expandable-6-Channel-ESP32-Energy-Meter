#include <StreamSpy.h>

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif

#ifndef DEBUG_PORT
#define DEBUG_PORT Serial
#endif

#ifndef DEBUG_LOG_BUFFER
#define DEBUG_LOG_BUFFER 512
#endif

StreamSpy SerialDebug(DEBUG_PORT);

void debug_setup()
{
  DEBUG_PORT.begin(115200);
  SerialDebug.begin(DEBUG_LOG_BUFFER);
}
