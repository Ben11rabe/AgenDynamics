#include "wifi/wifi.h"
#include <cstring>
#include "esp_log.h"

// WiFi
#define WIFI_SSID "ASUS_ZenBook"
#define WIFI_PASS "kingkong"

static const char *TAG = "WIFI_MODULE";


// ---- Fonctions Wi-Fi ----
void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    ESP_LOGI(TAG, "Connexion au Wi-Fi...");
    esp_wifi_connect();

    // Attente de la connexion
    for (int i = 0; i < 20; i++) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "Wi-Fi connecté à : %s", ap_info.ssid);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGE(TAG, "Connexion Wi-Fi échouée !");
}