import json
import os
import pathlib
import re
import subprocess
import tempfile
import unittest

SRC = pathlib.Path(__file__).resolve().parents[1] / "src"



def run_cpp(code):
    with tempfile.TemporaryDirectory() as tmp:
        cpp = pathlib.Path(tmp) / "test.cpp"
        exe = cpp.with_suffix(".exe" if os.name == "nt" else "")
        cpp.write_text(code)
        subprocess.run([os.environ.get("CXX", "g++"), "-std=c++11", str(cpp), "-o", str(exe)], check=True)
        return subprocess.check_output([str(exe)], text=True, timeout=10)


class RuntimeTests(unittest.TestCase):
    def test_mqtt_publish(self):
        source = (SRC / "mqtt.cpp").read_text()
        publish = source[source.index("void mqtt_publish("):source.index("// MQTT state management")]
        loop = source[source.index("void mqtt_loop("):source.index("void mqtt_restart(")]
        code = r"""
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <cassert>
struct String : std::string {
    using std::string::string;
    using std::string::operator=;
    size_t length() const { return size(); }
};
struct {
    std::vector<std::string> topics, values;
    unsigned int bufferSize = 4096;
    bool connected() { return false; }
    void loop() {}
    unsigned int getBufferSize() { return bufferSize; }
    bool setBufferSize(unsigned int size) { bufferSize = size; return true; }
    bool publish(const char *topic, const char *value, bool = false) {
        topics.emplace_back(topic);
        values.emplace_back(value);
        return true;
    }
} mqttclient;
struct { bool mqtt_json = false; } config_flags;
struct { unsigned long getFreeHeap() { return 123; } } ESP;
struct { int RSSI() { return -50; } } WiFi;
String mqtt_topic = "emon", mqtt_feed_prefix = "";
char mqtt_topic_prefix[128], mqtt_data[64];
#define MAX_DATA_LEN 4096
String input_json;
unsigned long tick, lastMqttReconnectAttempt;
int attempts;
unsigned long millis() { return tick; }
bool mqtt_connect() { ++attempts; return false; }
""" + publish + loop + r"""
int main() {
    mqtt_publish("CT1:2");
    assert(mqttclient.topics[0] == "emon/CT1");
    assert(mqttclient.values[0] == "2");
    mqttclient.topics.clear();
    mqtt_publish(std::string(300, 'x').c_str());
    assert(mqttclient.topics.empty());
    mqtt_publish(("CT1:" + std::string(64, '9')).c_str());
    assert(mqttclient.topics.empty());
    config_flags.mqtt_json = true;
    mqtt_publish("CT1:2");
    assert(mqttclient.topics.empty());
    input_json = String("{\"sensors\":[\"") + std::string(5000, 'a') + "\"]}";
    mqtt_publish("CT1:2");
    assert(mqttclient.values.back() == input_json);
    assert(mqttclient.bufferSize >= input_json.length() + mqtt_topic.length() + 7);
    for (int i = 0; i < 12; ++i) {
        tick += 10001;
        mqtt_loop();
        mqtt_loop();
    }
    assert(attempts == 12);
}
"""
        run_cpp(code)

    def test_meter_json_skips_missing_first_board_and_escapes_names(self):
        source = (SRC / "energy_meter.cpp").read_text()
        loop = source[source.index("void energy_meter_loop()"):]
        escape = (SRC / "json.h").read_text().split("#include <Arduino.h>", 1)[1].rsplit("#endif", 1)[0]
        getters = set(re.findall(r"\.(Get\w+)\(\)", loop))
        status = {"GetSysStatus0", "GetSysStatus1", "GetMeterStatus0", "GetMeterStatus1"}
        getters -= status
        code = r"""
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <cassert>
using std::isfinite;
struct String : std::string {
    using std::string::string;
    using std::string::operator=;
    String(const std::string &text) : std::string(text) {}
    String() {}
    String(float value) : std::string(std::to_string(value)) {}
    String(int value) : std::string(std::to_string(value)) {}
    String(int value, int) : String(value) {}
    bool concat(const std::string &text) { append(text); return true; }
    void remove(size_t start) { erase(start); }
};
""" + escape + r"""
#define NUM_BOARDS 7
#define NUM_INPUTS 6
#define MAX_DATA_LEN 4096
#define HEX 16
struct Logger {
    void println(const std::string &) {}
} Serial, DBUGS;
struct Meter {
    bool ready = true;
    unsigned short GetSysStatus0() { return ready ? 1 : 65535; }
    unsigned short GetSysStatus1() { return 1; }
    unsigned short GetMeterStatus0() { return 1; }
    unsigned short GetMeterStatus1() { return 1; }
""" + "".join("    float " + name + "() { return 1.25; }\n" for name in sorted(getters)) + r"""
} sensor_ic1[NUM_BOARDS], sensor_ic2[NUM_BOARDS];
struct { bool mqtt_json = true; } config_flags;
struct { unsigned long getFreeHeap() { return 123; } } ESP;
struct { int RSSI() { return -50; } } WiFi;
String input_json;
char input_string[MAX_DATA_LEN];
String ct_name[NUM_BOARDS * NUM_INPUTS];
float cur_mul[NUM_BOARDS * NUM_INPUTS], pow_mul[NUM_BOARDS * NUM_INPUTS];
unsigned long startMillis = 1, currentMillis, now = 1001;
const int period = 1000;
unsigned long millis() { return now; }
void delay(int) {}
""" + loop + r"""
int main() {
    for (String &name : ct_name) name = std::string(44, 'a') + char(34) + char(92) + char(10) + '!';
    sensor_ic1[0].ready = false;
    energy_meter_loop();
    puts(input_json.c_str());
    sensor_ic1[0].ready = true;
    now += 1000;
    energy_meter_loop();
    puts(input_json.c_str());
    energy_meter_loop();
    assert(input_json.empty());
}
"""
        missing, full = run_cpp(code).splitlines()
        missing = json.loads(missing)
        self.assertEqual(missing["sensors"][0]["ct"], 7)
        self.assertEqual(missing["sensors"][0]["name"], 'a' * 44 + '"\\\n!')
        self.assertGreater(len(full), 4096)
        full = json.loads(full)
        self.assertEqual(len(full["sensors"]), 42)
        self.assertEqual(full["freeram"], 123)


    def test_http_checks_pin_and_requires_complete_response(self):
        source = (SRC / "http.cpp").read_text()
        get_http = source[source.rfind('#include', 0, source.index("String get_http(")):].split("\n", 1)[1]
        code = r"""
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <strings.h>
#include <string>
#include <cassert>
struct String : std::string {
    using std::string::string;
    String(const std::string &text) : std::string(text) {}
    bool startsWith(const char *prefix) const { return find(prefix) == 0; }
};
struct Logger {
    template <typename... Args> void printf(const char *, Args...) {}
    template <typename T> void println(const T &) {}
} DBUGS;
#define HTTP_TIMEOUT 2
unsigned long tick;
unsigned long millis() { return tick; }
void delay(int ms) { tick += ms; }
std::string request, response;
bool verify = true, pinVerified, stopCalled, shortWrite, secured;
size_t responseIndex;
const unsigned char certificateDer[] = {'a', 'b', 'c'};
struct mbedtls_x509_crt {
    struct { const unsigned char *p; size_t len; } raw;
} peer = {{certificateDer, sizeof(certificateDer)}};
int mbedtls_sha1_ret(const unsigned char *der, size_t length, unsigned char *output) {
    assert(length == sizeof(certificateDer) && memcmp(der, certificateDer, length) == 0);
    // SHA-1("abc") from RFC 3174.
    const unsigned char digest[] = {0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,
                                    0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d};
    memcpy(output, digest, sizeof(digest));
    return 0;
}
struct WiFiClient {
    virtual bool connect(const char *, int, int) { return true; }
    void setTimeout(int seconds) { assert(seconds == HTTP_TIMEOUT); }
    virtual size_t printf(const char *fmt, ...) {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        const int length = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        request = buf;
        return length - shortWrite;
    }
    int available() { return responseIndex < response.size(); }
    int read() { return available() ? response[responseIndex++] : -1; }
    String readStringUntil(char delimiter) { return response.substr(0, response.find(delimiter)); }
    void stop() { stopCalled = true; }
};
struct WiFiClientSecure : WiFiClient {
    void setInsecure() { secured = true; }
    void setHandshakeTimeout(int seconds) { assert(seconds == HTTP_TIMEOUT); }
    const mbedtls_x509_crt *getPeerCertificate() { return &peer; }
    bool getFingerprintSHA256(unsigned char *output) {
        memset(output, 0x12, 32);
        return true;
    }
    bool verify(const char *pin, const char *host) {
        assert(secured && request.empty());
        assert(strcmp(host, "host") == 0);
        std::string expected;
        for (int i = 0; i < 32; ++i) expected += "12";
        assert(pin == expected);
        pinVerified = true;
        return ::verify;
    }
};
""" + get_http + r"""
int main() {
    const char *pin = "a9:99:3E:36:47:06:81:6A:BA:3E:25:71:78:50:C2:6C:9C:D0:D8:9D";
    const char *wrongPin = "a9:99:3E:36:47:06:81:6A:BA:3E:25:71:78:50:C2:6C:9C:D0:D8:90";
    assert(get_http("host", "/path", 443, wrongPin) == "HTTPS fingerprint no match");
    assert(!pinVerified && stopCalled && request.empty());
    assert(get_http("host", "/path", 443, "a9:99") == "HTTPS fingerprint no match");
    assert(!pinVerified && request.empty());

    verify = false;
    assert(get_http("host", "/path", 443, pin) == "HTTPS fingerprint no match");
    assert(pinVerified && stopCalled && request.empty());

    verify = true;
    pinVerified = stopCalled = false;
    response = "HTTP/1.1 200 OK\r\n";
    assert(get_http("host", "/path", 443, pin) == "ok");
    assert(pinVerified && stopCalled && request == "GET /path HTTP/1.1\r\nHost: host\r\nConnection: close\r\n\r\n");

    responseIndex = 0;
    response = "HTTP/1.1 200 OK";
    tick = 0;
    stopCalled = false;
    assert(get_http("host", "/path", 80, nullptr) != "ok");
    assert(stopCalled && tick >= 2000);

    response.clear();
    responseIndex = 0;
    tick = 0;
    stopCalled = false;
    assert(get_http("host", "/path", 80, nullptr) == "Client Timeout");
    assert(stopCalled && tick >= 2000);

    shortWrite = true;
    stopCalled = false;
    assert(get_http("host", "/path", 80, nullptr) == "Request write error");
    assert(stopCalled);
}
"""
        run_cpp(code)


if __name__ == "__main__":
    unittest.main()
