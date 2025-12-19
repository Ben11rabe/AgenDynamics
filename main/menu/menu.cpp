#include "menu.h"
#include "display/display.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "rc522/rfid.h"

extern SemaphoreHandle_t menuMutex;

extern int indexBatiment;
extern int indexEtage;
extern int currentStartIndex;
extern int highlightedIndex;

extern bool menuActive;
extern volatile bool salleSelected;
extern int menuState;

extern Gdew042t2 display;

// RFID
extern volatile bool rfid_scanned;
extern volatile uint64_t scanned_serial;

// TAG log
extern const char* TAG;

// ---- Handler RC522 : signale l'événement ----
void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    if(event_id != RC522_EVENT_TAG_SCANNED) return;
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;
    rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
    scanned_serial = tag->serial_number;
    rfid_scanned = true; // menu_task will handle
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