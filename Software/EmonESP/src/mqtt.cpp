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
#include "mqtt.h"
#include "config.h"
#include "esp_wifi.h"
#include "energy_meter.h"
#include "input.h"
#include "board_profile.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

NetworkClient espClient;              // Create client for MQTT
PubSubClient mqttclient(espClient);   // Create client for MQTT

long lastMqttReconnectAttempt = 0;
static int mqtt_connection_error_count = 0;
static char mqtt_state_payload[MQTT_STATE_JSON_SIZE];

#define MQTT_DISCOVERY_PREFIX "homeassistant"

#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)
static const char *mqtt_firmware_version = ESCAPEQUOTE(BUILD_TAG);

static bool append_to_buffer(char *buffer, size_t buffer_size, size_t &offset, const char *format, ...)
{
  if (offset >= buffer_size) {
    return false;
  }

  va_list args;
  va_start(args, format);
  const int written = vsnprintf(buffer + offset, buffer_size - offset, format, args);
  va_end(args);

  if (written < 0) {
    return false;
  }

  if (static_cast<size_t>(written) >= (buffer_size - offset)) {
    offset = buffer_size - 1;
    buffer[offset] = '\0';
    return false;
  }

  offset += static_cast<size_t>(written);
  return true;
}

static bool append_json_raw(char *buffer, size_t buffer_size, size_t &offset, bool &first, const char *key, const char *rawValue)
{
  const bool ok = append_to_buffer(buffer, buffer_size, offset, "%s\"%s\":%s", first ? "" : ",", key, rawValue);
  if (ok) {
    first = false;
  }
  return ok;
}

static bool append_json_float(char *buffer, size_t buffer_size, size_t &offset, bool &first, const char *key, float value, int precision)
{
  char valueBuffer[32];
  snprintf(valueBuffer, sizeof(valueBuffer), "%.*f", precision, value);
  return append_json_raw(buffer, buffer_size, offset, first, key, valueBuffer);
}

static bool append_json_int(char *buffer, size_t buffer_size, size_t &offset, bool &first, const char *key, long value)
{
  char valueBuffer[24];
  snprintf(valueBuffer, sizeof(valueBuffer), "%ld", value);
  return append_json_raw(buffer, buffer_size, offset, first, key, valueBuffer);
}

static bool append_json_null(char *buffer, size_t buffer_size, size_t &offset, bool &first, const char *key)
{
  return append_json_raw(buffer, buffer_size, offset, first, key, "null");
}

static String mqtt_topic_root()
{
  String topic = mqtt_topic;
  while (topic.endsWith("/")) {
    topic.remove(topic.length() - 1);
  }
  return topic;
}

static String mqtt_state_topic()
{
  const String root = mqtt_topic_root();
  if (!root.length()) {
    return "";
  }
  return root + "/state";
}

static String mqtt_status_topic()
{
  const String root = mqtt_topic_root();
  if (!root.length()) {
    return "";
  }
  return root + "/status";
}

static String mqtt_flat_topic_prefix()
{
  const String root = mqtt_topic_root();
  if (!root.length()) {
    return "";
  }
  return root + "/" + mqtt_feed_prefix;
}

static String mqtt_device_id()
{
  char deviceId[32];
  const uint64_t mac = ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL;
  snprintf(deviceId, sizeof(deviceId), "emonesp_%012llx", static_cast<unsigned long long>(mac));
  return String(deviceId);
}

static String mqtt_json_escape(const String &input)
{
  String output;
  output.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    if (c == '\\' || c == '"') {
      output += '\\';
    }
    output += c;
  }

  return output;
}

static String mqtt_channel_label(int channelIndex)
{
  const String fallback = "CT" + String(channelIndex + 1);
  const String configuredName = ct_name[channelIndex];
  const String lowerFallback = "ct" + String(channelIndex + 1);

  if (!configuredName.length()) {
    return fallback;
  }
  if (configuredName.equalsIgnoreCase(fallback) || configuredName.equalsIgnoreCase(lowerFallback)) {
    return fallback;
  }
  return configuredName;
}

