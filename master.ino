//MASTER

#include <Wire.h>
#define SLAVE_ADDR 0x08

// Definizione fasi (aggiunta fase 6)
#define FASE_0 0
#define FASE_1 1
#define FASE_2 2
#define FASE_3 3
#define FASE_4 4
#define FASE_5 5
#define FASE_EMERGENZA 6

struct Semaforo { uint8_t verde, giallo, rosso; };
Semaforo semPrin = {8, 7, 6}, semSec = {13, 12, 11};
const uint8_t pinPIR = 2;
uint8_t faseAttuale = 0;
unsigned long ultimoCambio = 0;
bool richiestaAttesa = false;
bool emergenzaRichiesta = false;

void setup() {
  Wire.begin();
  pinMode(semPrin.verde, OUTPUT); pinMode(semPrin.giallo, OUTPUT); pinMode(semPrin.rosso, OUTPUT);
  pinMode(semSec.verde, OUTPUT); pinMode(semSec.giallo, OUTPUT); pinMode(semSec.rosso, OUTPUT);
  pinMode(pinPIR, INPUT);
  passaAllaFase(FASE_0); 
}

void loop() {
  unsigned long ora = millis();
  
  // Controllo ingressi
  Wire.requestFrom(SLAVE_ADDR, 1);
  if (Wire.available()) {
    uint8_t flags = Wire.read();
    if (flags & (1 << 1)) emergenzaRichiesta = true; // Pulsante A1 (Pedonale 1)
    else if (flags > 0) richiestaAttesa = true;     // Altri pulsanti
  }
  if (digitalRead(pinPIR) == HIGH) richiestaAttesa = true;

  // Se viene premuto A1, salta immediatamente all'emergenza (se non è già attiva)
  if (emergenzaRichiesta && faseAttuale != FASE_EMERGENZA) {
    passaAllaFase(FASE_EMERGENZA);
    emergenzaRichiesta = false;
  }

  switch (faseAttuale) {
    case FASE_0:
      if (ora - ultimoCambio > 2000 && richiestaAttesa) passaAllaFase(FASE_1);
      break;
    case FASE_1: if (ora - ultimoCambio > 1000) passaAllaFase(2); break;
    case FASE_2: if (ora - ultimoCambio > 1000) passaAllaFase(3); break;
    case FASE_3:
      if (ora - ultimoCambio > 2000) { richiestaAttesa = false; passaAllaFase(4); }
      break;
    case FASE_4: if (ora - ultimoCambio > 1000) passaAllaFase(5); break;
    case FASE_5: if (ora - ultimoCambio > 1000) passaAllaFase(0); break;
    
    case FASE_EMERGENZA:
      // Dopo 4 secondi di "Tutti Verde Pedoni", torna alla fase 0
      if (ora - ultimoCambio > 4000) passaAllaFase(FASE_0);
      break;
  }
}

void passaAllaFase(uint8_t n) {
  faseAttuale = n; ultimoCambio = millis();
  Wire.beginTransmission(SLAVE_ADDR); Wire.write(faseAttuale); Wire.endTransmission();
  
  // Logica Luci Auto
  if (faseAttuale == FASE_EMERGENZA) {
    // TUTTI ROSSO
    digitalWrite(semPrin.verde, LOW); digitalWrite(semPrin.giallo, LOW); digitalWrite(semPrin.rosso, HIGH);
    digitalWrite(semSec.verde, LOW);  digitalWrite(semSec.giallo, LOW);  digitalWrite(semSec.rosso, HIGH);
  } else {
    digitalWrite(semPrin.verde, (faseAttuale <= 1));
    digitalWrite(semPrin.giallo, (faseAttuale == 2));
    digitalWrite(semPrin.rosso, (faseAttuale >= 3));
    digitalWrite(semSec.verde, (faseAttuale == 3 || faseAttuale == 4));
    digitalWrite(semSec.giallo, (faseAttuale == 5));
    digitalWrite(semSec.rosso, (faseAttuale <= 2));
  }
}
