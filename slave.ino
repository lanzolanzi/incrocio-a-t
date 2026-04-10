//SLAVE

#include <Wire.h>
#define MY_ADDR 0x08

struct SemaforoPedonale { uint8_t pinVerde, pinGiallo, pinRosso, pinBuzzer, pinPulsante; };
SemaforoPedonale pedoni[3] = {{8,9,10,4,A3}, {7,6,5,3,A1}, {12,13,11,2,A0}};

uint8_t pedState[3] = {0,0,0};
volatile uint8_t pendingFase = 0;
volatile bool faseCambiata = false;
uint8_t buttonFlags = 0;

void onReceive(int n) { if(Wire.available()){ pendingFase = Wire.read(); faseCambiata = true; } }
void onRequest() { Wire.write(buttonFlags); buttonFlags = 0; }

void setup() {
  for (int i=0; i<3; i++) {
    pinMode(pedoni[i].pinVerde, OUTPUT); pinMode(pedoni[i].pinGiallo, OUTPUT);
    pinMode(pedoni[i].pinRosso, OUTPUT); pinMode(pedoni[i].pinBuzzer, OUTPUT);
    pinMode(pedoni[i].pinPulsante, INPUT_PULLUP);
  }
  Wire.begin(MY_ADDR); Wire.onReceive(onReceive); Wire.onRequest(onRequest);
  aggiornaLed(0);
}

void loop() {
  if (faseCambiata) { faseCambiata = false; aggiornaLed(pendingFase); }
  
  unsigned long now = millis();
  for (int i=0; i<3; i++) {
    if (digitalRead(pedoni[i].pinPulsante) == LOW) buttonFlags |= (1 << i);

    if (pedState[i] == 1) { // Verde: Bip
      if ((now / 250) % 2) tone(pedoni[i].pinBuzzer, 1000); else noTone(pedoni[i].pinBuzzer);
    } else if (pedState[i] == 2) { // Giallo: Fisso
      tone(pedoni[i].pinBuzzer, 1800);
    } else {
      noTone(pedoni[i].pinBuzzer);
    }
  }
}

void aggiornaLed(uint8_t f) {
  for(int i=0; i<3; i++) pedState[i] = 0; // Tutti Rosso

  if (f == 7) { // A1_VERDE: Tutti verdi
    pedState[0] = 1; pedState[1] = 1; pedState[2] = 1;
  } else if (f == 8) { // A1_GIALLO: Principali Gialli, Secondario resta Verde
    pedState[0] = 2; pedState[1] = 2; pedState[2] = 1;
  } else {
    if(f == 0) pedState[2] = 1;
    else if(f == 1) pedState[2] = 2;
    else if(f == 3) pedState[0] = 1;
    else if(f == 4) pedState[0] = 2;
  }

  for(int i=0; i<3; i++) {
    digitalWrite(pedoni[i].pinVerde, pedState[i]==1);
    digitalWrite(pedoni[i].pinGiallo, pedState[i]==2);
    digitalWrite(pedoni[i].pinRosso, pedState[i]==0);
  }
}
