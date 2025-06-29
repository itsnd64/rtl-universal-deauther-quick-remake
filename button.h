#pragma once
#include <Arduino.h>

bool deauth, csa, wpa2;
unsigned long lastDebounceTime = 0, lastDebounceTime1 = 0, lastDebounceTime2 = 0, lastDebounceTime3 = 0;

void init_buttons() {
  pinMode(PA27, INPUT_PULLUP);
  pinMode(PA12, INPUT_PULLDOWN);
  pinMode(PA13, INPUT_PULLDOWN);
  pinMode(PA14, INPUT_PULLDOWN);
}

void attach_buttons() { // idk w name to use
  attachInterrupt(PA27, []() {
    unsigned long currentTime = millis();
    if ((currentTime - lastDebounceTime ) > 250) {
      channel_index = (channel_index + 1) % channels.size();
      wext_set_channel(WLAN0_NAME, channels[channel_index]);
      lastDebounceTime = currentTime;
    }
  }, FALLING);
  attachInterrupt(PA12, []() {
    unsigned long currentTime = millis();
    if ((currentTime - lastDebounceTime1) > 800) {
      deauth = !deauth;
      lastDebounceTime1 = currentTime;
    }
  }, RISING);
  attachInterrupt(PA13, []() {
    unsigned long currentTime = millis();
    if ((currentTime - lastDebounceTime2) > 800) {
      csa = !csa;
      lastDebounceTime2 = currentTime;
    }
  }, RISING);
  attachInterrupt(PA14, []() {
    unsigned long currentTime = millis();
    if ((currentTime - lastDebounceTime3) > 800) {
      wpa2 = !wpa2;
      lastDebounceTime3 = currentTime;
    }
  }, RISING);
}
