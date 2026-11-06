"""Run with python tests/test_config_eeprom.py [path-to-g++]."""

from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src"
COMPILER = sys.argv[1] if len(sys.argv) > 1 else "g++"

STUBS = r"""
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <math.h>
#include <string>
using byte = uint8_t;
struct String : std::string {
  using std::string::string;
  String(const std::string& value) : std::string(value) {}
  String(int value) : std::string(std::to_string(value)) {}
  long toInt() const { return std::strtol(c_str(), nullptr, 10); }
  float toFloat() const { return std::strtof(c_str(), nullptr); }
};
struct AsyncWebServerRequest {
  std::map<std::string, std::string> args;
  bool hasArg(const char* name) { return args.count(name) != 0; }
  String arg(const char* name) { return hasArg(name) ? args.at(name) : ""; }
};
struct {
  byte bytes[4096];
  void begin(int size) { assert(size == sizeof(bytes)); }
  byte read(int offset) { assert(offset >= 0 && offset < sizeof(bytes)); return bytes[offset]; }
  void write(int offset, byte value) { assert(offset >= 0 && offset < sizeof(bytes)); bytes[offset] = value; }
  void end() {}
  void commit() {}
} EEPROM;
std::string debug_output;
template <typename... Args>
void DBUGF(const char* format, Args... args) {
  char message[256];
  std::snprintf(message, sizeof(message), format, args...);
  debug_output += message;
}
#define NUM_INPUTS 6
#ifndef NUM_BOARDS
#define NUM_BOARDS 7
#endif
#define NUM_CHANNELS (NUM_BOARDS * NUM_INPUTS)
#define VOLTAGE_GAIN_DEFAULT 120
#define LINE_FREQ_DEFAULT 60
#define PGA_GAIN_DEFAULT 0
#define CURRENT_GAIN_DEFAULT 100
"""

