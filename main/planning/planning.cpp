#include "planning/planning.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <vector>      
#include "gdew042t2.h"
#include "epdspi.h"
#include "accents/remove_accents.h"
using std::vector;

static const char *TAG = "EPD_PLANNING";

// ---------------- HTTP and JSON code (reuse your functions) ----------------
std::string getPlanningFromServer(int salleId) {
    char url[128];
    snprintf(url, sizeof(url), "http://reverse-proxy.eseo.fr/API-SP/API/planning/salles/%d", salleId);
    std::string buffer;
    esp_http_client_config_t config = {};
    config.url = url;
    config.user_data = &buffer;
    config.timeout_ms = 5000;
    config.disable_auto_redirect = false;
    config.buffer_size = 2048;
    config.buffer_size_tx = 2048;
    config.event_handler = [](esp_http_client_event_t *evt) -> esp_err_t {
        if(evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0 && evt->user_data){
            std::string* resp = (std::string*)evt->user_data;
            resp->append((char*)evt->data, evt->data_len);
        }
        return ESP_OK;
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Erreur HTTP : %s", esp_err_to_name(err));
        buffer.clear();
    } else {
        ESP_LOGI(TAG, "Réponse finale : %s", buffer.c_str());
    }
    esp_http_client_cleanup(client);
    return buffer;
}

vector<Cours> parsePlanning(const std::string &jsonStr, std::string &salle) {
    vector<Cours> coursList;
    cJSON *root = cJSON_Parse(jsonStr.c_str());
    if (!root) return coursList;
    cJSON *salleItem = cJSON_GetObjectItem(root, "Salle");
    salle = (salleItem && cJSON_IsString(salleItem)) ? salleItem->valuestring : "Inconnue";
    cJSON *lesCours = cJSON_GetObjectItem(root, "LesCours");
    if (lesCours && cJSON_IsArray(lesCours)) {
        int count = cJSON_GetArraySize(lesCours);
        for (int i = 0; i < count; i++) {
            cJSON *item = cJSON_GetArrayItem(lesCours, i);
            Cours c;
            c.debut = cJSON_GetObjectItem(item, "Debut")->valuestring;
            c.fin = cJSON_GetObjectItem(item, "Fin")->valuestring;
            c.libelle = cJSON_GetObjectItem(item, "Libelle")->valuestring;
            c.prof = cJSON_GetObjectItem(item, "Professeurs")->valuestring;
            coursList.push_back(c);
        }
    }
    cJSON_Delete(root);
    return coursList;
}

void displayPlanning(Gdew042t2 &display, const std::string &salle, const std::vector<Cours> &coursList) {
    display.fillScreen(EPD_WHITE);
    display.setTextSize(2);
    display.setTextColor(EPD_BLACK);
    display.setCursor(10, 10);
    std::string title = "Planning: " + salle;
    display.println(title.c_str());
    int y = 40;
    display.setTextSize(1);
    for (const auto &c : coursList) {
        if (y > 280) break; // éviter overflow
        std::string line1 = c.debut + " - " + c.fin + ": " + removeAccents(c.libelle);
        display.setCursor(10, y);
        display.println(line1.c_str());
        y += 12;
        std::string line2 = "Prof: " + removeAccents(c.prof);
        display.setCursor(20, y);
        display.println(line2.c_str());
        y += 16;
    }
    display.update();
}