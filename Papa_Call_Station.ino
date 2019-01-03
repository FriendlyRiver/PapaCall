#define ID 1 //Change This to the Stations ID
#define SENSITIVITY 100

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

#define PIN            D5
#define NUMPIXELS      32

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);



#define STASSID "YOUR_SSID" //Change this to your SSID
#define STAPSK  "YOUR_PSK"  //Change this to your WLANs preshared key


unsigned int localPort = 8888;      //local port to listen on
unsigned int remotePort = 9999;     //remote port for the unicast reply
int Status = 1;
int id, command;
int j = 0;
unsigned long counter = 0;
boolean flash = false;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[9] = "whatever";       //buffer to hold the status reply
char * pch;

WiFiUDP Udp;
IPAddress multicast_ip_addr(224, 0, 0, 32);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D7, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  Udp.beginMulticast(WiFi.localIP(), multicast_ip_addr, localPort);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    IPAddress remote = Udp.remoteIP();
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    pch = strtok (packetBuffer, ",");
    id = atoi(pch);
    pch = strtok (NULL, ",");
    command = atoi(pch);

    if (id == ID) {
      if (command == 1) {
        Status = 2;

      } else if (command == 0) {
        Status = 1;
      } else if (command == 2) {
        Udp.beginPacket(Udp.remoteIP(), remotePort);
        sprintf(ReplyBuffer, "%d,%d\n", id, Status);
        Udp.write(ReplyBuffer);
        Udp.endPacket();
      }
      memset(packetBuffer, 0, sizeof(packetBuffer));
    }
  }
  delay(10);

  if (Status == 2) {
    digitalWrite(LED_BUILTIN, LOW);
    if (counter % 10 == 0) {
      flash = not(flash);
      if (flash) {
        for (int i = 0; i < NUMPIXELS; ++i) {
          strip.setPixelColor(i, 255, 255, 255);
        }
      } else {
        for (int i = 0; i < NUMPIXELS; ++i) {
          strip.setPixelColor(i, 0, 0, 0);
        }
      }
      strip.show();
    }
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    for (int i = 0; i < NUMPIXELS; ++i) {
      strip.setPixelColor(i, 0, 0, 0);
    }
    strip.show();
  }
  if (analogRead(A0) < SENSITIVITY && Status == 2) Status = 3;
  ++counter;
}
