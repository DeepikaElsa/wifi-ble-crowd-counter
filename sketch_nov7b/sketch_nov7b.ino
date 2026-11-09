#include <WiFi.h>
#include "esp_wifi.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define MAX_DEVICES 1500
#define WIFI_SCAN_MS 15000
#define BLE_SCAN_S 15
#define HOP_MS 300
#define RSSI_MIN -85

uint32_t wSeen[MAX_DEVICES]; volatile int wCount = 0; volatile long wRssi = 0;
uint32_t bSeen[MAX_DEVICES]; int bCount = 0; long bRssi = 0;

uint32_t hashMac(const uint8_t *mac) {
  uint32_t h = 2166136261u ^ 0xA5A5;
  for (int i = 0; i < 6; i++) { h ^= mac[i]; h *= 16777619u; }
  return h;
}

void sniffer(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  if (pkt->rx_ctrl.rssi < RSSI_MIN) return;
  const uint8_t *p = pkt->payload;
  if (p[0] != 0x40) return;
  uint32_t h = hashMac(p + 10);
  for (int i = 0; i < wCount; i++) if (wSeen[i] == h) return;
  if (wCount < MAX_DEVICES) { wSeen[wCount++] = h; wRssi += pkt->rx_ctrl.rssi; }
}

class BleCb : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) {
    if (d.getRSSI() < RSSI_MIN) return;
    uint32_t h = hashMac(*d.getAddress().getNative());
    for (int i = 0; i < bCount; i++) if (bSeen[i] == h) return;
    if (bCount < MAX_DEVICES) { bSeen[bCount++] = h; bRssi += d.getRSSI(); }
  }
};

BLEScan *scan;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_promiscuous_filter_t filt;
  filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);

  BLEDevice::init("");
  scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new BleCb(), true);
  scan->setActiveScan(false);
  scan->setInterval(100);
  scan->setWindow(99);

  Serial.println("wifi_count,ble_count,wifi_rssi,ble_rssi");
}

void loop() {
  wCount = 0; wRssi = 0; bCount = 0; bRssi = 0;

  // Phase 1: WiFi
  esp_wifi_set_promiscuous(true);
  int ch = 1;
  unsigned long start = millis(), lastHop = 0;
  while (millis() - start < WIFI_SCAN_MS) {
    if (millis() - lastHop >= HOP_MS) {
      ch = ch % 13 + 1;
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      lastHop = millis();
    }
    delay(10);
  }
  esp_wifi_set_promiscuous(false);

  // Phase 2: Bluetooth
  scan->start(BLE_SCAN_S, false);
  scan->clearResults();

  int wAvg = wCount ? wRssi / wCount : 0;
  int bAvg = bCount ? bRssi / bCount : 0;
  Serial.printf("%d,%d,%d,%d\n", wCount, bCount, wAvg, bAvg);
}