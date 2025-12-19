#include "planning/planning.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <vector>      
#include "gdew042t2.h"
#include "epdspi.h"
#include "accents/remove_accents.h"
using std::vector;
using std::string;

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

// extractHour : extrait l'heure au format HH:MM depuis "YYYY-MM-DD HH:MM:SS"
std::string extractHour(const std::string &datetime) {
    if (datetime.size() >= 16) {
        return datetime.substr(11, 5);
    }
    return datetime;
}

// wrapSimple : découpe le texte en lignes de longueur max lineLength
std::string wrapSimple(const std::string &text, size_t lineLength = 25) {
    std::string result;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t len = std::min(lineLength, text.size() - pos);
        result += text.substr(pos, len) + '\n';
        pos += len;
    }
    return result;
}

void displayPlanning(Gdew042t2 &display, const std::string &salle, const std::vector<Cours> &coursList) {
    // Nettoyer l'écran
    display.fillScreen(EPD_WHITE);
    display.setTextColor(EPD_BLACK);
    display.setTextSize(2);

    // Dimensions de l'écran
    const int W = 400;
    const int H = 300;

    // Centrer le titre
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(salle.c_str(), 0, 0, &x1, &y1, &w, &h);
    display.setCursor((W - w) / 2, 20);
    display.println(salle.c_str());

    // Paramètres de cadre
    int top = 50;
    int margin = 10;
    int frameH = H - top - margin;
    int third = top + int(0.35 * frameH);

    // Dessiner le cadre et la ligne séparatrice
    display.drawRect(margin, top, W - 2 * margin, frameH, EPD_BLACK);
    display.drawLine(margin, top + third, W - margin, top + third, EPD_BLACK);

    // Fonction lambda pour centrer du texte
    auto printCentered = [&](const std::string &text, int y){
        int16_t x1, y1; uint16_t w, h;
        display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
        display.setCursor((W - w) / 2, y);
        display.println(text.c_str());
    };

    // Afficher le cours actuel (si disponible)
    if (coursList.size() >= 1) {
        const Cours &c = coursList[0];
        std::string hDebut = extractHour(c.debut);
        std::string hFin = extractHour(c.fin);
        std::string lib = removeAccents(c.libelle);
        std::string prof = removeAccents(c.prof);

        int y = top + 5;
        printCentered("COURS ACTUEL:", y);

        std::string wrapped = wrapSimple(lib);
        int yText = y + 25;
        size_t pos = 0, next;
        while ((next = wrapped.find('\n', pos)) != std::string::npos) {
            printCentered(wrapped.substr(pos, next - pos), yText);
            yText += 25;
            pos = next + 1;
        }
        printCentered(wrapped.substr(pos), yText); yText += 25;
        printCentered(prof, yText);

        display.setCursor(margin + 10, yText + 30); display.println(hDebut.c_str());
        display.setCursor(W - margin - 70, yText + 30); display.println(hFin.c_str());
    }

    // Afficher le cours suivant (si disponible)
    if (coursList.size() >= 2) {
        const Cours &c = coursList[1];
        std::string hDebut = extractHour(c.debut);
        std::string hFin = extractHour(c.fin);
        std::string lib = removeAccents(c.libelle);
        std::string prof = removeAccents(c.prof);

        int y = top + third + 5;
        printCentered("COURS SUIVANT:", y);

        std::string wrapped = wrapSimple(lib);
        int yText = y + 25;
        size_t pos = 0, next;
        while ((next = wrapped.find('\n', pos)) != std::string::npos) {
            printCentered(wrapped.substr(pos, next - pos), yText);
            yText += 25;
            pos = next + 1;
        }
        printCentered(wrapped.substr(pos), yText); yText += 25;

        display.setCursor(margin + 10, yText + 5); display.println(hDebut.c_str());
        display.setCursor(W - margin - 70, yText + 5); display.println(hFin.c_str());
    }

    // Mettre à jour l'affichage
    display.update();
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