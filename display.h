#pragma once // useless again
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "idk.h"
#include "wifi_stuffs.h"
#include "button.h"

using namespace std;

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void drawCentreString(const char *buf) {
  int16_t x1, y1;
  uint16_t w, h;
  int y = display.getCursorY();
  display.getTextBounds(buf, 0, y, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2 - x1, y);
  display.println(buf);
}

void drawCentreString(const String &buf) {
  int16_t x1, y1;
  uint16_t w, h;
  int y = display.getCursorY();
  display.getTextBounds(buf, 0, y, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2 - x1, y);
  display.println(buf);
}


void init_display() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextWrap(false);
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1);
}

void drawIntro() {
  display.clearDisplay();
  display.setCursor(0, 0);
  drawCentreString("itsnd64 Present:");
  drawCentreString("Universal deauther");
  drawCentreString("WPA2 + WPA3");
  drawCentreString("2.4GHz + 5GHz");
  display.display();
}

void scan() { // argggggg i cant use display in wifi_stuffs.h bc i literally include it "in circle",too lazy to properly fix like my project soooooo
  display.print("Scanning");
  wifi_scan_networks([](rtw_scan_handler_result_t *r) -> rtw_result_t {
    if (r->scan_complete) return RTW_SUCCESS;
    auto *rec = &r->ap_details;
    rec->SSID.val[rec->SSID.len] = 0;

    Mac bssid;
    memcpy(bssid.data(), rec->BSSID.octet, 6);
    for (auto &ap : AP_Map) if (ap.bssid == bssid) return RTW_SUCCESS;

    AP_Info info;
    const char* prefix = (rec->security == RTW_SECURITY_WPA3_AES_PSK) ? "W3_" : (rec->security == RTW_SECURITY_WPA2_WPA3_MIXED) ? "W23_" : "";
    const char* ssid = (rec->SSID.len == 0 || rec->SSID.val[0] == 0) ? "<Hidden>" : (char*)rec->SSID.val;
    snprintf(info.ssid, sizeof(info.ssid), "%s%.*s", prefix, (int)(32 - strlen(prefix)), ssid);
    info.isWPA3 = prefix != ""; 
    info.bssid = bssid;
    info.channel = rec->channel;
    info.rssi = rec->signal_strength;

    AP_Map.push_back(info);
    if (find(channels.begin(), channels.end(), rec->channel) == channels.end()) channels.push_back(rec->channel);
    return RTW_SUCCESS;
  }, nullptr);

  for (uint8_t i = 8; AP_Map.empty() && i--;) {
    display.print(".");
    display.display();
    delay(1000);
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  if (activeDeauthMode) display.println("Active Deauth Mode!!!");
  if (AP_Map.empty()) {
    printf("No AP found\n");
    display.println("No AP found\n");
  }
  else for (size_t i = 0; i < AP_Map.size(); ++i) {
    const auto &ap = AP_Map[i];
    // char a[100];
    // snprintf(a, sizeof(a), "%-3d|%s", ap.rssi, ap.ssid);
    // display.println(a);
    display.print(ap.rssi);
    display.print("|");
    display.println(ap.ssid);
  }
  display.display();

  if (activeDeauthMode) return; // quick code
  while (!digitalRead(PA27)) delay(50);
  while  (digitalRead(PA27)) delay(50);
}

void draw() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Ch ");
  display.print(channels[channel_index]);
  display.println(" " + String(deauth ? "Deauth " : "") + String(csa ? "CSA " : "") + String(wpa2 ? "WPA2 " : ""));

  for (auto it = AP_Map.begin(); it != AP_Map.end(); ++it) if (it->channel == channels[channel_index]) display.println((it->beaconFound ? "F|" : "") + String(it->ssid)); // too lazy to use strcat

  display.display();
}
