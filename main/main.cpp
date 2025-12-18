#include <string>
#include <vector>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include <cstring>
#include "tinyxml2.h"
#include "cJSON.h"
#include "epdspi.h"
#include "gdew042t2.h"
#include "Adafruit_GFX.h"
#include "esp_task_wdt.h"
#include "accents/remove_accents.h"
#include "rc522/rfid.h"
#include <inttypes.h>
#include "data/salles.h"
#include "wifi/wifi.h"
#include "planning/planning.h"
#include "display/display.h"


// Buttons
#define BUTTON_UP_PIN 26
#define BUTTON_DOWN_PIN 25
#define BUTTON_SELECT_PIN 17
#define BUTTON_BACK_PIN 16

using namespace tinyxml2;
using std::string;
using std::vector;

static const char *TAG = "EPD_PLANNING";
EpdSpi io;
Gdew042t2 display(io);


// RC522
static rc522_handle_t scanner = NULL;

int indexBatiment = 0;
int indexEtage = 0;
int currentStartIndex = 0;   // première salle affichée (globale)
int highlightedIndex = 0; 

// UID autorisé (exemple Arduino {0x06,0x35,0x23,0xD4} -> 0xD4233506)
static const uint32_t ALLOWED_UID32 = 0xD4233506UL;



// ---- Variables menu ----
static volatile bool rfid_scanned = false;
static volatile uint64_t scanned_serial = 0; // rempli depuis handler

bool menuActive = false;   // menu activé après UID autorisé
bool salleSelected = false;

int menuState = 0; // 0 = choix bâtiment, 1 = choix étage, 2 = choix salle

int SALLE_ID = 207; // valeur finale

// Mutex pour protéger l'état du menu
SemaphoreHandle_t menuMutex = NULL;




// ---- Handler RC522 : signale l'événement ----
static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    if(event_id != RC522_EVENT_TAG_SCANNED) return;
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
    scanned_serial = tag->serial_number;
    rfid_scanned = true; // menu_task will handle
}

void planning_task(void *arg)
{
    while (true)
    {
        if (salleSelected) {
            string response = getPlanningFromServer(SALLE_ID);
            if (!response.empty()) {
                string salleName;
                vector<Cours> coursList = parsePlanning(response, salleName);
                displayPlanning(display,salleName, coursList);
            }
        }

        // rafraîchir toutes les 30 secondes (ou autre)
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

// ---- Bouton init ----
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

            displayClearAndTextCentered(display,140, "Salle sélectionnée !");
            vTaskDelay(pdMS_TO_TICKS(800));
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


// ---- Menu manager task: traite l'événement RFID scanné ----
void menu_task(void *arg) {
    while(true){
          if (rfid_scanned && !menuActive && salleSelected) {
            salleSelected = false;      // on arrête l'affichage continu de planning
            // On NE reset PAS tout ici — on laisse le if suivant gérer l'ouverture du menu
        }
                if(rfid_scanned && !menuActive){
                    uint64_t serial = scanned_serial;
                    rfid_scanned = false;

                    uint32_t low32 = (uint32_t)(serial & 0xFFFFFFFFULL);
                    ESP_LOGI(TAG, "Tag scanned serial=%" PRIu64 " low32=0x%" PRIX32, serial, low32);

                    if(xSemaphoreTake(menuMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        ESP_LOGI(TAG, "Tag scanné, activation menu");
                        menuActive = true;
                        menuState = 0;
                        indexBatiment = 0;
                        indexEtage = 0;
                        currentStartIndex = 0;
                        highlightedIndex = 0;
                        xSemaphoreGive(menuMutex);
                        displayMenuBuildings(display, indexBatiment);
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ---- app_main ----
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initialisation...");

    nvs_flash_init();
    wifi_init_sta();
    esp_task_wdt_deinit();
    display.init();

    // Affichage initial
    displayMaintenanceMode(display);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // create mutex
    menuMutex = xSemaphoreCreateMutex();
    if(menuMutex == NULL) {
        ESP_LOGE(TAG, "Impossible de créer menuMutex");
        // continue but behavior unsafe; ideally handle error
    }

    // Créer la task menu (gère l'activation suite au scan)
    xTaskCreate(menu_task, "menu_task", 4096, NULL, 6, NULL);

    // Création d'une seule tâche boutons (évite race conditions)
    xTaskCreate(buttons_task, "buttons_task", 4096, NULL, 5, NULL);

    // Création tâche planning (récupération et affichage périodique)
    xTaskCreate(planning_task, "planning_task", 4096, NULL, 5, NULL);

    // Initialisation RC522 (après affichage de départ pour éviter override)
    rc522_config_t config = {};
    config.scan_interval_ms = RC522_DEFAULT_SCAN_INTERVAL_MS;
    config.task_stack_size = RC522_DEFAULT_TASK_STACK_SIZE;
    config.task_priority   = RC522_DEFAULT_TASK_STACK_PRIORITY;
    config.transport = RC522_TRANSPORT_SPI;
    config.spi.host = VSPI_HOST;
    config.spi.miso_gpio = 19;
    config.spi.mosi_gpio = 23;
    config.spi.sck_gpio  = 18;
    config.spi.sda_gpio  = 5;
    config.spi.clock_speed_hz = RC522_DEFAULT_SPI_CLOCK_SPEED_HZ;
    config.spi.device_flags   = SPI_DEVICE_HALFDUPLEX;
    if(rc522_create(&config, &scanner) != ESP_OK){
        ESP_LOGE(TAG, "Erreur creation RC522");
    } else {
        rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
        rc522_start(scanner);
    }

    while(true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
