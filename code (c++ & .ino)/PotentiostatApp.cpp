#include "PotentiostatApp.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 1) INITIALISATION GÉNÉRALE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Variables globales (identiques à ton .ino)
String methode = "";
int etape = 0;   // État initial de la machine d’état
float E1, E2, vitesse, t0, cycles;
float E1_adapted, E2_adapted;

int pwmPin = 9;   // pin sortie (OC1A)
int inputPin = A0; // pin entrée
float frequencyHz_ech = 12.6582278418; // valeur changée selon la méthode
int value = 0;    // stocke la valeur PWM actuelle
int facteur_affichage = 4; // 1=affiche tout, 4=1 point sur 4

bool chrono_aff = false;
bool volto_aff = false;

float pas_mV = 0.0;
float temp = 0.0;

// On configure Timer1 en mode 10 bits PWM sur la broche 9 (OC1A)
void app_setup() {
  pinMode(pwmPin, OUTPUT);
  pinMode(inputPin, INPUT);
  Serial.begin(9600);

  // Fast PWM 10 bits, non-inversé, sortie sur OC1A (pin 9)
  TCCR1A = _BV(COM1A1) | _BV(WGM11) | _BV(WGM10);
  // Mode Fast PWM 10 bits + pas de prescaler (clk/1)
  TCCR1B = _BV(WGM12) | _BV(CS10);

  Serial.println("Choisissez la methode :");
  Serial.println("1 - Chronoamperometrie");
  Serial.println("2 - Voltammetrie cyclique");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2) GESTION DES ENTRÉES UTILISATEUR
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void app_loop() {
  if (Serial.available()) {
    String entree = Serial.readStringUntil('\n');

    switch (etape) {
      case 0:  // Choix méthode
        handleMethodSelection(entree);
        break;

      case 1:  // E1
        E1_adapted = entree.toFloat();
        etape = 2;
        Serial.println("Entrez E2 (en Volts) :");
        break;

      case 2:  // E2
        E2_adapted = entree.toFloat();
        if (methode == "Chronoamperometrie") {
          etape = 3;
          Serial.println("Entrez le temps de transition (en secondes) :");
        } else {
          etape = 4;
          Serial.println("Entrez la vitesse de balayage (en V/s) :");
        }
        break;

      case 3:  // t0 (chrono)
        t0 = entree.toFloat();
        afficherParametres();
        etape = 6;
        Serial.println("Entrez le facteur d'affichage (defaut=1, mesures=4) :");
        break;

      case 4:  // vitesse (volto)
        vitesse = entree.toFloat();
        Serial.println("Entrez le nombre de cycles :");
        etape = 5;
        break;

      case 5:  // cycles
        cycles = entree.toFloat();
        afficherParametres();
        etape = 6;
        Serial.println("Entrez le facteur d'affichage (defaut=1, mesures=4) :");
        break;

      case 6: { // facteur affichage + attente "ok"
        facteur_affichage = max(entree.toInt(), 1);

        Serial.print("Facteur d'affichage choisi : ");
        Serial.println(facteur_affichage);
        Serial.println("Entrez 'ok' pour commencer l'experience.");

        while (true) {
          if (Serial.available()) {
            String e = Serial.readStringUntil('\n');
            if (e == "ok") {
              Serial.println("Debut");
              break;
            } else {
              Serial.println("Veuillez entrer 'ok' pour commencer l'experience.");
            }
          }
        }

        if (methode == "Chronoamperometrie") chrono_aff = true;
        else volto_aff = true;

        Serial.println("Temps (s)\t tension (mV)\t courant (uA)");
        break;
      }
    }
  }

  if (chrono_aff) chronoamperometrieFunction();
  if (volto_aff)  voltammetrieFunction();

  if (etape == 7 && Serial.available()) {
    String entree = Serial.readStringUntil('\n');
    if (entree == "Again") resetProgramme();
  }
}

// Fonction pour choisir la méthode
void handleMethodSelection(const String& entree) {
  if (entree == "1") {
    methode = "Chronoamperometrie";
    etape = 1;
    Serial.println("Methode selectionnee : Chronoamperometrie");
    Serial.println("Entrez E1 (en Volts) :");
  } else if (entree == "2") {
    methode = "Voltammetrie cyclique";
    etape = 1;
    Serial.println("Methode selectionnee : Voltammetrie cyclique");
    Serial.println("Entrez E1 (en Volts) :");
  } else {
    Serial.println("Entree invalide. Veuillez choisir 1 ou 2.");
  }
}

// Affichage des paramètres
void afficherParametres() {
  Serial.println("Parametres enregistres.");
  Serial.print("E1 = "); Serial.print(E1_adapted); Serial.print(" V, ");
  Serial.print("E2 = "); Serial.print(E2_adapted); Serial.print(" V, ");

  if (methode == "Chronoamperometrie") {
    Serial.print("t0 = "); Serial.print(t0); Serial.println(" s");
  } else {
    Serial.print("Vitesse = "); Serial.print(vitesse); Serial.println(" V/s");
    Serial.print("Cycles = "); Serial.println(cycles);
  }
}

