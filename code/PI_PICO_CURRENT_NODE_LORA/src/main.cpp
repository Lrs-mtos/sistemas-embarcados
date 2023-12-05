#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <SPI.h>            
#include <LoRa.h>


// Define o tamanho da janela do filtro
#define WINDOW_SIZE 30

/*
 Pinos para comunicação LoRa
*/
#define nss 8
#define rst 9
#define dio0 7

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

int maxValue = -4096;
float multiplier = 0;
int16_t readValue = 0; // value read from the sensor
unsigned long stopTime;
float current;
// Crie um array para armazenar os valores do ADC
int16_t adcValues[WINDOW_SIZE] = {0};

// Crie uma variável para armazenar o índice atual do array
int i = 0;

// Crie uma variável para armazenar a soma dos valores do array
int32_t sum = 0;

// Crie uma variável para armazenar a média dos valores do array
float average = 0;

/*
 Variaveis para comunicação LoRa
*/
byte MasterNode = 0xFF;     
byte Node2 = 0xCC; 
int pot2=26;
String SenderNode = "";
String outgoing;              // outgoing message
String message1;
String message2; 
byte msgCount = 0;            // count of outgoing messages
 
// Tracks the time since last event fired
unsigned long previousMillis=0;
unsigned long int previoussecs = 0; 
unsigned long int currentsecs = 0; 
unsigned long currentMillis = 0;
int Secs = 0; 
 
unsigned long lastSendTime = 0;        // last send time
int interval = 20;          // interval between sends 

void sendMessage(String outgoing, byte MasterNode, byte otherNode) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(otherNode);              // add destination address
  LoRa.write(MasterNode);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void multiplierFit() {
  if (average < 100)
    multiplier = 0.00040;
  else if (average < 500)
    multiplier = 0.0013955343;
  else if (average < 900)
    multiplier = 0.0030189873;
  else
    multiplier = 0.0035; //precisamos calcular ainda    
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 0.015625mV/ADS1115)");

  ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.015625mV

  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }


  // Start continuous conversions.
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);

  LoRa.setPins(nss, rst, dio0);

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    digitalWrite(LED_BUILTIN, LOW);
    while (true);                       // if failed, do nothing
  }
 
 Serial.println("LoRa init succeeded.");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Setup completed.");
}


void loop() {
  // Ler o valor do ADC e armazena no array
  stopTime = millis() + 1000; // mark stop time
  maxValue = -4096;
  while (millis() < stopTime) { // sample for one second
    readValue = ads.getLastConversionResults();
    readValue = abs(readValue);
    if(readValue > maxValue) maxValue = readValue;
  }
  adcValues[i] = maxValue;

  // Atualiza a soma dos valores do array
  sum = sum + adcValues[i];

  // Incrementa o índice do array
  i++;

  // Se o índice do array for igual ao tamanho da janela, reinicia para zero
  if (i == WINDOW_SIZE) {
    i = 0;
  }

  // Subtrai o valor do array que foi substituído pelo novo valor do ADC
  sum = sum - adcValues[i];

  // Calcula a média dos valores do array
  average = (float)sum / WINDOW_SIZE;

  multiplierFit();

  current = average * multiplier; // calculate/calibrate/change multiplication factor
  float current_mA = current * 1000;
  // Imprime a média e a corrente no monitor serial
  Serial.print("Average: "); Serial.print(average); 
  Serial.print(" | Multiplier: "); Serial.print(multiplier, 5);
  Serial.print(" | Current: "); Serial.print(current_mA); Serial.print(" mA");
  message2 = String(current_mA); 
  sendMessage(message2,MasterNode,Node2);
  Serial.println(" | Sending: " + message2);
  message2 = "";
  

}
