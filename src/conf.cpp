#include <Arduino.h>
#include "./conf.h"

static bool read_string(File &file, String &value) {
    if (file.read() == -1) return false;
    else file.seek(-1, SeekMode::SeekCur);

    value = file.readStringUntil('\n');
    if (value.length() == 0) return false;
    value = value.substring(0, value.length() - 1);
    return true;
}
static bool read_bool(File &file, bool &value) {
    String tmp;
    if (!read_string(file, tmp)) return false;
    if (tmp == "true") value = true;
    else if (tmp == "false") value = false;
    else return false;
    return true;
}
static bool read_int(File &file, long &value) {
    String tmp;
    if (!read_string(file, tmp)) return false;
    const char *ptr = tmp.c_str();
    char *end;
    value = strtol(ptr, &end, 10);
    return ptr != end;
}

static void write_bool(File &file, bool value) {
    file.println(value ? "true" : "false");
}
static void write_int(File &file, long value) {
    file.println(value);
}
static void write_string(File &file, String value) {
    file.println(value);
}

void conf_t::load(FS &fs, const String &name) {
    auto f = fs.open(name, "r");
    read_bool(f, use_ap);
    read_string(f, ap_ssid);
    read_string(f, ap_pass);
    read_string(f, wifi_ssid);
    read_string(f, wifi_pass);
    read_string(f, ntp_server);
    read_int(f, feed_interval);
    read_int(f, feed_duration);
    read_int(f, feed_offset);
    f.close();
}
void conf_t::save(FS &fs, const String &name) {
    auto f = fs.open(name, "w");
    write_bool(f, use_ap);
    write_string(f, ap_ssid);
    write_string(f, ap_pass);
    write_string(f, wifi_ssid);
    write_string(f, wifi_pass);
    write_string(f, ntp_server);
    write_int(f, feed_interval);
    write_int(f, feed_duration);
    write_int(f, feed_offset);
    f.close();
}