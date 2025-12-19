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
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "epdspi.h"
#include "gdew042t2.h"
#include "esp_task_wdt.h"
#include "rc522/rfid.h"
#include "wifi/wifi.h"
#include "planning/planning.h"
#include "display/display.h"
#include "button/button.h"
#include "menu/menu.h"
using std::string;
using std::vector;

const char *TAG = "EPD_PLANNING";
EpdSpi io;
Gdew042t2 display(io);
rc522_handle_t scanner = NULL;

int indexBatiment = 0;
int indexEtage = 0;
int currentStartIndex = 0;  
int highlightedIndex = 0; 

// UID autorisé (exemple Arduino {0x06,0x35,0x23,0xD4} -> 0xD4233506)
const uint32_t ALLOWED_UID32 = 0xD4233506UL;

// ---- Variables menu ----
volatile bool rfid_scanned = false;
volatile uint64_t scanned_serial = 0; 
bool menuActive = false;   // menu activé après UID autorisé
volatile bool salleSelected = false;
int menuState = 0; // 0 = choix bâtiment, 1 = choix étage, 2 = choix salle
int SALLE_ID = 207; 

// Mutex pour protéger l'état du menu
SemaphoreHandle_t menuMutex = NULL;

// APP_MAIN
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
