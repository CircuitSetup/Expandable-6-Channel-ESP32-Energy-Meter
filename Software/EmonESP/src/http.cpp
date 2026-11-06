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
#include "http.h"
#include <mbedtls/sha1.h>
#include <strings.h>

static bool verifyFingerprint(WiFiClientSecure &client, const char *fingerprint, const char *host)
{
  // EEPROM stores the original 20-byte SHA-1 certificate fingerprint.
  unsigned char sha1[20];
  const mbedtls_x509_crt *certificate = client.getPeerCertificate();
  if (strlen(fingerprint) != sizeof(sha1) * 3 - 1 || !certificate ||
      mbedtls_sha1_ret(certificate->raw.p, certificate->raw.len, sha1) != 0) {
    return false;
  }
  for (size_t i = 0; i < sizeof(sha1); ++i) {
    char pair[3];
    snprintf(pair, sizeof(pair), "%02x", static_cast<unsigned int>(sha1[i]));
    const char *byte = fingerprint + i * 3;
    if (strncasecmp(byte, pair, 2) != 0 || (i + 1 < sizeof(sha1) && byte[2] != ':' && byte[2] != ' ')) {
      return false;
    }
  }

  // The Arduino verifier takes SHA-256 and also checks the certificate hostname.
  unsigned char sha256[32];
  if (!client.getFingerprintSHA256(sha256)) return false;
  char sha256Hex[sizeof(sha256) * 2 + 1];
  for (size_t i = 0; i < sizeof(sha256); ++i) {
    snprintf(sha256Hex + i * 2, 3, "%02x", static_cast<unsigned int>(sha256[i]));
  }
  return client.verify(sha256Hex, host);
}

String get_http(const char *host, const char *url, int port, const char *fingerprint)
{
  WiFiClient client;
  WiFiClientSecure client_ssl;
  if (fingerprint) {
    client_ssl.setInsecure(); // Verify the configured pin before sending the request.
    client_ssl.setHandshakeTimeout(HTTP_TIMEOUT);
  }
  WiFiClient &http = fingerprint ? static_cast<WiFiClient &>(client_ssl) : client;
  if (!http.connect(host, port, HTTP_TIMEOUT * 1000)) {
    DBUGS.printf("%s:%d\n", host, port);
    return "Connection error";
  }
  http.setTimeout(HTTP_TIMEOUT);
  if (fingerprint && !verifyFingerprint(client_ssl, fingerprint, host)) {
    http.stop();
    return "HTTPS fingerprint no match";
  }

  const char *format = "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
  const int length = snprintf(nullptr, 0, format, url, host);
  if (length < 0 || http.printf(format, url, host) != static_cast<size_t>(length)) {
    http.stop();
    return "Request write error";
  }

  const unsigned long start = millis();
  char status[sizeof("HTTP/1.1 200 ")] = "";
  size_t count = 0;
  while (millis() - start <= HTTP_TIMEOUT * 1000) {
    const int byte = http.read();
    if (byte == '\n') {
      http.stop();
      if (strcmp(status, "HTTP/1.1 200 ") == 0 || strcmp(status, "HTTP/1.0 200 ") == 0) return "ok";
      return "error " + String(host);
    }
    if (byte >= 0) {
      if (count < sizeof(status) - 1) status[count++] = byte;
    } else {
#ifdef ENABLE_WDT
      feedLoopWDT();
#endif
      delay(1);
    }
  }
  http.stop();
  return "Client Timeout";
}
