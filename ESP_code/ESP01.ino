#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>        
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <Esp.h>

const char* mqtt_server = "54.191.93.50";
String topicLuz = "";
String estado = "";
String ID = "";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {

  delay(10);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

}

void callback(char* topic, byte* payload, unsigned int length) {
  
  if ((char)payload[0] == 'O' && (char)payload[1] == 'n') {
    digitalWrite(0, LOW);
    client.publish(estado.c_str(), "on", true); // true=retined
  } else if ((char)payload[0] == 'O' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {
    digitalWrite(0, HIGH);
    client.publish(estado.c_str(), "off", true); 
  }
}

void reconectar() {
  while (!client.connected()) {
    if (client.connect(ID.c_str())) {
      client.subscribe(topicLuz.c_str());
      client.publish(topicLuz.c_str(), "On", true);
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
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  setup_wifi();
  client.setServer(mqtt_server, 1234);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconectar();
  }
  client.loop();
}
