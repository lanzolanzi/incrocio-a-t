//MASTER

#include <Wire.h>
#define SLAVE_ADDR 0x08

#define FASE_0         0   // Verde Prin / Rosso Sec
#define FASE_1         1   // Verde Prin / Ped Sec Giallo
#define FASE_2         2   // Giallo Prin / Rosso Sec
#define FASE_3         3   // Rosso Prin / Verde Sec
#define FASE_4         4   // Rosso Prin / Verde Sec / Ped Prin Giallo
#define FASE_5         5   // Rosso Prin / Giallo Sec
#define FASE_A1_STOP   6   // A1: verde → giallo
#define FASE_A1_VERDE  7   // A1: tutti rossi, pedoni verdi + buzzer
#define FASE_A1_GIALLO 8   // A1: tutti rossi, pedoni principali gialli

struct Semaforo { uint8_t verde, giallo, rosso; };
Semaforo semPrin = {8, 7, 6};
Semaforo semSec  = {13, 12, 11};

const uint8_t pinPIR = 2;
uint8_t faseAttuale  = 0;
unsigned long ultimoCambio = 0;
bool richiestaA1     = false;
bool richiestaNormale = false;

void setup() {
  Wire.begin();
  pinMode(semPrin.verde,  OUTPUT);
  pinMode(semPrin.giallo, OUTPUT);
  pinMode(semPrin.rosso,  OUTPUT);
  pinMode(semSec.verde,   OUTPUT);
  pinMode(semSec.giallo,  OUTPUT);
  pinMode(semSec.rosso,   OUTPUT);
  pinMode(pinPIR, INPUT);
  passaAllaFase(FASE_0);
}

void loop() {
  unsigned long ora = millis();

  // Lettura pulsanti dallo Slave via I2C
  Wire.requestFrom(SLAVE_ADDR, 1);
  if (Wire.available()) {
    uint8_t flags = Wire.read();
    if (flags & (1 << 1)) richiestaA1 = true;
    else if (flags > 0)   richiestaNormale = true;
  }

  // Lettura PIR
  if (digitalRead(pinPIR) == HIGH) richiestaNormale = true;

  // Gestione pulsante A1 (solo se non si è già in una fase A1)
  if (richiestaA1 && faseAttuale < FASE_A1_STOP) {
    passaAllaFase(FASE_A1_STOP);
    richiestaA1 = false;
  }

  switch (faseAttuale) {
    case FASE_0:
      if (ora - ultimoCambio > 2000 && richiestaNormale)
        passaAllaFase(FASE_1);
      break;

    case FASE_1:
      if (ora - ultimoCambio > 1000)
        passaAllaFase(FASE_2);
      break;

    case FASE_2:
      if (ora - ultimoCambio > 1000)
        passaAllaFase(FASE_3);
      break;

    case FASE_3:
      if (ora - ultimoCambio > 2000) {
        richiestaNormale = false;
        passaAllaFase(FASE_4);
      }
      break;

    case FASE_4:
      if (ora - ultimoCambio > 1000)
        passaAllaFase(FASE_5);
      break;

    case FASE_5:
      if (ora - ultimoCambio > 1000)
        passaAllaFase(FASE_0);
      break;

    // --- Sequenza A1 ---
    case FASE_A1_STOP:
      if (ora - ultimoCambio > 1500)
        passaAllaFase(FASE_A1_VERDE);
      break;

    case FASE_A1_VERDE:
      if (ora - ultimoCambio > 4000)
        passaAllaFase(FASE_A1_GIALLO);
      break;

    case FASE_A1_GIALLO:
      if (ora - ultimoCambio > 1500) {
        richiestaA1      = false;
        richiestaNormale = false;
        passaAllaFase(FASE_0); // Diretto a verde principale
      }
      break;
  }
}

void passaAllaFase(uint8_t n) {
  uint8_t fasePrecedente = faseAttuale;
  faseAttuale  = n;
  ultimoCambio = millis();

  // Invia la nuova fase allo Slave
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(faseAttuale);
  Wire.endTransmission();

  // Spegni tutto prima di aggiornare
  digitalWrite(semPrin.verde,  LOW);
  digitalWrite(semPrin.giallo, LOW);
  digitalWrite(semPrin.rosso,  LOW);
  digitalWrite(semSec.verde,   LOW);
  digitalWrite(semSec.giallo,  LOW);
  digitalWrite(semSec.rosso,   LOW);

  switch (faseAttuale) {
    case FASE_0:
    case FASE_1:
      digitalWrite(semPrin.verde, HIGH);
      digitalWrite(semSec.rosso,  HIGH);
      break;

    case FASE_2:
      digitalWrite(semPrin.giallo, HIGH);
      digitalWrite(semSec.rosso,   HIGH);
      break;

    case FASE_3:
    case FASE_4:
      digitalWrite(semPrin.rosso, HIGH);
      digitalWrite(semSec.verde,  HIGH);
      break;

    case FASE_5:
      digitalWrite(semPrin.rosso,  HIGH);
      digitalWrite(semSec.giallo,  HIGH);
      break;

    case FASE_A1_STOP:
      // Se la principale era verde o gialla → diventa gialla
      // Se era la secondaria ad essere verde → secondaria diventa gialla
      if (fasePrecedente <= FASE_2) {
        digitalWrite(semPrin.giallo, HIGH);
        digitalWrite(semSec.rosso,   HIGH);
      } else {
        digitalWrite(semPrin.rosso,  HIGH);
        digitalWrite(semSec.giallo,  HIGH);
      }
      break;

    case FASE_A1_VERDE:
    case FASE_A1_GIALLO:
      // Tutti i veicoli rossi — i pedoni sono gestiti dallo Slave
      digitalWrite(semPrin.rosso, HIGH);
      digitalWrite(semSec.rosso,  HIGH);
      break;
  }
}