void resetProgramme() {
  methode = "";
  etape = 0;
  E1 = E2 = vitesse = t0 = cycles = 0;
  E1_adapted = E2_adapted = 0;
  facteur_affichage = 4;
  chrono_aff = false;
  volto_aff = false;
  pas_mV = 0;
  temp = 0;

  Serial.println("Redemarrage du programme: Veuillez choisir 1 ou 2.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 3) MÉTHODES EXPÉRIMENTALES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

float temps_actuel(float frequencyHz_ech) {
  static bool started = false;
  static float currentTime = 0;
  static unsigned long lastUpdateTime = 0;

  float period_ms = (1.0f / frequencyHz_ech) * 1000.0f;
  unsigned long now = millis();

  if (!started) {
    lastUpdateTime = now;
    currentTime = period_ms / 1000.0f;
    started = true;
    return currentTime;
  }

  if (now - lastUpdateTime >= (unsigned long)period_ms) {
    currentTime += period_ms / 1000.0f;
    lastUpdateTime = now;
    return currentTime;
  }

  return -1;
}

void chronoamperometrieFunction() {
  chrono_aff = false;

  pas_mV = 2000.0 / 1023.0;

  int E1_mV = (int)(E2_adapted * 1000);
  int E2_mV = (int)(E1_adapted * 1000);

  int pwmE1 = (int)round((-E1_mV + 1000.0) / pas_mV);
  int pwmE2 = (int)((-E2_mV + 1000.0) / pas_mV);
  pwmE1 = constrain(pwmE1, 0, 1023);
  pwmE2 = constrain(pwmE2, 0, 1023);

  frequencyHz_ech = 34.48f;
  unsigned long interval = (unsigned long)(1000.0 / frequencyHz_ech);
  float interval_s = interval / 1000.0f;

  float temps = 0.0;

  while (temps < t0) {
    analogWrite10bit(pwmE1);
    value = pwmE1;

    float tension = -1000.0 + value * pas_mV;
    if (abs(E1_adapted) < 0.001 && abs(E2_adapted) < 0.001) tension = 0;

    Serial.print(temps, 6);
    Serial.print(";\t");
    Serial.print(tension, 0);
    Serial.print(";\t");
    Serial.println(getPWMCurrent(), 2);

    delay(interval);
    temps += interval_s;
  }

  for (int i = 0; i < (int)(5 * frequencyHz_ech); i++) {
    analogWrite10bit(pwmE2);
    value = pwmE2;

    float tension = -1000.0 + value * pas_mV;
    if (abs(E1_adapted) < 0.001 && abs(E2_adapted) < 0.001) tension = 0;

    Serial.print(temps, 6);
    Serial.print(";\t");
    Serial.print(tension, 0);
    Serial.print(";\t");
    Serial.println(getPWMCurrent(), 2);

    delay(interval);
    temps += interval_s;
  }

  Serial.println("\nAcquisition completee");
  etape = 7;
}

void voltammetrieFunction() {
  volto_aff = false;

  int E1_mV = (int)(E2_adapted * 1000);
  int E2_mV = (int)(E1_adapted * 1000);
  int v_milli = (int)(vitesse * 1000);

  pas_mV = 2000.0 / 1023.0;
  unsigned long interval = (unsigned long)((pas_mV * 1000.0) / v_milli);

  // NOTE: ici tu redéclares un local; creationTableau() utilise le global frequencyHz_ech.
  // On garde ton comportement original en ne changeant pas ça.
  float frequencyHz_ech_local = v_milli / pas_mV;
  (void)frequencyHz_ech_local; // éviter warning si pas utilisé

  int pwmMin = (int)((-E1_mV + 1000.0) / pas_mV);
  int pwmMax = (int)((-E2_mV + 1000.0) / pas_mV);

  int compteur_cycle = 0;
  while (compteur_cycle <= (cycles - 1)) {
    for (value = pwmMin; value <= pwmMax; value++) {
      analogWrite10bit(value);
      creationTableau();
      delay(interval);
    }

    for (value = pwmMax; value >= pwmMin; value--) {
      analogWrite10bit(value);
      creationTableau();
      delay(interval);
    }
    compteur_cycle++;
  }

  Serial.println("\nAcquisition completee");
  etape = 7;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 4) UTILITAIRES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void creationTableau() {
  static int compteur_echantillon = 0;

  temp = temps_actuel(frequencyHz_ech);
  if (temp > 0) {
    compteur_echantillon++;

    if (facteur_affichage == 4) {
      delay(2);
    }

    if (compteur_echantillon % facteur_affichage == 0) {
      float tension_affichee = -pas_mV * value + 1000;

      if (facteur_affichage == 4) {
        tension_affichee += 5.87;
      }

      Serial.print(temp, 4);
      Serial.print(";\t");
      Serial.print(tension_affichee, 2);
      Serial.print(";\t");
      Serial.println(getPWMCurrent(), 2);
    }
  }
}

void analogWrite10bit(int v) {
  OCR1A = constrain(v, 0, 1023);
}

float getPWMCurrent() {
  return analogRead(inputPin) * (400.0 / 1023.0) - 200.0;
}
