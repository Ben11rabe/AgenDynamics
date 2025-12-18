#pragma once
#include <string>
#include <vector>

struct SalleInfo {
    int id;
    std::string name;
};

char getBatiment(const std::string &salleName);
int getEtage(const std::string &salleName);
int nombreDeSalles(char batiment, int etage);
int getSalleIndex(char batiment, int etage, int index);
int clampIndex(char batiment, int etage, int index);

extern const SalleInfo salles[];
extern const int TOTAL_SALLES;