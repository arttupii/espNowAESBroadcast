#include"wifi802_11.h"

#ifdef ESP32
#include <WiFi.h>
#include "esp_wifi.h"
#else
#include <ESP8266WiFi.h>
#include <user_interface.h>
#endif

const char *ssid = "MESH_NETWORK";
char wifi_password[20];

#define BEACON_SSID_OFFSET 38
#define SRCADDR_OFFSET 10
#define BSSID_OFFSET 16
#define MY_MAC_OFFSET 10
#define SEQNUM_OFFSET 22
#define DATA_START_OFFSET 24

uint8_t raw_HEADER[] = {
  //MAC HEADER
  0x40, 0x0C,             // 0-1: Frame Control  //Version 0 && Data Frame && MESH
  0x00, 0x00,             // 2-3: Duration
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff,       // 4-9: Destination address (broadcast)
  0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,       // 10-15: Source address
  0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,       // 16-21: BSSID
  0x00, 0x00             // 22-23: Sequence / fragment number
};
short sequence = 0;
void(*wifi_802_receive_callback)(const uint8_t *, int, uint8_t) = NULL;

#ifdef ESP32
void receive_raw_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type) {
  if (!memcmp(station_info->bssid, bsid, sizeof(station_info->bssid))) {
     return;
  }
  wifi_promiscuous_pkt_t *sniffer = (wifi_promiscuous_pkt_t *)recv_buf;

  if(sniffer->payload[0]!=0x40) return;
//  if(memcmp(sniffer->payload+BSSID_OFFSET,raw_HEADER+BSSID_OFFSET, 6)!=0) return;
  short length = sniffer->payload[0])
  //hexDump(sniffer->payload, 100);
  wifi_802_receive_callback(sniffer->payload+2, )
  Serial.print("Channel:");Serial.println(sniffer->rx_ctrl.channel);
  Serial.print("RSSI:");Serial.println(sniffer->rx_ctrl.rssi);
  return;
}
#else
void receive_raw_cb(unsigned char*frm, short unsigned int len) {
  /*
    16:34:05.795 ->            C3 10 26 50 00 00 00 00 00 00 01 00 [40 0C 00 00 __&P________@___
    16:34:05.795 ->            FF FF FF FF FF FF BA DE AF FE 00 [06] BA DE AF FE ________________
    16:34:05.795 ->            00 06 00 00 48 65 6C 6C 6F 20 31 35 32 37 00 00 ____Hello_1527__
    16:34:05.828 ->            00 00 01 08 8B 96 82 84 0C 18 30 60 03 01 01 05 __________0`____
    16:34:05.828 ->            05 01 02 00 00 00 07 06 43 4E 00 01 0D 14 2A 01 ________CN____*_
    16:34:05.828 ->            00 32 04 6C 12 24 48 30 18 01 00 00 0F AC 02 02 _2_l_$H0________
    16:34:05.828 ->            00 00 0F AC ____
    16:34:05.828 ->                    Length: 100
  */
  uint8_t rssi = frm[0];
  //if(frm[0]!=0x40) return;
  struct wifi_promiscuous_pkt_t *p = (struct wifi_promiscuous_pkt_t *)frm;

  if(frm[12]!=0x40!=0) return;
  if(memcmp(frm+BSSID_OFFSET+12,raw_HEADER+BSSID_OFFSET, 6)!=0) return;
  unsigned char *d = frm+12+DATA_START_OFFSET;

  short length = ((unsigned short)d[0])<<8 | d[1];

  if(wifi_802_receive_callback!=NULL) {
    wifi_802_receive_callback(d+2, length, length);
  }
}
#endif

void wifi_802_11_begin(char bsId[], int channel){
  //WiFi.begin();
  char password[15];
  for(int i=0;i<sizeof(password);i++) {
    #ifdef ESP32
    char r = (esp_random()%('z'-' ')) + ' ';
    #else
    char r = random(' ','z');
    #endif
    password[i] = r;
  }
  password[sizeof(password)-1]=0;

  String mac = WiFi.macAddress();
  memcpy(raw_HEADER+BSSID_OFFSET, bsId, 6);
  memcpy(raw_HEADER+MY_MAC_OFFSET, mac.c_str(), 6);

  WiFi.softAP(ssid, password, 1, true);

  #ifdef ESP32
  esp_wifi_set_promiscuous_rx_cb(receive_raw_cb);
  esp_wifi_set_promiscuous(1);
  #else
  wifi_set_opmode(0x1);
  wifi_set_channel(1);
  wifi_set_promiscuous_rx_cb(receive_raw_cb);
  wifi_promiscuous_enable(true);
  #endif
}

void wifi_802_receive_cb(void(*cb)(const uint8_t *, int, uint8_t)) {
    wifi_802_receive_callback = cb;
}

void wifi_802_11_send(const uint8_t *d, int len) {
  uint8_t buf[300];
  if(len>sizeof(buf)-sizeof(raw_HEADER)-2) return;

  memcpy(buf,raw_HEADER, sizeof(raw_HEADER));
  memcpy(buf+SEQNUM_OFFSET,(char*)&sequence, 2);
  memcpy(buf+sizeof(raw_HEADER)+2, d, len);

  buf[sizeof(raw_HEADER)]=(len>>8)&0xff;
  buf[sizeof(raw_HEADER)+1]=len&0xff;

  #ifdef ESP32
  esp_wifi_80211_tx(WIFI_IF_AP, buf, sizeof(raw_HEADER) + len+ 2, false);
  #else
  wifi_send_pkt_freedom(buf, sizeof(raw_HEADER) + len+ 2, false);
  #endif
  sequence++;

}