#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>              
#include <LoRa.h>

#define LED_LORA_OFF 27
#define LED_LORA_ON 26

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte MasterNode = 0xFF;     
byte Node2 = 0xCC;
int current;
boolean syncData = true;
// Definindo as portas usadas pelo modulo receptor
#define nss 5    // Definindo o pino SS (Selecao de Escravo)
#define rst 4  // Definindo o pino de reset
#define dio0 2   // Definindo o pino DIO0 (Interrupcao de recepcao)

// Update these with values suitable for your network.

const char* ssid = "Galaxy_A14";
const char* password = "hugo1234";
const char* mqtt_server = "192.168.105.135";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(27, LOW);   
    // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(27, HIGH);  
    // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_lora() {
  pinMode(LED_LORA_OFF, OUTPUT);
  pinMode(LED_LORA_ON, OUTPUT);
  digitalWrite(LED_LORA_OFF, HIGH);
  digitalWrite(LED_LORA_ON, LOW);

  LoRa.setPins(nss, rst, dio0);

  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  digitalWrite(LED_LORA_OFF, LOW);
  digitalWrite(LED_LORA_ON, HIGH);

  Serial.println("LoRa init succeeded.");
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
 
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

boolean onReceive(int packetSize) {
  if (packetSize == 0) {
    return false;          // if there's no packet, return
  }
  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length
 
  String incoming = "";
 
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
 
  if (incomingLength != incoming.length()) {   // check length for error
   Serial.println("error: message length does not match length");
   ;
    return false;                             // skip rest of function
  }
 
  // if the recipient isn't this device or broadcast,
  if (recipient != Node2 && recipient != MasterNode) {
    Serial.println("This message is not for me.");
    ;
    return false;                             // skip rest of function
  }
   // Serial.println(incoming);
   // int Val = incoming.toInt();
   String q = getValue(incoming, ',', 0); 
  current = q.toInt();
  
  Serial.printf("Valor recebido: %d \n", current);
  incoming="";
  delay(3000);
  return true;
}

boolean sendData() {
  if (!client.connected()) {
      reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 2000) {
      lastMsg = now;
      //++value;
      //snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);

      StaticJsonDocument<48> doc;

      doc["Dispositivo"] = "ESP32";
      doc["Corrente"] = current;

      String output;

      serializeJson(doc, output);

      Serial.print("Publish message: ");
      Serial.println(output);
      client.publish("dataOut", output.c_str());
      return true;
    } 
    return false;
}

void setup() {
  Serial.begin(115200);
  setup_lora();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if(onReceive(LoRa.parsePacket())) {
    if(!sendData()) {
      Serial.println("Erro na comunicacao LoRa/MQTT");
    }
  }
  delay(100);
}