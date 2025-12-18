#include "display/display.h"
#include "epdspi.h"
#include "Adafruit_GFX.h"
#include "accents/remove_accents.h"
#include "data/salles.h"
#include <cstdio>

void displayClearAndTextCentered(Gdew042t2 &display, int y, const std::string &text) {
    display.fillScreen(EPD_WHITE);
    display.setTextSize(2);
    display.setTextColor(EPD_BLACK);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    int x = (display.width() - w) / 2;
    display.setCursor(x, y);
    display.println(text.c_str());
    display.update();
}

void displayMaintenanceMode(Gdew042t2 &display) {
    display.fillScreen(EPD_WHITE);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(2);
    display.setCursor(40, 120);
    display.println("Mode Maintenance");
    display.setCursor(20, 160);
    display.println("Veuillez scanner un tag RFID");
    display.update();
}

void displayMenuBuildings(Gdew042t2 &display, int indexBatiment) {
    display.fillScreen(EPD_WHITE);
    display.setTextSize(3);
    display.setTextColor(EPD_BLACK);
    const char letters[4] = {'A','B','C','D'};
    int y = 60;
    for(int i=0;i<4;i++){
        if(i == indexBatiment){
            display.fillRect(10, y-6, 380, 34, EPD_BLACK);
            display.setTextColor(EPD_WHITE);
            display.setCursor(20, y);
            char buf[8]; snprintf(buf,sizeof(buf), "> %c", letters[i]);
            display.println(buf);
            display.setTextColor(EPD_BLACK);
        } else {
            display.setCursor(20, y);
            char buf[8]; snprintf(buf,sizeof(buf), "  %c", letters[i]);
            display.println(buf);
        }
        y += 40;
    }
    display.update();
}

void displayMenuEtages(Gdew042t2 &display, int indexEtage) {
    display.fillScreen(EPD_WHITE);
    display.setTextSize(2);
    display.setTextColor(EPD_BLACK);
    int y = 40;
    for(int i=0;i<5;i++){
        display.setCursor(20, y);
        if(i == indexEtage) {
            display.fillRect(10, y-6, 380, 28, EPD_BLACK);
            display.setTextColor(EPD_WHITE);
            char buf[32]; snprintf(buf,sizeof(buf), "> Etage %d", i);
            display.println(buf);
            display.setTextColor(EPD_BLACK);
        } else {
            char buf[32]; snprintf(buf,sizeof(buf), "  Etage %d", i);
            display.println(buf);
        }
        y += 36;
    }
    display.update();
}

void displaySalleList(Gdew042t2 &display, int currentStartIndex, int highlightedIndex) {
    display.fillScreen(EPD_WHITE);
    display.setTextSize(2);
    display.setTextColor(EPD_BLACK);
    display.setCursor(20, 10);
    display.println("Choisir salle:");
    int y = 40;
    int maxDisplay = 5;
    for(int i=0;i<maxDisplay && (currentStartIndex + i) < TOTAL_SALLES; i++){
        const SalleInfo &s = salles[currentStartIndex + i];
        if(i == highlightedIndex){
            display.fillRect(12, y-4, 376, 22, EPD_BLACK);
            display.setTextColor(EPD_WHITE);
            display.setCursor(20, y);
            display.println(s.name);
            display.setTextColor(EPD_BLACK);
        } else {
            display.setCursor(20, y);
            display.println(s.name);
        }
        y += 26;
    }
    display.update();
}