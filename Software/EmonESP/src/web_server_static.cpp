#if defined(ENABLE_DEBUG) && !defined(ENABLE_DEBUG_WEB_STATIC)
#undef ENABLE_DEBUG
#endif

#include <Arduino.h>

#include "emonesp.h"
#include "web_server.h"
#include "web_server_static.h"
#include "app_config.h"
#include "esp_wifi.h"

// Static files
#include "web_static/web_server_static_files.h"

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof((x)[0]))

#define IS_ALIGNED(x)   (0 == ((uint32_t)(x) & 0x3))

// Pages
static const char _HOME_PAGE[] PROGMEM = "/home.html";
#define HOME_PAGE FPSTR(_HOME_PAGE)

static const char _WIFI_PAGE[] PROGMEM = "/wifi_portal.html";
#define WIFI_PAGE FPSTR(_WIFI_PAGE)

static const char _BUILD_TIME[] PROGMEM = __DATE__ " " __TIME__ " GMT";
#define BUILD_TIME FPSTR(_BUILD_TIME)

static const char _HEADER_IF_MODIFIED_SINCE[] PROGMEM = "If-Modified-Since";
#define HEADER_IF_MODIFIED_SINCE FPSTR(_HEADER_IF_MODIFIED_SINCE)

StaticFileWebHandler::StaticFileWebHandler()
{
}

bool StaticFileWebHandler::_getFile(AsyncWebServerRequest *request, StaticFile **file)
{
  // Remove the found uri
  String path = request->url();
  if(path == "/") {
//    path = String(wifi_mode_is_ap_only() ? WIFI_PAGE : HOME_PAGE);
    path = String(HOME_PAGE);
  }

  DBUGF("Looking for %s", path.c_str());

  for(uint32_t i = 0; i < ARRAY_LENGTH(staticFiles); i++) {
    if(path == staticFiles[i].filename)
    {
      DBUGF("Found %s %d@%p", staticFiles[i].filename, staticFiles[i].length, staticFiles[i].data);

      if(file) {
        *file = &staticFiles[i];
      }
      return true;
    }
  }

  return false;
}

bool StaticFileWebHandler::canHandle(AsyncWebServerRequest *request)
{
  StaticFile *file = NULL;
  if (request->method() == HTTP_GET &&
      _getFile(request, &file))
  {
    request->_tempObject = file;
    DBUGF("[StaticFileWebHandler::canHandle] TRUE");
    request->addInterestingHeader(HEADER_IF_MODIFIED_SINCE);
    return true;
  }

  return false;
}

void StaticFileWebHandler::handleRequest(AsyncWebServerRequest *request)
{
  dumpRequest(request);

  // Get the filename from request->_tempObject and free it
  StaticFile *file = (StaticFile *)request->_tempObject;
  if (file)
  {
    // Clear so we do not try and free
    request->_tempObject = NULL;

    // Are we authenticated
    if(wifi_mode_is_sta() &&
      _username != "" && _password != "" &&
      false == request->authenticate(_username.c_str(), _password.c_str()))
    {
      request->requestAuthentication(node_name.c_str());
      return;
    }

    if (request->header(HEADER_IF_MODIFIED_SINCE).equals(BUILD_TIME)) {
      request->send(304);
      return;
    }

    AsyncWebServerResponse *response = new StaticFileResponse(200, file);
    //response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Last-Modified", BUILD_TIME);
    request->send(response);
  } else {
    request->send(404);
  }
}

StaticFileResponse::StaticFileResponse(int code, StaticFile *content){
  _code = code;
  _content = content;
  _contentType = String(FPSTR(content->type));
  _contentLength = content->length;
  ptr = content->data;
  addHeader("Connection","close");
}

size_t StaticFileResponse::write(AsyncWebServerRequest *request)
{
  size_t total = 0;
  size_t written = 0;
  do {
    written = writeData(request);
    if(written > 0) {
      total += written;
    }
  } while(written > 0);

  if(total > 0)
  {
    //DBUGF("%p: Sending %d", request, total);

    // How should failures to send be handled?
    request->client()->send();
  }

  return total;
}

size_t StaticFileResponse::writeData(AsyncWebServerRequest *request)
{
  size_t space = request->client()->space();

  DBUGF("%p: StaticFileResponse::write: %s %d %d@%p, free %d", request,
    RESPONSE_SETUP == _state ? "RESPONSE_SETUP" :
    RESPONSE_HEADERS == _state ? "RESPONSE_HEADERS" :
    RESPONSE_CONTENT == _state ? "RESPONSE_CONTENT" :
    RESPONSE_WAIT_ACK == _state ? "RESPONSE_WAIT_ACK" :
    RESPONSE_END == _state ? "RESPONSE_END" :
    RESPONSE_FAILED == _state ? "RESPONSE_FAILED" :
    "UNKNOWN",
    space, length, ptr, ESP.getFreeHeap());

  if(length > 0 && space > 0)
  {
    size_t written = 0;

    char buffer[128];
    uint32_t copy = sizeof(buffer);
    if(copy > length) {
      copy = length;
    }
    if(copy > space) {
      copy = space;
    }
    //DBUGF("%p: write %d@%p", request, copy, ptr);
    if(IS_ALIGNED(ptr)) {
      uint32_t *end = (uint32_t *)(ptr + copy);
      for(uint32_t *src = (uint32_t *)ptr, *dst = (uint32_t *)buffer;
          src < end; src++, dst++)
      {
        *dst = *src;
      }
    } else {
      memcpy_P(buffer, ptr, copy);
    }

    written = request->client()->add(buffer, copy);
    if(written > 0) {
      _writtenLength += written;
      ptr += written;
      length -= written;
    } else {
      DBUGF("Failed to write data");
    }

/*
    bool aligned = RESPONSE_CONTENT == _state;
    if(aligned && (!IS_ALIGNED(ptr) || length < 32))
    {
      char buffer[32];
      uint32_t copy = sizeof(buffer) - ((uint32_t)ptr & 0x00000003); // byte aligned mask
      if(copy > length) {
        copy = length;
      }
      DBUGF("None aligned write %d@%p", copy, ptr);
      memcpy_P(buffer, ptr, copy);

      written = request->client()->write(buffer, copy);
      if(written > 0) {
        _writtenLength += written;
        ptr += written;
        length -= written;
      } else {
        DBUGF("Failed to write data");
      }
    }

    if(!aligned || IS_ALIGNED(ptr))
    {
      size_t outLen = length;
      if(outLen > space) {
        outLen = space;
      }
      DBUGF("Aligned write %d@%p", outLen, ptr);
      written = request->client()->write(ptr, outLen);
      if(written > 0) {
        _writtenLength += written;
        ptr += written;
        length -= written;
      } else {
        DBUGF("Failed to write data");
      }
    }
*/

    if(0 == length)
    {
      switch(_state)
      {
        case RESPONSE_HEADERS:
          _state = RESPONSE_CONTENT;
          ptr = _content->data;
          length = _content->length;
          break;
        case RESPONSE_CONTENT:
          _state = RESPONSE_WAIT_ACK;
          break;
        default:
          break;
      }
    }

    return written;
  }

  return 0;
}

void StaticFileResponse::_respond(AsyncWebServerRequest *request){
  _state = RESPONSE_HEADERS;
  _header = _assembleHead(request->version());

  _state = RESPONSE_HEADERS;
  ptr = _header.c_str();
  length = _header.length();

  write(request);
}

size_t StaticFileResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time){
  _ackedLength += len;
  return write(request);
}
