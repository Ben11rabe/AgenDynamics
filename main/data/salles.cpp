#include "salles.h"
#include <cctype>

const SalleInfo salles[] = {
    {169, "B008/B009 - FERMI-DIRAC"}, {170, "B008 - FERMI"}, {172, "B009 - DIRAC"}, {173, "A107 - TURING"},
    {174, "A108 - BABBAGE"}, {175, "B005 - EULER"}, {176, "B006 - WEIERSTRASS"}, {177, "B108 - TAYLOR"},
    {178, "B111 - RIEMANN"}, {179, "B113 - PASCAL"}, {180, "B114 - CAUCHY"}, {181, "B115 - FOURIER"},
    {182, "B116 - DESCARTES"}, {183, "B118 - GAUSS"}, {184, "B404 - RAMANUJAN"}, {185, "B405 - GALOIS"},
    {186, "A204 - WATT"}, {187, "A205 - BLONDEL"}, {188, "A206 - FLOYD"}, {189, "A207 - HOARE"},
    {190, "A208 - von NEUMANN"}, {191, "A209 - BOOLE"}, {192, "B119 - BRAGG"}, {193, "B204 - BODE"},
    {195, "B209 - VOLTA"}, {196, "B211 - CARNOT"}, {197, "B219 - MAXWELL"}, {198, "B304 - AMPERE"},
    {200, "B305 - FRESNEL"}, {201, "B306 - MICHELSON"}, {202, "B308 - FARADAY"}, {203, "B309 - LAPLACE"},
    {204, "B313 - BRANLY"}, {205, "B315 - SHAKESPEARE"}, {206, "A303 - HEISENBERG"}, {207, "A304 - LEPRINCE-RINGUET"},
    {208, "A305 - SHANNON"}, {209, "A306 - PLANCK"}, {210, "A307 - EINSTEIN"}, {211, "A308 - SCHRODINGER"},
    {212, "A314 - LANDAU"}, {213, "A315 - NEWTON"}, {214, "A316 - LANGEVIN"}, {215, "A401 - BELL"},
    {216, "A402 - BERNOULLI"}, {217, "A403 - COULOMB"}, {218, "A404 - CURIE"}, {219, "A405 - MEITNER"},
    {220, "A411 - BOHR"}, {222, "A412 - KELVIN"}, {223, "A413 - GALILEE"}, {224, "B007 - BROGLIE"},
    {225, "B104 - KEPLER"}, {226, "B310 - WIENER"}, {227, "B311 - JOULE"}, {228, "B312 - SIEMENS"},
    {229, "B314 - EDISON"}, {230, "B316 - DICKENS"}, {231, "B317 - TESLA"}, {232, "C205 - MARCONI"},
    {233, "C206 - LAENNEC"}, {234, "C207 - KALMAN"}, {235, "D002 - JEANNETEAU"}, {236, "DS02 - Auditorium de l'ANJOU"},
    {247, "B205 - NYQUIST"}, {252, "A401 - BELL - T1"}, {253, "A401 - BELL - T2"}, {254, "A402 - BERNOULLI - T1"},
    {255, "A402 - BERNOULLI - T2"}, {256, "A403 - COULOMB - T1"}, {257, "A403 - COULOMB - T2"}, {258, "A404 - CURIE - T1"},
    {259, "A404 - CURIE - T2"}, {260, "A304 - LEPRINCE-RINGUET - T1"}, {261, "A304 - LEPRINCE-RINGUET - T2"}, {262, "A305 - SHANNON - T1"},
    {263, "A305 - SHANNON - T2"}, {264, "A306 - PLANCK - T1"}, {265, "A306 - PLANCK - T2"}, {266, "A307 - EINSTEIN - T1"},
    {267, "A307 - EINSTEIN - T2"}, {268, "A308 - SCHRODINGER - T1"}, {269, "A308 - SCHRODINGER - T2"}, {277, "B309 - LAPLACE T2"},
    {278, "B309 - LAPLACE T1"}
};


const int TOTAL_SALLES = sizeof(salles)/sizeof(salles[0]);

char getBatiment(const std::string &salleName) {
    if(salleName.size() == 0) return '?';
    return salleName[0];
}

int getEtage(const std::string &salleName) {
    if(salleName.empty()) return 0;

    // on ignore la première lettre (A/B/C/D)
    for(size_t i = 1; i < salleName.size(); i++) {
        if(isdigit(salleName[i])) {
            return salleName[i] - '0'; // premier chiffre = étage
        }
    }

    return 0; // par sécurité
}

int nombreDeSalles(char batiment, int etage) {
    int count = 0;
    for (int i=0;i<TOTAL_SALLES;i++){
        if(getBatiment(salles[i].name) == batiment && getEtage(salles[i].name) == etage) count++;
    }
    return count;
}

int getSalleIndex(char batiment, int etage, int index) {
    int count = 0;
    for (int i=0;i<TOTAL_SALLES;i++){
        if(getBatiment(salles[i].name) == batiment && getEtage(salles[i].name) == etage){
            if(count == index) return i;
            count++;
        }
    }
    return -1;
}

int clampIndex(char batiment, int etage, int index) {
    int total = nombreDeSalles(batiment, etage);
    if (total == 0) return -1;

    if (index < 0) return 0;
    if (index >= total) return total - 1;

    return index;
}