static bool mqtt_should_publish_state()
{
  return config_flags.mqtt_json || config_flags.mqtt_home_assistant;
}

static bool mqtt_publish_payload(const String &topic, const char *payload, bool retained = false)
{
  if (!topic.length()) {
    return false;
  }
  return mqttclient.publish(topic.c_str(), payload, retained);
}

static bool mqtt_publish_float_metric(const String &topic, float value, int precision, bool retained = false)
{
  char valueBuffer[32];
  snprintf(valueBuffer, sizeof(valueBuffer), "%.*f", precision, value);
  return mqtt_publish_payload(topic, valueBuffer, retained);
}

static bool mqtt_publish_int_metric(const String &topic, long value, bool retained = false)
{
  char valueBuffer[24];
  snprintf(valueBuffer, sizeof(valueBuffer), "%ld", value);
  return mqtt_publish_payload(topic, valueBuffer, retained);
}

static bool mqtt_publish_flat_metric(const char *metricName, float value, int precision)
{
  const String topic = mqtt_flat_topic_prefix() + metricName;
  return mqtt_publish_float_metric(topic, value, precision);
}

static bool mqtt_publish_flat_int_metric(const char *metricName, long value)
{
  const String topic = mqtt_flat_topic_prefix() + metricName;
  return mqtt_publish_int_metric(topic, value);
}

static bool mqtt_snapshot_has_primary_voltage(const energy_meter_snapshot_t &snapshot)
{
  for (int i = 0; i < NUM_INPUTS; ++i) {
    if (snapshot.channels[i].valid) {
      return true;
    }
  }
  return false;
}

static bool mqtt_build_state_from_snapshot(char *buffer, size_t buffer_size)
{
  const energy_meter_snapshot_t &snapshot = energy_meter_get_snapshot();
  const bool primaryValid = mqtt_snapshot_has_primary_voltage(snapshot);
  bool first = true;
  size_t offset = 0;

  buffer[0] = '\0';
  if (!append_to_buffer(buffer, buffer_size, offset, "{")) {
    return false;
  }

  if (primaryValid) {
    append_json_float(buffer, buffer_size, offset, first, "temperature_c", snapshot.temperature_c, 1);
    append_json_float(buffer, buffer_size, offset, first, "frequency_hz", snapshot.frequency_hz, 2);
    append_json_float(buffer, buffer_size, offset, first, "voltage_l1_v", snapshot.voltage_l1_v, 2);
    append_json_float(buffer, buffer_size, offset, first, "voltage_l2_v", snapshot.voltage_l2_v, 2);
#ifdef THREE_PHASE
    append_json_float(buffer, buffer_size, offset, first, "voltage_l3_v", snapshot.voltage_l3_v, 2);
#endif
  } else {
    append_json_null(buffer, buffer_size, offset, first, "temperature_c");
    append_json_null(buffer, buffer_size, offset, first, "frequency_hz");
    append_json_null(buffer, buffer_size, offset, first, "voltage_l1_v");
    append_json_null(buffer, buffer_size, offset, first, "voltage_l2_v");
#ifdef THREE_PHASE
    append_json_null(buffer, buffer_size, offset, first, "voltage_l3_v");
#endif
  }

  for (int i = 0; i < NUM_CHANNELS; ++i) {
    char key[40];
    const energy_channel_snapshot_t &channel = snapshot.channels[i];

    snprintf(key, sizeof(key), "ct%d_current_a", i + 1);
    channel.valid ? append_json_float(buffer, buffer_size, offset, first, key, channel.current_a, 4)
                  : append_json_null(buffer, buffer_size, offset, first, key);

    snprintf(key, sizeof(key), "ct%d_power_w", i + 1);
    channel.valid ? append_json_float(buffer, buffer_size, offset, first, key, channel.power_w, 2)
                  : append_json_null(buffer, buffer_size, offset, first, key);

    snprintf(key, sizeof(key), "ct%d_power_factor", i + 1);
    channel.valid ? append_json_float(buffer, buffer_size, offset, first, key, channel.power_factor, 3)
                  : append_json_null(buffer, buffer_size, offset, first, key);

    snprintf(key, sizeof(key), "ct%d_apparent_power_va", i + 1);
    channel.valid ? append_json_float(buffer, buffer_size, offset, first, key, channel.apparent_power_va, 2)
                  : append_json_null(buffer, buffer_size, offset, first, key);

    if (config_flags.mqtt_metric_reactive_power) {
      snprintf(key, sizeof(key), "ct%d_reactive_power_var", i + 1);
      channel.valid ? append_json_float(buffer, buffer_size, offset, first, key, channel.reactive_power_var, 2)
                    : append_json_null(buffer, buffer_size, offset, first, key);
    }

    if (config_flags.mqtt_metric_phase_angle) {
      snprintf(key, sizeof(key), "ct%d_phase_angle_deg", i + 1);
      channel.valid ? append_json_float(buffer, buffer_size, offset, first, key, channel.phase_angle_deg, 1)
                    : append_json_null(buffer, buffer_size, offset, first, key);
    }
  }

  if (config_flags.mqtt_metric_totals) {
    if (snapshot.valid) {
      append_json_float(buffer, buffer_size, offset, first, "power_total_w", snapshot.total_power_w, 2);
      append_json_float(buffer, buffer_size, offset, first, "apparent_power_total_va", snapshot.total_apparent_power_va, 2);
      append_json_float(buffer, buffer_size, offset, first, "power_factor_total", snapshot.total_power_factor, 3);
      if (config_flags.mqtt_metric_reactive_power) {
        append_json_float(buffer, buffer_size, offset, first, "reactive_power_total_var", snapshot.total_reactive_power_var, 2);
      }
    } else {
      append_json_null(buffer, buffer_size, offset, first, "power_total_w");
      append_json_null(buffer, buffer_size, offset, first, "apparent_power_total_va");
      append_json_null(buffer, buffer_size, offset, first, "power_factor_total");
      if (config_flags.mqtt_metric_reactive_power) {
        append_json_null(buffer, buffer_size, offset, first, "reactive_power_total_var");
      }
    }
  }

  append_json_int(buffer, buffer_size, offset, first, "free_heap_bytes", static_cast<long>(ESP.getFreeHeap()));
  if (wifi_transport_is_ethernet()) {
    append_json_null(buffer, buffer_size, offset, first, "signal_dbm");
  } else {
    append_json_int(buffer, buffer_size, offset, first, "signal_dbm", static_cast<long>(wifi_signal_strength()));
  }

  return append_to_buffer(buffer, buffer_size, offset, "}");
}

