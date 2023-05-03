#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <LittleFS.h>

#include <Arduino.h>
#include <Servo.h>

#include "./conf.h"

#define TRY_EXEC(func, err, ...) for (int i = 0; i < 10; i++) {\
    if (func(__VA_ARGS__)) break;\
    if (i == 9) fatal_error(err);\
}

void (*resetFunc)(void) = 0;

ESP8266WebServer server(80);
WiFiUDP udp;
NTPClient ntp(udp);
conf_t config;
Servo servo;
int i = 0;

void load_wifi();
void load_server();
void load_config();
void load_ntp();
void save_config();

void logln(const String &data) {
    File f = LittleFS.open("log.txt", "a");
    Serial.println(data);
    f.println(data);
    f.close();
}
void log(const String &data) {
    File f = LittleFS.open("log.txt", "a");
    Serial.print(data);
    f.print(data);
    f.close();
}

void fatal_error(const String &err) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    log("Fatal error ocurred:");
    logln(err);
    logln("Please, check the config. Rebooting in 5 seconds...");
    delay(3000);
    resetFunc();
}

void handle_config_get() {
    String res = String("function getConfig(){return{") +
        "use_ap:" + (config.use_ap ? "true" : "false") +
        ",ap_ssid:'" + config.ap_ssid +
        "',ap_pass:'" + config.ap_pass +
        "',wifi_ssid:'" + config.wifi_ssid +
        "',wifi_pass:'" + config.wifi_pass +
        "',ntp_server:'" + config.ntp_server +
        "',feed_offset:" + config.feed_offset +
        ",feed_duration:" + config.feed_duration +
        ",feed_interval:" + config.feed_interval +
        "}}";
    server.send(200, "application/javascript", res);
}
void handle_config_set() {
    config.use_ap = server.arg("use_ap") == "true";
    config.ap_ssid = server.arg("ap_ssid");
    config.ap_pass = server.arg("ap_pass");
    config.wifi_ssid = server.arg("wifi_ssid");
    config.wifi_pass = server.arg("wifi_pass");
    config.ntp_server = server.arg("ntp_server");
    config.feed_offset = atol(server.arg("feed_offset").c_str());
    config.feed_duration = atol(server.arg("feed_duration").c_str());
    config.feed_interval = atol(server.arg("feed_interval").c_str());
    server.send(200, "text/html", "ok.");
    delay(1000);
    save_config();
}

bool try_wifi() {
    logln("Trying to connect to WiFi...");

    for (int i = 0; i < 10; i++) {
        TRY_EXEC(WiFi.begin, "Couldn't load WiFi.", config.wifi_ssid, config.wifi_pass);

        switch (WiFi.waitForConnectResult(5000)) {
            case WL_WRONG_PASSWORD:
                logln("Wrong password. Please, reconfigure.");
                return false;
            case WL_NO_SSID_AVAIL:
                logln("Wrong SSID. Please, reconfigure.");
                return false;
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
                logln("Connection failed.");
                return false;
            case WL_CONNECTED:
                logln("Connected, got IP " + WiFi.localIP().toString() + ".");
                return true;
            default:
                break;
        }

        log("Timed out (");
        log(String(i + 1));
        logln("/20)...");
    }

    return false;
}

void load_ntp() {
    logln("Loading NTP client...");
    ntp.setPoolServerName(config.ntp_server.c_str());
    ntp.begin();
    for (int i = 0; i < 10; i++) {
        if (ntp.forceUpdate()) return;
    }
    logln("Unable to retrieve time, defaulting to Jan 1th, 1970th.");
}
void load_wifi() {
    if (!config.use_ap) {
        if (try_wifi()) return;
        logln("WiFi didn't work, falling back to AP...");
    }

    logln("Loading WiFi access point...");
    logln("WARNING: Due to this, a connection to an NTP server won't be possible.");
    TRY_EXEC(WiFi.softAPConfig, "Couldn't load access point.", { 10, 0, 0, 1 }, { 10, 0, 0, 1 }, { 255, 0, 0, 0 });
    TRY_EXEC(WiFi.softAP, "Access point couldn't be started.", config.ap_ssid, config.ap_pass);
}
void load_server() {
    logln("Loading HTTP server...");

    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/logs", LittleFS, "/log.txt");
    server.on("/config.js", HTTP_GET, handle_config_get);
    server.on("/", HTTP_POST, handle_config_set);

    server.begin();
}
void load_config() {
    logln("Loading config...");
    config.load(LittleFS, "/config.txt");
}

void save_config() {
    logln("Saving config...");
    config.save(LittleFS, "/config.txt");
    logln("Restarting device, in order to apply changes...");
    resetFunc();
}

void setup() {
    servo.attach(D1, 0, 16);
    analogWrite(D1, 0);

    delay(1000);
    Serial.begin(9600);
    Serial.println();

    Serial.println("Serial connection established!");

    logln("Loading LittleFS...");
    TRY_EXEC(LittleFS.begin, "LittleFS couldn't be loaded.");

    load_config();
    load_wifi();
    load_ntp();
    load_server();
}

void loop() {
    if (config.feed_interval != 0 && (ntp.getEpochTime() - config.feed_offset) % config.feed_interval == 0) {
        log("Feeding (current time ");
        log(ntp.getFormattedTime());
        logln(")...");
        analogWrite(D1, 2);
        delay(config.feed_duration);
        analogWrite(D1, 0);
        delay(1000 - config.feed_duration);
    }
    i++;
    // if (i == 1000) {
    //     i = 0;
    // }
    server.handleClient();
}
