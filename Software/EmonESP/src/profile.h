#ifndef __PROFILE_H
#define __PROFILE_H

#if defined(ENABLE_PROFILE) && defined(ENABLE_DEBUG)

#define Profile_Start(x) \
  unsigned long profile ## x = millis()

#define Profile_End(x, max) \
  unsigned long profile ## x ## Diff = millis() - profile ## x; \
  if(profile ## x ## Diff > max) { \
    DBUGF(">> Slow " #x " %lums", profile ## x ## Diff);\
  }

#else // ENABLE_PROFILE

#define Profile_Start(x)
#define Profile_End(x, min)

#endif // ENABLE_PROFILE

#endif
