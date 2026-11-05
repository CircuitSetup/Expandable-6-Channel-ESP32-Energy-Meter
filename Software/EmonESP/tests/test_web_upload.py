"""Exercise production upload callbacks without a device: python tests/test_web_upload.py [g++]."""
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

SOURCE = Path(__file__).resolve().parents[1] / "src/web_server.cpp"
COMPILER = sys.argv[1] if len(sys.argv) > 1 else "g++"

STUBS = r'''
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <map>
struct String : std::string {
  using std::string::string;
  int indexOf(const char* s) { auto p = find(s); return p == npos ? -1 : p; }
};
String www_username = "admin", www_password = "password";
bool station = true;
bool wifi_mode_is_sta() { return station; }
unsigned long systemRestartTime = 0;
unsigned long millis() { return 100; }
#define DBUGF(...)
#define UPDATE_SIZE_UNKNOWN 0
#define U_FLASH 0
#define U_SPIFFS 100
struct AsyncWebServerResponse {
  int code; void addHeader(const char*, const char*) {}
};
struct AsyncWebServerRequest {
  bool authorized = false, challenged = false;
  int status = 0;
  std::map<std::string, bool> attributes;
  std::function<void()> disconnectHandler;
  bool authenticate(const char*, const char*) { return authorized; }
  void requestAuthentication() { challenged = true; status = 401; }
  void setAttribute(const char* key, bool value) { attributes[key] = value; }
  bool getAttribute(const char* key, bool fallback) {
    return attributes.count(key) ? attributes[key] : fallback;
  }
  AsyncWebServerResponse* beginResponse(int code, const char*, const char*) {
    return new AsyncWebServerResponse{code};
  }
  void send(AsyncWebServerResponse* response) { status = response->code; delete response; }
  void onDisconnect(std::function<void()> callback) { disconnectHandler = callback; }
  void disconnect() { if (disconnectHandler) disconnectHandler(); }
};
struct {
  int begins = 0, writes = 0, ends = 0, aborts = 0, command = -1;
  bool error = false, running = false, failBegin = false, failEnd = false, failWrite = false;
  bool begin(int, int cmd) {
    ++begins; command = cmd;
    if (running || failBegin) return false;
    running = true; return true;
  }
  size_t write(uint8_t*, size_t len) { ++writes; return failWrite ? 0 : len; }
  bool end(bool) { ++ends; running = false; error = failEnd; return !error; }
  bool hasError() { return error; }
  void abort() { ++aborts; running = false; error = true; }
} Update;
'''

CHECKS = r'''
int main(int argc, char** argv) {
  assert(argc == 2);
  std::string test = argv[1];
  AsyncWebServerRequest request;
  uint8_t data[4] = {0};
  if (test == "unauthorized" || test == "ap-auth") {
    station = test != "ap-auth";
    handleUpdateUpload(&request, "firmware.bin", 0, data, 4, true);
    handleUpdatePost(&request);
    assert(Update.begins == 0 && Update.writes == 0 && Update.ends == 0);
    assert(request.status == 401 && systemRestartTime == 0);
  } else {
    request.authorized = true;
    if (test == "empty") {
      handleUpdatePost(&request);
      assert(request.status == 400 && systemRestartTime == 0);
    } else if (test == "begin-failure") {
      Update.failBegin = true;
      handleUpdateUpload(&request, "firmware.bin", 0, data, 4, true);
      handleUpdatePost(&request);
      assert(Update.writes == 0 && Update.ends == 0);
      assert(request.status == 400 && systemRestartTime == 0);
    } else if (test == "incomplete") {
      handleUpdateUpload(&request, "firmware.bin", 0, data, 4, false);
      handleUpdatePost(&request);
      assert(request.status == 400 && systemRestartTime == 0);
      assert(Update.aborts == 1 && !Update.running);
    } else if (test == "disconnect" || test == "overlap") {
      handleUpdateUpload(&request, "firmware.bin", 0, data, 4, false);
      AsyncWebServerRequest other;
      other.authorized = true;
      if (test == "disconnect") {
        request.disconnect();
        assert(Update.aborts == 1 && !Update.running);
        handleUpdateUpload(&other, "firmware.bin", 0, data, 4, true);
        handleUpdatePost(&other);
        assert(other.status == 200);
      } else {
        handleUpdateUpload(&other, "spiffs.bin", 0, data, 4, true);
        handleUpdatePost(&other);
        assert(other.status == 400 && Update.writes == 1 && Update.running);
        handleUpdateUpload(&request, "firmware.bin", 4, data, 4, true);
        handleUpdatePost(&request);
        assert(request.status == 200 && Update.writes == 2 && Update.begins == 1);
      }
    } else if (test == "short-write") {
      Update.failWrite = true;
      handleUpdateUpload(&request, "firmware.bin", 0, data, 4, true);
      handleUpdatePost(&request);
      assert(Update.ends == 0 && Update.aborts == 1 && request.status == 400);
    } else if (test == "completed-owner") {
      handleUpdateUpload(&request, "firmware.bin", 0, data, 4, true);
      AsyncWebServerRequest other;
      other.authorized = true;
      Update.failEnd = true;
      handleUpdateUpload(&other, "spiffs.bin", 0, data, 4, true);
      handleUpdatePost(&other);
      handleUpdatePost(&request);
      assert(request.status == 200 && other.status == 400 && Update.begins == 1);
    } else {
      Update.failEnd = test == "end-failure";
      handleUpdateUpload(&request, "spiffs.bin", 0, data, 4, true);
      handleUpdatePost(&request);
      assert(Update.command == U_SPIFFS);
      assert(request.status == (Update.failEnd ? 400 : 200));
      assert((systemRestartTime != 0) == !Update.failEnd);
    }
  }
}
'''

with TemporaryDirectory() as temp:
    source = SOURCE.read_text()
    # Compile both complete production callbacks, including any intervening helpers.
    start = source.find("static AsyncWebServerRequest *updateUploadRequest")
    if start < 0:
        start = source.index("void handleUpdatePost(")
    callbacks = source[start:source.index("void handleNotFound(")]
    cpp = Path(temp) / "upload.cpp"
    exe = Path(temp) / "upload.exe"
    cpp.write_text(STUBS + callbacks + CHECKS)
    subprocess.run([COMPILER, "-std=c++11", "-static", str(cpp), "-o", str(exe)], check=True)
    failures = []
    cases = (
        "unauthorized", "ap-auth", "empty", "begin-failure", "incomplete", "success",
        "end-failure", "disconnect", "overlap", "short-write", "completed-owner",
    )
    for case in cases:
        if subprocess.run([str(exe), case]).returncode:
            failures.append(case)
    if failures:
        sys.exit("Failed: " + ", ".join(failures))
    print(f"Upload regression checks passed ({len(cases)} cases)")
