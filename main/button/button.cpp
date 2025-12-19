#include "button/button.h"
#include "display/display.h"
#include "planning/planning.h"
#include "data/salles.h"
#include "planning/planning.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"


extern int indexBatiment ;
extern int indexEtage ;
extern int currentStartIndex ;   
extern int highlightedIndex ; 
extern volatile bool menuActive ;   // menu activé après UID autorisé
extern volatile bool salleSelected ;
extern Gdew042t2 display ;

extern int menuState ; // 0 = choix bâtiment, 1 = choix étage, 2 = choix salle

extern int SALLE_ID ; // valeur finale

void button_monitor_init_pin(int gpio)
{
    // select pad then configure input + pullup
    esp_rom_gpio_pad_select_gpio((gpio_num_t)gpio);
    gpio_set_direction((gpio_num_t)gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)gpio, GPIO_PULLUP_ONLY);
}

// ---- Handlers pour actions boutons (protégés par menuMutex) ----
void handle_up_press()
{
    if(xSemaphoreTake(menuMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
    if(!menuActive || salleSelected) { xSemaphoreGive(menuMutex); return; }

    if(menuState == 0) {
        indexBatiment = (indexBatiment - 1 + 4) % 4;
        displayMenuBuildings(display, indexBatiment);
    } else if(menuState == 1) {
        indexEtage = (indexEtage - 1 + 5) % 5;
        displayMenuEtages(display, indexEtage);
    } else if(menuState == 2) {
        if(highlightedIndex > 0) {
            highlightedIndex--;
        } else if(currentStartIndex > 0) {
            currentStartIndex--;
        }
        displaySalleList(display, currentStartIndex, highlightedIndex);
    }
    xSemaphoreGive(menuMutex);
}


void handle_down_press()
{
    if(xSemaphoreTake(menuMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
    if(!menuActive || salleSelected) { xSemaphoreGive(menuMutex); return; }

    if(menuState == 0) {
        indexBatiment = (indexBatiment + 1) % 4;
        displayMenuBuildings(display, indexBatiment);
    } else if(menuState == 1) {
        indexEtage = (indexEtage + 1) % 5;
        displayMenuEtages(display, indexEtage);
    } else if(menuState == 2) {
        if(highlightedIndex < 4 && (currentStartIndex + highlightedIndex + 1) < TOTAL_SALLES) {
            highlightedIndex++;
        } else if((currentStartIndex + 5) < TOTAL_SALLES) {
            currentStartIndex++;
        }
        displaySalleList(display, currentStartIndex, highlightedIndex);
    }
    xSemaphoreGive(menuMutex);
}

void handle_select_press()
{
    // lock menu state while we compute selection
    if(xSemaphoreTake(menuMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
    if(!menuActive || salleSelected) { xSemaphoreGive(menuMutex); return; }

    if(menuState == 0) {
        menuState = 1;
        indexEtage = 0;
        displayMenuEtages(display, indexEtage);
    } else if(menuState == 1) {
        menuState = 2;
        // initialisation liste salles (premier index de ce batiment/etage)
        int firstIdx = -1;
        char bat = 'A' + indexBatiment;
        for(int i=0;i<TOTAL_SALLES;i++){
            if(getBatiment(salles[i].name) == bat && getEtage(salles[i].name) == indexEtage){ firstIdx = i; break;}
        }
        if(firstIdx >= 0){
            currentStartIndex = firstIdx;
            highlightedIndex = 0;
        } else {
            currentStartIndex = 0;
            highlightedIndex = 0;
        }
        displaySalleList(display, currentStartIndex, highlightedIndex);
    } else if(menuState == 2) {
        int selectedGlobalIndex = currentStartIndex + highlightedIndex;
        if(selectedGlobalIndex >= 0 && selectedGlobalIndex < TOTAL_SALLES){
            SALLE_ID = salles[selectedGlobalIndex].id;
            salleSelected = true;
            menuActive = false;
            // release mutex before performing long blocking display/network ops
            xSemaphoreGive(menuMutex);

            displayClearAndTextCentered(display,140, "Salle selectionnee,\n en cours de chargement...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            string response = getPlanningFromServer(SALLE_ID);
            if(!response.empty()){
                string salleName;
                vector<Cours> coursList = parsePlanning(response,salleName);
                if(!coursList.empty()) displayPlanning(display,salleName, coursList);
                else displayClearAndTextCentered(display,140, "Planning vide ou erreur");
                vTaskDelay(pdMS_TO_TICKS(1000));
            } else {
                displayClearAndTextCentered(display,140, "Erreur API ou connexion");
                vTaskDelay(pdMS_TO_TICKS(1000));
            }            
        }
    }
    xSemaphoreGive(menuMutex);
}


void handle_back_press()
{
    if(xSemaphoreTake(menuMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;

    // Si tu es en planning : retour impossible (retour = RFID)
    if(!menuActive) {
        xSemaphoreGive(menuMutex);
        return;
    }

    // ↩ Retour depuis Salles → Étages
    if(menuState == 2) {
        menuState = 1;
        displayMenuEtages(display, indexEtage);
    }
    // ↩ Retour depuis Étages → Bâtiments
    else if(menuState == 1) {
        menuState = 0;
        displayMenuBuildings(display, indexBatiment);
    }
    // ↩ Retour depuis Bâtiments → Mode maintenance
    else if(menuState == 0) {
        menuActive = false;
        salleSelected = false;
        displayMaintenanceMode(display);
    }

    xSemaphoreGive(menuMutex);
}

// ---- Single buttons task (prévents race conditions) ----
void buttons_task(void *arg) {
    // init pins
    button_monitor_init_pin(BUTTON_UP_PIN);
    button_monitor_init_pin(BUTTON_DOWN_PIN);
    button_monitor_init_pin(BUTTON_SELECT_PIN);
    button_monitor_init_pin(BUTTON_BACK_PIN);

    int lastUp = 1, lastDown = 1, lastSel = 1, lastBack = 1;

    while(true) {
        int up = gpio_get_level((gpio_num_t)BUTTON_UP_PIN);
        int down = gpio_get_level((gpio_num_t)BUTTON_DOWN_PIN);
        int sel = gpio_get_level((gpio_num_t)BUTTON_SELECT_PIN);
        int back = gpio_get_level((gpio_num_t)BUTTON_BACK_PIN);

        // only act if menuActive and not salleSelected (handlers check too)
        if(up == 0 && lastUp == 1) {
            // basic debounce: wait small time and confirm still pressed
            vTaskDelay(pdMS_TO_TICKS(20));
            if(gpio_get_level((gpio_num_t)BUTTON_UP_PIN) == 0) {
                handle_up_press();
                // wait release
                while(gpio_get_level((gpio_num_t)BUTTON_UP_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
                vTaskDelay(pdMS_TO_TICKS(40));
            }
        }

        if(down == 0 && lastDown == 1) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if(gpio_get_level((gpio_num_t)BUTTON_DOWN_PIN) == 0) {
                handle_down_press();
                while(gpio_get_level((gpio_num_t)BUTTON_DOWN_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
                vTaskDelay(pdMS_TO_TICKS(40));
            }
        }

        if(sel == 0 && lastSel == 1) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if(gpio_get_level((gpio_num_t)BUTTON_SELECT_PIN) == 0) {
                handle_select_press();
                while(gpio_get_level((gpio_num_t)BUTTON_SELECT_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
                vTaskDelay(pdMS_TO_TICKS(40));
            }
        }

        if(back == 0 && lastBack == 1) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if(gpio_get_level((gpio_num_t)BUTTON_BACK_PIN) == 0) {
                handle_back_press();
                while(gpio_get_level((gpio_num_t)BUTTON_BACK_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(10));
                vTaskDelay(pdMS_TO_TICKS(40));
            }
        }

        lastUp = up; lastDown = down; lastSel = sel; lastBack = back;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
