// Code PWM 10 BITS ---

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ░░░░░░░░░░░░ 1. INITIALISATION GÉNÉRALE ░░░░░░░░░░░░
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Variables globales
String methode = "";
int etape = 0;   // État initial de la machine d’état
float E1, E2, vitesse, t0, cycles;  // variables à déclarer
float E1_adapted,E2_adapted;

int pwmPin = 9; //pin sortie
int inputPin=A0; //pin entrée
float frequencyHz_ech = 12.6582278418; //valeur changer en fonction de la méthode choisit
int value=0; // Variable globale pour stocker la tension actuelle
int facteur_affichage = 4; // Affiche 1 valeur sur 4 (modifiable) où 1 c'est la valeur max qui peut être exploité par l'arduino avec notre code

bool chrono_aff=false; //bool pour envoyer le créneau après le choix des variables
bool volto_aff=false; //bool pour envoyer le triangle après le choix des variables

float pas_mV=0.0;
float temp=0.0;


// On configure Timer1 en mode 10 bits PWM sur la broche 9 (OC1A)
void setup() {
    // Configurer les pins
    pinMode(pwmPin, OUTPUT); // PWM sur pin 9 (OC1A)
    pinMode(inputPin, INPUT);
    Serial.begin(9600);

    // TCCR1A : Contrôle la configuration du Timer1 pour le mode de génération du PWM
    TCCR1A = _BV(COM1A1) | _BV(WGM11) | _BV(WGM10); // Fast PWM 10 bits, Clear OC1A on match
    // -> COM1A1 = 1 : Active la sortie PWM sur la broche OC1A (pin 9 sur Uno/Mega), en mode "non-inversé" (Clear OC1A on compare match, set at BOTTOM)
    // -> WGM11 = 1 et WGM10 = 1 : Deux bits du mode WGM pour activer le mode Fast PWM 10 bits (avec WGM12 ci-dessous)

    // TCCR1B : Complète la configuration du mode PWM et définit la source d’horloge
    TCCR1B = _BV(WGM12) | _BV(CS10); // Pas de prescaler (clk/1)
    // -> WGM12 = 1 : Troisième bit requis pour activer le mode Fast PWM 10 bits (WGM13=0, WGM12=1, WGM11=1, WGM10=1 donne WGM=0b1110 = Mode 7 : Fast PWM 10 bits)
    // -> CS10 = 1 : Aucun prescaler, donc fréquence du Timer = fréquence du système (16 MHz sur Arduino Uno), ce qui permet un PWM à haute résolution et haute fréquence

    Serial.println("Choisissez la methode :");
    Serial.println("1 - Chronoamperometrie");
    Serial.println("2 - Voltammetrie cyclique");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ░░░░░░░░░░░░ 2. GESTION DES ENTRÉES UTILISATEUR ░░░░░░░░░░░░
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void loop() {
    if (Serial.available()) {
        String entree = Serial.readStringUntil('\n'); // Lire la saisie

        switch (etape) {
            case 0:  // Choix de la méthode
                handleMethodSelection(entree);
                break;

            case 1:  // Saisie de E1
                E1_adapted = entree.toFloat();
                etape = 2;
                Serial.println("Entrez E2 (en Volts) :");
                break;

            case 2:  // Saisie de E2
                E2_adapted = entree.toFloat();
                if (methode == "Chronoamperometrie") {
                    etape = 3;
                    Serial.println("Entrez le temps de transition (en secondes) :");
                } else {
                    etape = 4;
                    Serial.println("Entrez la vitesse de balayage (en V/s) :");
                }
                break;

            case 3:  // Saisie de t0 (Chronoampérométrie)
                t0 = entree.toFloat();
                afficherParametres();
                etape = 6;
                Serial.println("Entrez le facteur d'affichage (valeur par défaut = 1, valeur utilisée pour les mesures = 4) :");
                break;

            case 4:  // Saisie de vitesse (Voltammétrie)
                vitesse = entree.toFloat();
                Serial.println("Entrez le nombre de cycles :");
                etape = 5;
                break;

            case 5:  // Saisie du nombre de cycles pour Voltammétrie
                cycles = entree.toFloat();
                afficherParametres();
                etape = 6;
                Serial.println("Entrez le facteur d'affichage (valeur par défaut = 1, valeur utilisée pour les mesures = 4) :");
                break;

            case 6: // Saisie du facteur d'affichage
                facteur_affichage = max(entree.toInt(), 1); // on s'assurer que c’est >= 1

                Serial.print("Facteur d'affichage choisi : ");
                Serial.println(facteur_affichage);
                Serial.println("Entrez 'ok' pour commencer l'expérience.");

                // Attente utilisateur
                while (true) {
                    if (Serial.available()) {
                        String entree = Serial.readStringUntil('\n');

                        if (entree == "ok") {
                            Serial.println("Debut");
                            break;
                        } else {
                            Serial.println("Veuillez entrer 'ok' pour commencer l'expérience.");
                        }
                    }
                }

                if (methode == "Chronoamperometrie") {
                    chrono_aff = true;
                } else {
                    volto_aff = true;
                }

                Serial.println("Temps (s)\t tension (mV)\t courant (uA)");
                break;
                
        }
    }

    if (chrono_aff) {
        chronoamperometrieFunction();
    }

    if (volto_aff) {
        voltammetrieFunction();
    }



    if (etape==7 && Serial.available()){
      String entree=Serial.readStringUntil('\n');
      if (entree == "Again"){
        resetProgramme();
      }
    }
}

// Fonction pour choisir la méthode
void handleMethodSelection(String entree) {
    if (entree == "1") {
        methode = "Chronoamperometrie";
        etape = 1;
        Serial.println("Methode selectionnee : Chronoamperometrie");
        Serial.println("Entrez E1 (en Volts) :");
    }
    else if (entree == "2") {
        methode = "Voltammetrie cyclique";
        etape = 1;
        Serial.println("Methode selectionnee : Voltammetrie cyclique");
        Serial.println("Entrez E1 (en Volts) :");
    }
    else {
        Serial.println("Entree invalide. Veuillez choisir 1 ou 2.");
    }
}

// Fonction pour afficher les paramètres sur le moniteur série
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

void resetProgramme(){
  //réinitialisation de toutes les variables
  methode="";
  etape=0;
  E1=E2=vitesse=t0=cycles=0;
  E1_adapted=E2_adapted=0;
  facteur_affichage=4;
  chrono_aff=false;
  volto_aff=false;
  pas_mV=0;
  temp=0;
  Serial.println("Redémarrage du programme: Veuillez choisir 1 ou 2.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ░░░░░░░░░░░░ 3. EXÉCUTION DE LA MÉTHODE EXPÉRIMENTALE ░░░░░░░░░░░░
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Calcule le temps écoulé basé sur une fréquence d’échantillonnage donnée
float temps_actuel(float frequencyHz_ech) {
    static bool started = false;             // Démarrage initial
    static float currentTime = 0;            // Temps actuel (s)
    static unsigned long lastUpdateTime = 0; // Dernier temps (ms)

    float period_ms = (1.0 / frequencyHz_ech) * 1000; // Période d’échantillonnage (ms)
    unsigned long now = millis();                     // Temps actuel (ms)

    if (!started) {
        // Premier appel
        lastUpdateTime = now;
        currentTime = period_ms / 1000.0;  // Premier point (> 0)
        started = true;
        return currentTime;
    }

    if (now - lastUpdateTime >= period_ms) {
        // Temps écoulé atteint
        currentTime += period_ms / 1000.0;
        lastUpdateTime = now;
        return currentTime;
    }

    return -1; // Trop tôt, attendre encore
}

void chronoamperometrieFunction() {
  chrono_aff = false;  // Pour n'exécuter qu'une seule fois

  // Calcul du pas en mV par niveau PWM
  pas_mV = 2000.0 / 1023.0;

  // Conversion des tensions d'entrée en mV
  int E1_mV = E2_adapted * 1000;
  int E2_mV = E1_adapted * 1000;

  // Conversion en valeur PWM 10 bits
  int pwmE1 = round((-E1_mV + 1000.0) / pas_mV);
  int pwmE2 = (-E2_mV + 1000) / pas_mV;
  pwmE1 = constrain(pwmE1, 0, 1023);
  pwmE2 = constrain(pwmE2, 0, 1023);

  // Fréquence d’échantillonnage
  frequencyHz_ech = 34.48;
  unsigned long interval = 1000.0 / frequencyHz_ech; // ms
  float interval_s = interval / 1000.0;

  float temps = 0.0;

  // Phase 1 : maintien à E1 pendant t0 secondes
  while (temps < t0) {
    analogWrite10bit(pwmE1);
    value = pwmE1;

    float tension = -1000.0 + value * pas_mV;
    if (abs(E1_adapted) < 0.001 && abs(E2_adapted) < 0.001) tension = 0;

    Serial.print(temps, 6);
    Serial.print(";\t");
    Serial.print(tension,0);
    Serial.print(";\t");
    Serial.println(getPWMCurrent(), 2);

    delay(interval);
    temps += interval_s;
  }

  // Phase 2 : maintien à E2 pendant 5 secondes
  for (int i = 0; i < (5 * frequencyHz_ech); i++) {
    analogWrite10bit(pwmE2);
    value = pwmE2;

    float tension = -1000.0 + value * pas_mV;
    if (abs(E1_adapted) < 0.001 && abs(E2_adapted) < 0.001) tension = 0;

    Serial.print(temps, 6);
    Serial.print(";\t");
    Serial.print(tension,0);
    Serial.print(";\t");
    Serial.println(getPWMCurrent(), 2);

    delay(interval);
    temps += interval_s;
  }

  Serial.println("\nAcquisition complétée");
  etape = 7;
}


// --- DANS voltammetrieFunction() ---


void voltammetrieFunction() {

  volto_aff=false; //Afin de faire une seule boucle à la fois

  int E1_mV = E2_adapted*1000;
  int E2_mV = E1_adapted*1000;
  int v_milli = vitesse*1000;

  //avoir le max fonctionne très bien avec le max
  //pour faciliter les calculs des pentes on utilise des mV, ms et de mV/s dans les calculs
  pas_mV = 2000.0 / 1023.0; // 1023 pas au lieu de 255
  unsigned long interval = (pas_mV * 1000) / v_milli;
  float frequencyHz_ech=v_milli/(pas_mV); //c'est la fréquence max
  // Conversion en PWM 10 bits (0-1023)
  int pwmMin = (-E1_mV + 1000) / pas_mV;
  int pwmMax = (-E2_mV + 1000) / pas_mV;

  int compteur_cycle=0;
  while (compteur_cycle <= (cycles-1)) {
    // Descente
    for (value = pwmMin; value <= pwmMax; value++) {
      analogWrite10bit(value);
      creationTableau();
      delay(interval);
    }

    // Montée
    for (value = pwmMax; value >= pwmMin; value--) {
      analogWrite10bit(value);
      creationTableau();
      delay(interval);
    }
    compteur_cycle++;
  }

  Serial.println("\nAcquisition complétée");
  etape=7;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ░░░░░░░░░░░░ 4. UTILITAIRES DE MESURE ET SIGNAL ░░░░░░░░░░░░
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void creationTableau() {
  static int compteur_echantillon = 0; //pour réduire ou augmenter le nombre de valeurs affiché

  temp = temps_actuel(frequencyHz_ech);
  if (temp > 0) {
    compteur_echantillon++;

    if (facteur_affichage == 4) {
      delay(2); // Simule le temps qu'aurait pris l’affichage des 3 lignes non affiché sinon c'est trop rapide (valide seulement pour 4)
    }

    if (compteur_echantillon % facteur_affichage == 0) {
      float tension_affichee = -pas_mV * value + 1000;

      if (facteur_affichage == 4) {
        tension_affichee += 5.87; // décalage expérimental thomas
      }
      Serial.print(temp, 4);
      Serial.print(";\t");
      Serial.print(tension_affichee, 2);
      Serial.print(";\t");
      Serial.println(getPWMCurrent(), 2);
    }
  }
}

// --- UTILISER analogWrite 10 BITS (remplace analogWrite pour Timer1) ---
void analogWrite10bit(int value) {
    OCR1A = constrain(value, 0, 1023); // PWM 10 bits sur pin 9
}

// Fonction pour estimer le courant (en µA) à partir de la tension PWM filtrée
float getPWMCurrent() {
  return analogRead(inputPin) * (400.0 / 1023.0) - 200.0;
}
