#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Vključitev potrebnih knjižnic za delovanje

// Podatki za povezavo z WiFi
const char* ssid = "********";      // Ime WiFi omrežja
const char* password = "*********";  // Geslo WiFi omrežja

// Nastavitve za WebSocket
WebSocketsClient webSocket;           // Objekt za sprejemanje podatkov
WebSocketsClient webSocketSend;       // Objekt za pošiljanje podatkov
const char* websocket_server = "192.168.151.144"; // IP naslov WebSocket strežnika
int websocket_port = 8811;            // Vrata, na katerih deluje WebSocket strežnik za ESP32
int portNumber = 8888;                // Dodatna vrata za WebSocket povezave

int vrednostPotenciometra = 0;

String dataString;                    // Niz za shranjevanje podatkov

// Določanje pinov
const int PIN5 = 5;                   // Pin 5 za LED
const int PHOTO_PIN = 34;             // Pin za fotoupornik

bool lastLightState = LOW;            // Spremljanje prejšnjega stanja svetlobe

const int  SERVO_PIN = 32;  // Določitev PWM pina za servomotor
const int  SERVO_CHN = 0;   // Kanal PWM za servomotor
const int  SERVO_FRQ = 50;  // Frekvenca PWM za servomotor
const int  SERVO_BIT = 12;  // Bitna globina PWM signala
const int  ADC_PIN = 35;  // Analogni vhodni pin potenciometra

void servo_set_pin(int pin);          // Funkcija za nastavitev servomotorja na pin
void servo_set_angle(int angle);      // Funkcija za nastavitev kota servomotorja

void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length){
  Serial.println("Dogodek na WebSocketu");
  Serial.println(type);
  switch(type){
    case WStype_TEXT:
      {
        //<256> določa kapaciteto objekta
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if(error){
          Serial.print(F("Napaka pri deserializaciji JSON"));
          Serial.println(error.c_str());
          return;
        }

        char tip[40];
        memset(tip, 0, 40); // Pobrišemo vrednosti iz znakovnega niza tip
        strcpy(tip, doc["tipSporočila"]); // Kopiramo tipSporočila v tip {LED}

        if(strcmp(tip, "LED")==0){
          Serial.println("Ukaz je LED");
          int vrednost = doc["vrednost"];
          String pin = doc["pin"];
          if(pin == "PIN5"){
            Serial.println("PIN5");
            if(vrednost == 0){ digitalWrite(PIN5, LOW); Serial.println("Ugasni PIN5");}
            else if (vrednost == 1){ digitalWrite(PIN5, HIGH); Serial.println("Prižgi PIN5");}
          }
          else if(pin == "PHOTO_PIN"){
            Serial.println("PHOTO_PIN");
          }
          else{
            Serial.println("Ukaz ni prepoznan");
          }
          break;
        }
      }
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);                // Začetek komunikacije na 115200 baud
  pinMode(PIN5, OUTPUT);               // Nastavitev pina PIN5 kot izhod
  pinMode(PHOTO_PIN, INPUT);           // Nastavitev pina PHOTO_PIN kot vhod
  servo_set_pin(SERVO_PIN);            // Nastavitev servomotorja na določen pin

  WiFi.begin(ssid, password);          // Začetek WiFi povezave
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  // Izpis IP
  Serial.println("Modul esp je povezan!") ;
  Serial.print("IP naslov esp32 modula: ") ;
  Serial.println(WiFi.localIP()) ;
  Serial.print("Številka porta: ");
  Serial.println(portNumber) ;
  Serial.print("WiFi MAC: ");
  Serial.println(WiFi.macAddress());

  webSocket.begin(websocket_server, portNumber); // Začetek WebSocket povezave
  webSocket.onEvent(onWebSocketEvent);
  webSocketSend.begin(websocket_server, websocket_port); // Začetek drugega WebSocket klienta
}

void loop() {
  // Branje analognega vhoda GPIO za merjenje fotoupornika
  int foto_svetloba = analogRead(PHOTO_PIN); // Meritev svetlobne intenzitete
  String message = "LDR: " + String(foto_svetloba); // Sestavljanje sporočila
  Serial.println(message); // Izpis na serijski vmesnik

  // Preberi vrednost potenciometra (vrednost med 0 in 4095)
  int potVal = analogRead(ADC_PIN);              
  Serial.printf("potVal_1: %d\t",potVal); 
  potVal = map(potVal, 0, 4095, 0, 180);  
  servo_set_angle(potVal);                   
  Serial.printf("potVal_2: %d\r\n",potVal);
  delay(15);// Počakaj, da servo doseže položaj

  //branje vrednosti iz potenciometra  
    vrednostPotenciometra = analogRead (ADC_PIN);

  // Ustvarjanje JSON sporočila
  /*StaticJsonDocument<200> doc;
  doc["tipSporocila"] = "potenciometer";
  doc["pin"] = ADC_PIN;
  doc["vrednost"] = vrednostPotenciometra;
  String output;
  serializeJson(doc, output);
  Serial.println(output);*/

  Serial.println(vrednostPotenciometra);

  dataString = R"({"tipSporočila":"potenciometer","pin":34,"vrednost":)";
  dataString = dataString + String(vrednostPotenciometra) + "}"; //R"()" omogoča enostaven vnos niza "literal"

  // Pošiljanje podatkov na websocket
  webSocketSend.sendTXT(dataString);
  webSocket.loop(); // Redno preverjanje in obdelava WebSocket dogodkov
  webSocketSend.loop(); // Redno preverjanje in obdelava WebSocket dogodkov
}

void servo_set_pin(int pin) { 
  ledcSetup(SERVO_CHN, SERVO_FRQ, SERVO_BIT); // Nastavitev PWM parametra
  ledcAttachPin(pin, SERVO_CHN); // Pritrditev pina na PWM kanal
}

void servo_set_angle(int angle) { 
  if (angle > 180 || angle < 0) 
    return; // Preverjanje veljavnosti kota
  long pwm_value = map(angle, 0, 180, 103, 512); // Izračun PWM vrednosti
  ledcWrite(SERVO_CHN, pwm_value); // Nastavitev PWM vrednosti

  int lightValue = analogRead(PHOTO_PIN); // Ponovno branje svetlobne vrednosti
  bool currentLightState = (lightValue > 2900); // Primerjalna vrednost, prilagodite glede na svetlobne pogoje
  if (currentLightState != lastLightState) { // Preverjanje spremembe stanja
    StaticJsonDocument<1024> doc; 
    doc["tipSporocila"] = "Svetloba"; 
    doc["stanje"] = currentLightState ? 1 : 0; 
    String output; 
    serializeJson(doc, output); 
    webSocket.sendTXT(output); // Pošiljanje sporočila o spremembi svetlobe

    lastLightState = currentLightState; // Posodobitev prejšnjega stanja
  }

  delay(1000); // Časovni zamik za zmanjšanje obremenitve procesorja
  webSocket.loop(); // Redno preverjanje in obdelava WebSocket dogodkov
  webSocketSend.loop(); // Redno preverjanje in obdelava WebSocket dogodkov
}
