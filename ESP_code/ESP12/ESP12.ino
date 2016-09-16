#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>        
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ArduinoOTA.h> 
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <TimeLib.h> 

const char* mqtt_server = "54.191.93.50";
String topicLuz = "";
String estado = "";
String ID = "";
String Now = "";
byte flag1=0;

WiFiClient espClient;
PubSubClient client(espClient);
IPAddress timeServer(132, 163, 4, 101);
const int timeZone = 0;
WiFiUDP Udp;
unsigned int localPort = 8888;

void callback(char* topic, byte* payload, unsigned int length) {
  Now = String(now());
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
    if(flag1==1){
      Now = "1." + String(now());
      client.publish(estado.c_str(), "on", true);
      client.publish("Tempo", Now.c_str());
      flag1--;
    }else{
      Now = "0." + String(now());
      client.publish(estado.c_str(), "off", true);
      client.publish("Tempo", Now.c_str());
      flag1++;
    }
    
  }
}

void reconectar() {
  while (!client.connected()) {
    if (client.connect(ID.c_str())) {
      client.subscribe(topicLuz.c_str());
      client.publish("ChipID", ID.c_str());
    } else {
      delay(5000);
    }
  }
}

void setup() {
  topicLuz = "Luz/"+ String( ESP.getChipId() );
  estado = "EstadoLuz/"+ String( ESP.getChipId() );
  ID = String( ESP.getChipId() );
 
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  ArduinoOTA.begin();
  client.setServer(mqtt_server, 1234);
  client.setCallback(callback);
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
}

void loop() {
  
  if (!client.connected()) {
    reconectar();
  }
  ArduinoOTA.handle();
  client.loop();
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

