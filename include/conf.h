#pragma once
#include <WString.h>
#include <FS.h>

struct conf_t {
    bool use_ap;

    String ap_ssid;
    String ap_pass;

    String wifi_ssid;
    String wifi_pass;

    String ntp_server;

    long feed_interval;
    long feed_duration;
    long feed_offset;

    void load(FS &fs, const String &name);
    void save(FS &fs, const String &name);
};