static String mqtt_translate_legacy_key(const char *key)
{
  if (!strcmp(key, "temp")) return "temperature_c";
  if (!strcmp(key, "freq")) return "frequency_hz";
  if (!strcmp(key, "V1")) return "voltage_l1_v";
  if (!strcmp(key, "V2")) return "voltage_l2_v";
  if (!strcmp(key, "V3")) return "voltage_l3_v";
  if (!strcmp(key, "freeram")) return "free_heap_bytes";
  if (!strcmp(key, "rssi")) return "signal_dbm";
  if (!strcmp(key, "W_total")) return "power_total_w";
  if (!strcmp(key, "VA_total")) return "apparent_power_total_va";
  if (!strcmp(key, "PF_total")) return "power_factor_total";
  if (!strcmp(key, "VAR_total")) return "reactive_power_total_var";

  if (!strncmp(key, "ANGLE", 5)) {
    return "ct" + String(atoi(key + 5)) + "_phase_angle_deg";
  }
  if (!strncmp(key, "VAR", 3)) {
    return "ct" + String(atoi(key + 3)) + "_reactive_power_var";
  }
  if (!strncmp(key, "VA", 2)) {
    return "ct" + String(atoi(key + 2)) + "_apparent_power_va";
  }
  if (!strncmp(key, "PF", 2)) {
    return "ct" + String(atoi(key + 2)) + "_power_factor";
  }
  if (key[0] == 'W' && key[1] != '_' && isdigit(static_cast<unsigned char>(key[1]))) {
    return "ct" + String(atoi(key + 1)) + "_power_w";
  }
  if (!strncmp(key, "CT", 2)) {
    return "ct" + String(atoi(key + 2)) + "_current_a";
  }

  return "";
}

