/*MIT License

Copyright (c) 2019 Friendly River LLC, Dimitrios F. Kallivroussis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>       // this is needed for display
#include <Adafruit_ILI9341.h>
#include <Wire.h>      // this is needed for FT6206
#include <Adafruit_FT6206.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>

int tileColor;
int textColor;
unsigned long counter = 0;
int j = 1;
int jj = 1;
int jjj = 1;

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206();

// The display also uses hardware SPI, plus #D8 & #D0
#define TFT_CS D8
#define TFT_DC D0
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define BOXSIZE_X 115
#define BOXSIZE_Y 100
#define SPACE 10
#define NUMCLIENTS 6

struct position {
  int ID;
  char Name[8];
  int Postion_x;
  int Postion_y;
  int Status; //0=offline, 1=online, 2=activated, 3=acknowledged
};

struct position positions[NUMCLIENTS] = { //Change names to suit your needs
  {1, "Name 1", 0, 0, 9},
  {2, "Name 2", SPACE + BOXSIZE_X, 0, 9},
  {3, "Name 3", 0, SPACE + BOXSIZE_Y, 9},
  {4, "Name 4", SPACE + BOXSIZE_X, SPACE + BOXSIZE_Y, 9},
  {5, "Name 5", 0, 2 * (SPACE + BOXSIZE_Y), 9},
  {6, "Name 6", SPACE + BOXSIZE_X, 2 * (SPACE + BOXSIZE_Y), 9}
};

int client = 0;

#define STASSID "YOUR_SSID" //Change this to your SSID
#define STAPSK  "YOUR_PSK"  //Change this to your WLANs preshared key

unsigned int localPort = 8888;      // local port to listen on
unsigned int remotePort = 9999;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[9] = "whatever";       //buffer to hold the commands

WiFiUDP Udp;
IPAddress multicast_ip_addr(224, 0, 0, 32);


void setup(void) {
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
  tft.begin();
  if (! ctp.begin(255)) {  // pass in 'sensitivity' coefficient
    while (1);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Udp.beginMulticast(WiFi.localIP(), multicast_ip_addr, remotePort);
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextSize(1);
  for (int i = 1; i <= NUMCLIENTS; ++i) {
    updateStatus(i, checkStatus(i, 3));
  }
}

void loop() {
  if (ctp.touched()) {
    counter = 0;
    if (digitalRead(D3) == HIGH) {
      TS_Point p = ctp.getPoint();
      // flip it around to match the screen.
      p.x = map(p.x, 0, 240, 240, 0);
      p.y = map(p.y, 0, 320, 320, 0);
      if (p.y <= positions[0].Postion_y + BOXSIZE_Y && p.x <= positions[0].Postion_x + BOXSIZE_X) {
        onTouch(0);
      } else if (p.y <= positions[1].Postion_y + BOXSIZE_Y && p.x <= positions[1].Postion_x + BOXSIZE_X) {
        onTouch(1);
      } else if (p.y <= positions[2].Postion_y + BOXSIZE_Y && p.x <= positions[2].Postion_x + BOXSIZE_X) {
        onTouch(2);
      } else if (p.y <= positions[3].Postion_y + BOXSIZE_Y && p.x <= positions[3].Postion_x + BOXSIZE_X) {
        onTouch(3);
      } else if (p.y <= positions[4].Postion_y + BOXSIZE_Y && p.x <= positions[4].Postion_x + BOXSIZE_X) {
        onTouch(4);
      } else if (p.y <= positions[5].Postion_y + BOXSIZE_Y && p.x <= positions[5].Postion_x + BOXSIZE_X) {
        onTouch(5);
      }
    } else {
      digitalWrite(D3, HIGH);
      while (ctp.touched()) delay(10);
    }
  }
  if (counter % 1000 == 0) {
    if (positions[j - 1].Status == 1 || positions[j - 1].Status == 3) updateStatus(positions[j - 1].ID, checkStatus(positions[j - 1].ID, 4));
    ++j;
    if (j > NUMCLIENTS) j = 1;
  }
  if (counter % 100 == 0) {
    if (positions[jj - 1].Status == 2) updateStatus(positions[jj - 1].ID, checkStatus(positions[jj - 1].ID, 4));
    ++jj;
    if (jj > NUMCLIENTS) jj = 1;
  }
  if (counter % 1000 == 0) {
    if (positions[jjj - 1].Status == 0) updateStatus(positions[jjj - 1].ID, checkStatus(positions[jjj - 1].ID, 1));
    ++jjj;
    if (jjj > NUMCLIENTS) jjj = 1;
  }
  ++counter;
  if (counter > 100000) digitalWrite(D3, LOW);
}

boolean updateStatus(int ID, int Status) {
  if (positions[ID - 1].Status != Status) {
    if (Status == 1) {
      tft.fillRect(positions[ID - 1].Postion_x, positions[ID - 1].Postion_y, BOXSIZE_X, BOXSIZE_Y, ILI9341_BLUE);
      tft.setCursor(positions[ID - 1].Postion_x + SPACE, positions[ID - 1].Postion_y + 55);
      tft.setTextColor(ILI9341_WHITE);
      tft.print(positions[ID - 1].Name);
      positions[ID - 1].Status = 1;
    } else if (Status == 2) {
      tft.fillRect(positions[ID - 1].Postion_x, positions[ID - 1].Postion_y, BOXSIZE_X, BOXSIZE_Y, ILI9341_RED);
      tft.setCursor(positions[ID - 1].Postion_x + SPACE, positions[ID - 1].Postion_y + 55);
      tft.setTextColor(ILI9341_WHITE);
      tft.print(positions[ID - 1].Name);
      positions[ID - 1].Status = 2;
    } else if (Status == 0) {
      tft.fillRect(positions[ID - 1].Postion_x, positions[ID - 1].Postion_y, BOXSIZE_X, BOXSIZE_Y, ILI9341_DARKGREY);
      tft.setCursor(positions[ID - 1].Postion_x + SPACE, positions[ID - 1].Postion_y + 55);
      tft.setTextColor(ILI9341_BLACK);
      tft.print(positions[ID - 1].Name);
      positions[ID - 1].Status = 0;
    } else if (Status == 3) {
      tft.fillRect(positions[ID - 1].Postion_x, positions[ID - 1].Postion_y, BOXSIZE_X, BOXSIZE_Y, ILI9341_DARKGREEN);
      tft.setCursor(positions[ID - 1].Postion_x + SPACE, positions[ID - 1].Postion_y + 55);
      tft.setTextColor(ILI9341_WHITE);
      tft.print(positions[ID - 1].Name);
      positions[ID - 1].Status = 3;
    }
    return (true);
  } else {
    return (false);
  }
}

int checkStatus(int ID, int maxGrace) {
  char * pch;
  int ID_packet, Status_packet, Status, Packets, gracePeriod, gracePeriodSuperior;
  gracePeriodSuperior = 0;
  do {
    Udp.beginPacketMulticast(multicast_ip_addr, localPort, WiFi.localIP());
    sprintf(ReplyBuffer, "%d,2\n", ID);
    Udp.write(ReplyBuffer);
    Udp.endPacket();
    Status = 0;
    gracePeriod = 0;
    while (!Udp.parsePacket() && gracePeriod < 4) {
      delay(30);
      ++gracePeriod;
    }
    // read the packet into packetBufffer
    while (Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE)) {
      pch = strtok (packetBuffer, ",");
      ID_packet = atoi(pch);
      pch = strtok (NULL, ",");
      Status_packet = atoi(pch);
      if (ID == ID_packet) Status = Status_packet;
      Udp.flush();
    }
    ++gracePeriodSuperior;
  } while (Status == 0 && gracePeriodSuperior < maxGrace);
  return (Status);
}

void onTouch(int i) {
  int targetStatus, effectiveStatus, command;
  if (positions[i].Status > 1) {
    targetStatus = 1;
    command = 0;
  } else {
    targetStatus = 2;
    command = 1;
  }
  do {
    Udp.beginPacketMulticast(multicast_ip_addr, localPort, WiFi.localIP());
    sprintf(ReplyBuffer, "%d,%d\n", positions[i].ID, command);
    Udp.write(ReplyBuffer);
    Udp.endPacket();
    effectiveStatus = checkStatus(positions[i].ID, 3);
  } while ( effectiveStatus != targetStatus && effectiveStatus != 0);
  updateStatus(positions[i].ID, effectiveStatus);
  while (ctp.touched()) {
    delay(1);
  }
}
