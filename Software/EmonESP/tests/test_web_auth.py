"""Run with python tests/test_web_auth.py [path-to-g++]."""

from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


SOURCE = Path(__file__).resolve().parents[1] / "src/web_server.cpp"
COMPILER = sys.argv[1] if len(sys.argv) > 1 else "g++"
STUBS = r"""
#include <cassert>
#include <string>
struct String : std::string {
  using std::string::string;
  bool equals(const char* value) const { return *this == value; }
};
String www_username = "admin", www_password = "old-password";
bool station = true;
bool enableCors = false;
bool wifi_mode_is_sta() { return station; }
const char* esp_hostname = "meter";
#define F(value) value
#define DUMMY_PASSWORD "_DUMMY_PASSWORD"
enum { HTTP_POST, HTTP_OPTIONS };
struct AsyncResponseStream {
  int status = 200;
  void addHeader(const char*, const char*) {}
  void setCode(int code) { status = code; }
  void print(const char*) {}
};
struct {
  bool exists(const char*) { return true; }
} SPIFFS;
struct AsyncStaticWebHandler {
  String username, password;
  int authChanges = 0;
  AsyncStaticWebHandler& setDefaultFile(const char*) { return *this; }
  AsyncStaticWebHandler& setAuthentication(const char* user, const char* pass) {
    username = user;
    password = pass;
    ++authChanges;
    return *this;
  }
};
struct {
  AsyncStaticWebHandler handler;
  AsyncStaticWebHandler& serveStatic(const char*, decltype(SPIFFS)&, const char*) { return handler; }
} server;
struct AsyncWebServerRequest {
  bool authorized = false;
  int status = 0;
  String user = "new-admin", pass = "new-password";
  AsyncResponseStream response;
  int method() { return HTTP_POST; }
  AsyncResponseStream* beginResponseStream(const String&) { return &response; }
  bool authenticate(const char*, const char*) { return authorized; }
  void requestAuthentication(const char* = nullptr) { status = 401; }
  void send(AsyncResponseStream* stream) { status = stream->status; }
  void send(decltype(SPIFFS)&, const char*) { status = 200; }
  void send(int code, const char*, const char*) { status = code; }
  String arg(const char* name) { return std::string(name) == "user" ? user : pass; }
};
void dumpRequest(AsyncWebServerRequest*) {}
void config_save_admin(String user, String pass) { www_username = user; www_password = pass; }
"""
CHECKS = r"""
int main(int argc, char** argv) {
  assert(argc == 2);
  std::string test = argv[1];
  AsyncWebServerRequest request;
  if (test == "blank") www_username = "";
  setupStaticFiles();
  if (test == "ap-unauthorized" || test == "ap-home" || test == "sta-unauthorized") {
    station = test == "sta-unauthorized";
    if (test == "ap-home") {
      handleHome(&request);
    } else {
      handleSaveAdmin(&request);
    }
    if (request.status != 401 || www_username != "admin") return 1;
  } else {
    station = false;
    request.authorized = test != "blank";
    if (test == "long-user") request.user = "0123456789abcdef";
    if (test == "long-pass") request.pass = "0123456789abcdef";
    if (test == "boundary") {
      request.user = "0123456789abcde";
      request.pass = DUMMY_PASSWORD;
    }
    handleSaveAdmin(&request);
    if (test == "long-user" || test == "long-pass") {
      if (request.status != 400 || www_username != "admin" || www_password != "old-password") return 1;
    } else {
      if (request.status != 200 || www_username != request.user) return 1;
      if (www_password != (test == "boundary" ? "old-password" : request.pass.c_str())) return 1;
    }
  }
  const auto& assets = server.handler;
  const int expectedChanges = request.status == 200 ? 2 : 1;
  if (assets.authChanges != expectedChanges || assets.username != www_username || assets.password != www_password) return 1;
}
"""

with TemporaryDirectory() as temp:
    source = SOURCE.read_text()
    static_handler = "\n".join(line for line in source.splitlines() if line.startswith("static AsyncStaticWebHandler *"))
    setup = source[source.index("  // Setup the static files"):source.index("  // Start server & server root html")]
    gate = source[source.index("bool requestPreProcess("):source.index("void handleScan(")]
    save_admin = source[source.index("void handleSaveAdmin("):source.index("void handleLastValues(")]
    cpp = Path(temp) / "auth.cpp"
    exe = Path(temp) / "auth.exe"
    cpp.write_text(STUBS + static_handler + "\nvoid setupStaticFiles() {\n" + setup + "}\n" + gate + save_admin + CHECKS)
    subprocess.run([COMPILER, "-std=c++11", "-static", str(cpp), "-o", str(exe)], check=True)
    cases = (
        "ap-unauthorized", "ap-home", "sta-unauthorized", "ap-authorized", "blank",
        "long-user", "long-pass", "boundary",
    )
    failures = [case for case in cases if subprocess.run([str(exe), case]).returncode]
    if failures:
        sys.exit("Failed: " + ", ".join(failures))
    print(f"Web authentication regression checks passed ({len(cases)} cases)")
