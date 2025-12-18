#pragma once
#include <gdew042t2.h>
#include <string>
#include <vector>
#include "planning/planning.h"

class Gdew042t2;

void displayClearAndTextCentered(Gdew042t2 &display, int y, const std::string &text);
void displayMaintenanceMode(Gdew042t2 &display);
void displayMenuBuildings(Gdew042t2 &display, int indexBatiment);
void displayMenuEtages(Gdew042t2 &display, int indexEtage);
void displaySalleList(Gdew042t2 &display, int currentStartIndex, int highlightedIndex);