CHECKS = r"""
int main(int argc, char** argv) {
  assert(argc == 2);
  std::string test = argv[1];
  config_reset();
  if (test == "reload") {
    config_save_wifi("stored network", "stored password");
    config_load_settings();
    config_load_settings();
    assert(esid == "stored network");
    assert(epass == "stored password");
  } else if (test == "legacy") {
    const int start = EEPROM_CONFIG_FLAGS_START;
    EEPROM.write(start, 1);
    EEPROM.write(start + 1, 0xAB);
    EEPROM.write(start + 2, 128 ^ 1 ^ 0xAB);
    config_load_settings();
    assert(config_flags.mqtt_json);
    config_save_mqtt("host", "topic", "prefix", "user", "password", false);
    assert(EEPROM.read(start) == 0);
    assert(EEPROM.read(start + 1) == 0);
    assert(EEPROM.read(start + 2) == 128);
    config_load_settings();
    assert(!config_flags.mqtt_json);
  } else if (test == "calibration") {
    EEPROM_write_ushort(EEPROM_CAL_VOLTAGE_START, 0x1234);
    EEPROM_write_float(EEPROM_CUR_MUL_START, -1.25f);
    config_load_settings();
    assert(voltage_cal == 0x1234);
    assert(cur_mul[0] == -1.25f);
    EEPROM.write(EEPROM_CAL_VOLTAGE_END - 1, 0);
    EEPROM.write(EEPROM_CUR_MUL_START + EEPROM_CUR_MUL_SIZE - 1, 0);
    config_load_settings();
    assert(voltage_cal == VOLTAGE_GAIN_DEFAULT);
    assert(cur_mul[0] == 1.0f);
  } else if (test == "fingerprint") {
    const String fingerprint = "01:23:45:67:89:AB:CD:EF:12:34:56:78:90:AB:CD:EF:12:34:56:78";
    config_save_emoncms("server", "path", "node", "key", fingerprint);
    config_load_settings();
    assert(emoncms_fingerprint == fingerprint);
    config_save_emoncms("server", "path", "node", "key", "");
    config_load_settings();
    assert(emoncms_fingerprint.empty());
  } else if (test == "logs") {
    config_save_wifi("network", "wifi-secret");
    config_save_admin("admin", "admin-secret");
    config_save_mqtt("host", "topic", "prefix", "user", "mqtt-secret", true);
    config_save_emoncms("server", "path", "node", "emon-secret", "");
    config_load_settings();
    assert(debug_output.find("-secret") == std::string::npos);
  } else if (test.compare(0, 4, "cal-") == 0) {
    AsyncWebServerRequest request;
    request.args["voltage_cal"] = "5000";
    request.args["voltage2_cal"] = "6000";
    request.args["freq_cal"] = "65535";
    for (int i = 1; i <= NUM_CHANNELS; ++i) {
      const auto suffix = std::to_string(i);
      request.args["ct" + suffix + "_name"] = "sensor";
      request.args["ct" + suffix + "_cal"] = "300";
      request.args["cur" + suffix + "_mul"] = "-1.25";
      request.args["pow" + suffix + "_mul"] = "2.5";
      request.args["gain" + suffix + "_cal"] = "4";
    }
    config_load_settings();
    byte oldBytes[sizeof(EEPROM.bytes)];
    memcpy(oldBytes, EEPROM.bytes, sizeof(oldBytes));
    const auto last = std::to_string(NUM_CHANNELS);
    if (test == "cal-missing") request.args.erase("ct" + last + "_name");
    if (test == "cal-overflow") request.args["ct" + last + "_cal"] = "65536";
    if (test == "cal-negative") request.args["ct" + last + "_cal"] = "-1";
    if (test == "cal-nonfinite") request.args["cur" + last + "_mul"] = "nan";
    if (test == "cal-trailing") request.args["pow" + last + "_mul"] = "3.25oops";
    if (test == "cal-gain") request.args["gain" + last + "_cal"] = "3";
    if (test == "cal-name") request.args["ct" + last + "_name"] = std::string(49, 'x');
    assert(config_save_cal(&request) == (test == "cal-valid"));
    if (test == "cal-valid") {
      config_load_settings();
      assert(voltage_cal == 5000 && voltage2_cal == 6000 && freq_cal == 65535);
      assert(ct_cal[NUM_CHANNELS - 1] == 300);
      assert(ct_name[NUM_CHANNELS - 1] == "sensor");
      assert(cur_mul[NUM_CHANNELS - 1] == -1.25f && pow_mul[NUM_CHANNELS - 1] == 2.5f);
      assert(gain_cal[NUM_BOARDS - 1] == 0x2A2A);
    } else {
      assert(voltage_cal == VOLTAGE_GAIN_DEFAULT);
      assert(ct_cal[NUM_CHANNELS - 1] == CURRENT_GAIN_DEFAULT);
      assert(memcmp(oldBytes, EEPROM.bytes, sizeof(oldBytes)) == 0);
    }
  } else {
    assert(false);
  }
}
"""

with TemporaryDirectory() as temp:
    source = Path(temp) / "config_test.cpp"
    executable = Path(temp) / "config_test.exe"
    # Replace hardware headers only. Compile the production implementations verbatim.
    headers = "\n".join(
        line for line in (SOURCE / "config.h").read_text().splitlines()
        if not line.startswith("#include")
    )
    config = "\n".join(
        line for line in (SOURCE / "config.cpp").read_text().splitlines()
        if not line.startswith("#include")
    )
    source.write_text("\n".join((STUBS, headers, config, CHECKS)))
    compile_args = [COMPILER, "-std=c++11", "-O2", "-static", str(source)]
    subprocess.run([*compile_args, "-o", str(executable)], check=True)
    failures = []
    cases = (
        "reload", "legacy", "calibration", "fingerprint", "logs", "cal-valid",
        "cal-missing", "cal-overflow", "cal-negative", "cal-nonfinite",
        "cal-trailing", "cal-gain", "cal-name",
    )
    for case in cases:
        if subprocess.run([str(executable), case]).returncode:
            failures.append(case)
    oversized = subprocess.run(
        [*compile_args, "-DNUM_BOARDS=100", "-fsyntax-only"],
        capture_output=True, text=True,
    )
    if oversized.returncode == 0 or "EEPROM_SIZE too small" not in oversized.stderr:
        failures.append("oversized layout")
    if failures:
        sys.exit("Failed: " + ", ".join(failures))
    print("EEPROM regression checks passed")
