#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "MFRC522.h"

// Définition des pins
#define SS_PIN    5   // SDA / CS
#define RST_PIN   4  // Reset
#define MOSI_PIN  23  // MOSI SPI
#define MISO_PIN  19  // MISO SPI
#define SCK_PIN   18  // Clock SPI

static const char* TAG = "RC522_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "Initialisation RC522 avec toutes les pins...");

    // Initialisation du RC522
    MFRC522 rc522(SS_PIN, RST_PIN, MOSI_PIN, MISO_PIN, SCK_PIN);
    rc522.begin();

    while(true) {
        if(rc522.isNewCardPresent() && rc522.readCardSerial()) {
            ESP_LOGI(TAG, "Badge détecté !");
            ESP_LOGI(TAG, "UID : %02X %02X %02X %02X",
                     rc522.uid.uidByte[0],
                     rc522.uid.uidByte[1],
                     rc522.uid.uidByte[2],
                     rc522.uid.uidByte[3]);
        }

        vTaskDelay(pdMS_TO_TICKS(200)); // Vérification toutes les 200 ms
    }
}