static bool mqtt_build_state_from_input(char *buffer, size_t buffer_size, const char *data)
{
  char dataCopy[MAX_DATA_LEN];
  bool first = true;
  size_t offset = 0;
  bool appendedAny = false;

  strncpy(dataCopy, data, sizeof(dataCopy) - 1);
  dataCopy[sizeof(dataCopy) - 1] = '\0';

  buffer[0] = '\0';
  if (!append_to_buffer(buffer, buffer_size, offset, "{")) {
    return false;
  }

  char *context = nullptr;
  char *token = strtok_r(dataCopy, ",", &context);
  while (token != nullptr) {
    char *separator = strchr(token, ':');
    if (separator != nullptr) {
      *separator = '\0';
      const char *key = token;
      const char *value = separator + 1;
      const String mappedKey = mqtt_translate_legacy_key(key);
      if (mappedKey.length()) {
        append_json_raw(buffer, buffer_size, offset, first, mappedKey.c_str(), value);
        appendedAny = true;
      }
    }
    token = strtok_r(nullptr, ",", &context);
  }

  append_json_int(buffer, buffer_size, offset, first, "free_heap_bytes", static_cast<long>(ESP.getFreeHeap()));
  if (wifi_transport_is_ethernet()) {
    append_json_null(buffer, buffer_size, offset, first, "signal_dbm");
  } else {
    append_json_int(buffer, buffer_size, offset, first, "signal_dbm", static_cast<long>(wifi_signal_strength()));
  }

  if (!append_to_buffer(buffer, buffer_size, offset, "}")) {
    return false;
  }

  return appendedAny || first == false;
}

static bool mqtt_publish_flat_from_input(const char *data)
{
  const char *data_ptr = data;
  const String topicPrefix = mqtt_flat_topic_prefix();
  char topic[160];
  char value[64];

  if (!topicPrefix.length()) {
    return false;
  }

  while (*data_ptr != '\0') {
    const int keyLength = strcspn(data_ptr, ":");
    if (keyLength <= 0) {
      break;
    }

    snprintf(topic, sizeof(topic), "%s%.*s", topicPrefix.c_str(), keyLength, data_ptr);
    data_ptr += keyLength;
    if (*data_ptr++ == '\0') {
      break;
    }

    const int valueLength = strcspn(data_ptr, ",");
    const size_t copyLength = (valueLength < static_cast<int>(sizeof(value) - 1))
      ? static_cast<size_t>(valueLength)
      : (sizeof(value) - 1);
    strncpy(value, data_ptr, copyLength);
    value[copyLength] = '\0';
    data_ptr += valueLength;
    if (*data_ptr == ',') {
      ++data_ptr;
    }

    if (!mqtt_publish_payload(topic, value)) {
      return false;
    }
  }

  if (!mqtt_publish_flat_int_metric("freeram", static_cast<long>(ESP.getFreeHeap()))) {
    return false;
  }
  return mqtt_publish_flat_int_metric("rssi", static_cast<long>(wifi_signal_strength()));
}

