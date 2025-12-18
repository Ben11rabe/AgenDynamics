#pragma once
//#include "gdew042t2.h"
#include <string>
#include <vector>

struct Cours{
    std::string libelle;
    std::string prof;
    std::string debut;
    std::string fin;
};

std::string getPlanningFromServer(int salleId);
std::vector<Cours> parsePlanning(const std::string &jsonStr, std::string &salle);

class Gdew042t2; 

void displayPlanning(Gdew042t2 &display, const std::string &salle, const std::vector<Cours> &coursList);

