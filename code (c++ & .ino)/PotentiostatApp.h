#pragma once
#include <Arduino.h>

// entry main
void app_setup();
void app_loop();

// project functions
void handleMethodSelection(const String& entree);
void afficherParametres();
void resetProgramme();

float temps_actuel(float frequencyHz_ech);
void chronoamperometrieFunction();
void voltammetrieFunction();

void creationTableau();
void analogWrite10bit(int value);
float getPWMCurrent();