static bool mqtt_publish_flat_from_snapshot()
{
  const energy_meter_snapshot_t &snapshot = energy_meter_get_snapshot();
  const bool primaryValid = mqtt_snapshot_has_primary_voltage(snapshot);

  if (!mqtt_flat_topic_prefix().length()) {
    return false;
  }

  if (primaryValid) {
    if (!mqtt_publish_flat_metric("temp", snapshot.temperature_c, 1)) return false;
    if (!mqtt_publish_flat_metric("freq", snapshot.frequency_hz, 2)) return false;
    if (!mqtt_publish_flat_metric("V1", snapshot.voltage_l1_v, 2)) return false;
    if (!mqtt_publish_flat_metric("V2", snapshot.voltage_l2_v, 2)) return false;
#ifdef THREE_PHASE
    if (!mqtt_publish_flat_metric("V3", snapshot.voltage_l3_v, 2)) return false;
#endif
  }

  for (int i = 0; i < NUM_CHANNELS; ++i) {
    if (!snapshot.channels[i].valid) {
      continue;
    }

    char metricName[24];
    snprintf(metricName, sizeof(metricName), "CT%d", i + 1);
    if (!mqtt_publish_flat_metric(metricName, snapshot.channels[i].current_a, 4)) return false;

    snprintf(metricName, sizeof(metricName), "PF%d", i + 1);
    if (!mqtt_publish_flat_metric(metricName, snapshot.channels[i].power_factor, 3)) return false;

    snprintf(metricName, sizeof(metricName), "W%d", i + 1);
    if (!mqtt_publish_flat_metric(metricName, snapshot.channels[i].power_w, 2)) return false;

    snprintf(metricName, sizeof(metricName), "VA%d", i + 1);
    if (!mqtt_publish_flat_metric(metricName, snapshot.channels[i].apparent_power_va, 2)) return false;

    if (config_flags.mqtt_metric_reactive_power) {
      snprintf(metricName, sizeof(metricName), "VAR%d", i + 1);
      if (!mqtt_publish_flat_metric(metricName, snapshot.channels[i].reactive_power_var, 2)) return false;
    }

    if (config_flags.mqtt_metric_phase_angle) {
      snprintf(metricName, sizeof(metricName), "ANGLE%d", i + 1);
      if (!mqtt_publish_flat_metric(metricName, snapshot.channels[i].phase_angle_deg, 1)) return false;
    }
  }

  if (config_flags.mqtt_metric_totals && snapshot.valid) {
    if (!mqtt_publish_flat_metric("W_total", snapshot.total_power_w, 2)) return false;
    if (!mqtt_publish_flat_metric("VA_total", snapshot.total_apparent_power_va, 2)) return false;
    if (!mqtt_publish_flat_metric("PF_total", snapshot.total_power_factor, 3)) return false;
    if (config_flags.mqtt_metric_reactive_power) {
      if (!mqtt_publish_flat_metric("VAR_total", snapshot.total_reactive_power_var, 2)) return false;
    }
  }

  if (!mqtt_publish_flat_int_metric("freeram", static_cast<long>(ESP.getFreeHeap()))) return false;
  return mqtt_publish_flat_int_metric("rssi", static_cast<long>(wifi_signal_strength()));
}

static bool mqtt_publish_state_topic(const char *data)
{
  const bool buildFromSnapshot = input_last_data_from_energy_meter() && energy_meter_snapshot_valid();
  const bool built = buildFromSnapshot
    ? mqtt_build_state_from_snapshot(mqtt_state_payload, sizeof(mqtt_state_payload))
    : mqtt_build_state_from_input(mqtt_state_payload, sizeof(mqtt_state_payload), data);

  if (!built) {
    return false;
  }

  return mqtt_publish_payload(mqtt_state_topic(), mqtt_state_payload, false);
}

static String mqtt_discovery_topic(const String &objectId)
{
  return String(MQTT_DISCOVERY_PREFIX) + "/sensor/" + mqtt_device_id() + "/" + objectId + "/config";
}

static bool mqtt_publish_discovery_metric(const String &objectId,
                                          const String &name,
                                          const String &stateKey,
                                          const char *unit,
                                          const char *deviceClass,
                                          const char *stateClass,
                                          bool enabled)
{
  const String topic = mqtt_discovery_topic(objectId);

  if (!config_flags.mqtt_home_assistant || !enabled) {
    return mqtt_publish_payload(topic, "", true);
  }

  String payload;
  payload.reserve(768);
  payload += "{";
  payload += "\"name\":\"" + mqtt_json_escape(name) + "\",";
  payload += "\"unique_id\":\"" + mqtt_device_id() + "_" + objectId + "\",";
  payload += "\"state_topic\":\"" + mqtt_state_topic() + "\",";
  payload += "\"availability_topic\":\"" + mqtt_status_topic() + "\",";
  payload += "\"value_template\":\"{{ value_json." + stateKey + " }}\",";
  if (unit && unit[0]) {
    payload += "\"unit_of_measurement\":\"" + String(unit) + "\",";
  }
  if (deviceClass && deviceClass[0]) {
    payload += "\"device_class\":\"" + String(deviceClass) + "\",";
  }
  if (stateClass && stateClass[0]) {
    payload += "\"state_class\":\"" + String(stateClass) + "\",";
  }
  payload += "\"device\":{";
  payload += "\"identifiers\":[\"" + mqtt_device_id() + "\"],";
  payload += "\"manufacturer\":\"CircuitSetup\",";
  payload += "\"model\":\"Expandable 6-Channel Energy Meter (" + String(BOARD_PROFILE_NAME) + ")\",";
  payload += "\"name\":\"" + String(esp_hostname) + "\",";
  payload += "\"sw_version\":\"" + String(mqtt_firmware_version) + "\"";
  payload += "}}";

  return mqtt_publish_payload(topic, payload.c_str(), true);
}

