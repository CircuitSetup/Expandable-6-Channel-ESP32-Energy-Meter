#ifndef EMONESP_JSON_H
#define EMONESP_JSON_H

#include <Arduino.h>

inline String jsonEscape(const String &value)
{
  String escaped;
  for (unsigned int i = 0; i < value.length(); ++i) {
    unsigned char byte = value[i];
    if (byte < 0x20) {
      char hex[7];
      snprintf(hex, sizeof(hex), "\\u%04x", static_cast<unsigned int>(byte));
      escaped += hex;
    } else {
      if (byte == '"' || byte == '\\') escaped += '\\';
      escaped += static_cast<char>(byte);
    }
  }
  return escaped;
}

#endif
