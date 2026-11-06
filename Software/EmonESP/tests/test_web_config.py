"""Check the actual configuration response with quotes/control bytes in saved names."""
from pathlib import Path
import json
import os
import subprocess
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/web_server.cpp").read_text()
handler = source[source.index("void handleConfig("):source.index("void handleRst(")]
escape = "\n".join(line for line in (ROOT / "src/json.h").read_text().splitlines()
                   if not line.startswith("#include"))
stubs = r'''
#include <cstdio>
#include <string>
#include <cstdint>
#include <type_traits>
using byte = uint8_t;
struct String : std::string {
  using std::string::string;
  String() = default;
  String(const std::string& s) : std::string(s) {}
  template <typename T> String(T n) : std::string(std::to_string(n)) {}
  bool operator!=(int) const { return !empty(); }
};
template <typename T> typename std::enable_if<std::is_arithmetic<T>::value, String>::type
operator+(const std::string& s, T n) { return s + std::to_string(n); }
struct AsyncResponseStream {
  void setCode(int) {}
  void print(const String& value) { puts(value.c_str()); }
};
struct AsyncWebServerRequest { void send(AsyncResponseStream* r) { delete r; } };
bool requestPreProcess(AsyncWebServerRequest*, AsyncResponseStream*& response) {
  response = new AsyncResponseStream; return true;
}
#define DUMMY_PASSWORD "_DUMMY_PASSWORD"
#define NUM_BOARDS 1
#define NUM_INPUTS 6
#define NUM_CHANNELS 6
struct { unsigned int getFlashChipSize() { return 4194304; } } ESP;
String currentfirmware = "2.6";
String esid, epass, emoncms_server, emoncms_path, emoncms_node, emoncms_apikey;
String emoncms_fingerprint, mqtt_server, mqtt_topic, mqtt_feed_prefix, mqtt_user, mqtt_pass;
String www_username, www_password, ct_name[6];
unsigned short voltage_cal = 7305, voltage2_cal = 7305, freq_cal = 4231, gain_cal[1] = {};
unsigned short ct_cal[6] = {};
float cur_mul[6] = {}, pow_mul[6] = {};
struct { bool mqtt_json = true; } config_flags;
'''
checks = r'''
int main() {
  esid = "a\"b\\c\n";
  ct_name[0] = "Kitchen\t\"oven\"";
  mqtt_topic = "meter\\floor";
  www_username = "a\"b";
  epass = "secret";
  AsyncWebServerRequest request;
  handleConfig(&request);
}
'''
with TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / "config.cpp", Path(temp) / "config.exe"
    cpp.write_text("\n".join((stubs, escape, handler, checks)))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++11", "-static", str(cpp), "-o", str(exe)], check=True)
    data = json.loads(subprocess.check_output([str(exe)], text=True))
    assert data["ssid"] == 'a"b\\c\n'
    assert data["ct1_name"] == 'Kitchen\t"oven"'
    assert data["mqtt_topic"] == 'meter\\floor'
    assert data["www_username"] == 'a"b'
    assert data["pass"] == "_DUMMY_PASSWORD"
    print("Configuration JSON regression passed")