static bool mqtt_sync_home_assistant()
{
  bool ok = true;
  const bool publishVoltageL3 =
#ifdef THREE_PHASE
    true;
#else
    false;
#endif

  ok = mqtt_publish_discovery_metric("temperature", "Temperature", "temperature_c", "C", "temperature", "measurement", true) && ok;
  ok = mqtt_publish_discovery_metric("frequency", "Frequency", "frequency_hz", "Hz", "frequency", "measurement", true) && ok;
  ok = mqtt_publish_discovery_metric("voltage_l1", "Voltage L1", "voltage_l1_v", "V", "voltage", "measurement", true) && ok;
  ok = mqtt_publish_discovery_metric("voltage_l2", "Voltage L2", "voltage_l2_v", "V", "voltage", "measurement", true) && ok;
  ok = mqtt_publish_discovery_metric("voltage_l3", "Voltage L3", "voltage_l3_v", "V", "voltage", "measurement", publishVoltageL3) && ok;

  for (int i = 0; i < NUM_CHANNELS; ++i) {
    const String channelLabel = mqtt_channel_label(i);
    char objectId[32];
    char stateKey[40];

    snprintf(objectId, sizeof(objectId), "ct%02d_current", i + 1);
    snprintf(stateKey, sizeof(stateKey), "ct%d_current_a", i + 1);
    ok = mqtt_publish_discovery_metric(objectId, channelLabel + " Current", stateKey, "A", "current", "measurement", true) && ok;

    snprintf(objectId, sizeof(objectId), "ct%02d_power", i + 1);
    snprintf(stateKey, sizeof(stateKey), "ct%d_power_w", i + 1);
    ok = mqtt_publish_discovery_metric(objectId, channelLabel + " Power", stateKey, "W", "power", "measurement", true) && ok;

    snprintf(objectId, sizeof(objectId), "ct%02d_power_factor", i + 1);
    snprintf(stateKey, sizeof(stateKey), "ct%d_power_factor", i + 1);
    ok = mqtt_publish_discovery_metric(objectId, channelLabel + " Power Factor", stateKey, "", "", "measurement", true) && ok;

    snprintf(objectId, sizeof(objectId), "ct%02d_apparent_power", i + 1);
    snprintf(stateKey, sizeof(stateKey), "ct%d_apparent_power_va", i + 1);
    ok = mqtt_publish_discovery_metric(objectId, channelLabel + " Apparent Power", stateKey, "VA", "", "measurement", true) && ok;

    snprintf(objectId, sizeof(objectId), "ct%02d_reactive_power", i + 1);
    snprintf(stateKey, sizeof(stateKey), "ct%d_reactive_power_var", i + 1);
    ok = mqtt_publish_discovery_metric(objectId, channelLabel + " Reactive Power", stateKey, "VAR", "", "measurement", config_flags.mqtt_metric_reactive_power) && ok;

    snprintf(objectId, sizeof(objectId), "ct%02d_phase_angle", i + 1);
    snprintf(stateKey, sizeof(stateKey), "ct%d_phase_angle_deg", i + 1);
    ok = mqtt_publish_discovery_metric(objectId, channelLabel + " Phase Angle", stateKey, "deg", "", "measurement", config_flags.mqtt_metric_phase_angle) && ok;
  }

  ok = mqtt_publish_discovery_metric("total_power", "Total Power", "power_total_w", "W", "power", "measurement", config_flags.mqtt_metric_totals) && ok;
  ok = mqtt_publish_discovery_metric("total_apparent_power", "Total Apparent Power", "apparent_power_total_va", "VA", "", "measurement", config_flags.mqtt_metric_totals) && ok;
  ok = mqtt_publish_discovery_metric("total_power_factor", "Total Power Factor", "power_factor_total", "", "", "measurement", config_flags.mqtt_metric_totals) && ok;
  ok = mqtt_publish_discovery_metric("total_reactive_power", "Total Reactive Power", "reactive_power_total_var", "VAR", "", "measurement",
                                     config_flags.mqtt_metric_totals && config_flags.mqtt_metric_reactive_power) && ok;

  return ok;
}

// -------------------------------------------------------------------
// MQTT Connect
// Called only when MQTT server field is populated
// -------------------------------------------------------------------
boolean mqtt_connect()
{
  DBUGS.println("MQTT Connecting...");

  if (espClient.connect(mqtt_server.c_str(), 1883, MQTT_TIMEOUT * 1000) != 1)
  {
     DBUGS.println("MQTT connect timeout.");
     return (0);
  }

  espClient.setTimeout(MQTT_TIMEOUT);
  mqttclient.setSocketTimeout(MQTT_TIMEOUT);
  mqttclient.setBufferSize(MQTT_STATE_JSON_SIZE + 256);

  const String clientId = mqtt_device_id();
  const String statusTopic = mqtt_status_topic();
  bool connected = false;

  if (mqtt_user.length() == 0) {
    if (statusTopic.length()) {
      connected = mqttclient.connect(clientId.c_str(), statusTopic.c_str(), 0, true, "offline");
    } else {
      connected = mqttclient.connect(clientId.c_str());
    }
  } else {
    if (statusTopic.length()) {
      connected = mqttclient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str(), statusTopic.c_str(), 0, true, "offline");
    } else {
      connected = mqttclient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
    }
  }

  if (!connected)
  {
    DBUGS.print("MQTT failed: ");
    DBUGS.println(mqttclient.state());
    return (0);
  }

  DBUGS.println("MQTT connected");

  if (statusTopic.length()) {
    mqtt_publish_payload(statusTopic, "online", true);
  }

  mqtt_sync_home_assistant();

  if (config_flags.mqtt_legacy_flat && !mqtt_should_publish_state() && mqtt_topic_root().length()) {
    mqtt_publish_payload(mqtt_topic_root(), "connected", false);
  }

  return (1);
}

// -------------------------------------------------------------------
// Publish to MQTT
// -------------------------------------------------------------------
void mqtt_publish(const char * data)
{
  if (config_flags.mqtt_legacy_flat) {
    if (input_last_data_from_energy_meter() && energy_meter_snapshot_valid()) {
      if (!mqtt_publish_flat_from_snapshot()) {
        return;
      }
    } else if (!mqtt_publish_flat_from_input(data)) {
      return;
    }
  }

  if (mqtt_should_publish_state()) {
    mqtt_publish_state_topic(data);
  }
}

// -------------------------------------------------------------------
// MQTT state management
//
// Call every time around loop() if connected to the active network uplink
// -------------------------------------------------------------------
void mqtt_loop()
{
  if (!mqttclient.connected()) {
    long now = millis();
    // try and reconnect continuously for first 5s then try again once every 10s
    if ( (now < 5000) || ((now - lastMqttReconnectAttempt)  > 10000) ) {
      lastMqttReconnectAttempt = now;
      if (mqtt_connect()) { // Attempt to reconnect
        lastMqttReconnectAttempt = 0;
        mqtt_connection_error_count = 0;
      } else {
        mqtt_connection_error_count ++;
        if (mqtt_connection_error_count > 10) {
          esp_restart();
        }
      }
    }
  } else {
    // if MQTT connected
    mqttclient.loop();
  }
}

void mqtt_restart()
{
  if (mqttclient.connected()) {
    mqttclient.disconnect();
  }
  espClient.stop();
}

boolean mqtt_connected()
{
  return mqttclient.connected();
}
