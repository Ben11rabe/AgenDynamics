#include <stdio.h>
#include <string>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include <cstring>
#include "tinyxml2.h"
#include "cJSON.h"

#include "epdspi.h"
#include "gdew042t2.h"
#include "Adafruit_GFX.h"
#include "esp_task_wdt.h"
#include "accents/remove_accents.h"

using namespace tinyxml2;
using std::string;
using std::vector;

static const char *TAG = "EPD_PLANNING";

EpdSpi io;
Gdew042t2 display(io);

// CONFIG WIFI 
#define WIFI_SSID "ASUS_ZenBook"
#define WIFI_PASS "kingkong"

// change ici pour la salle voulue
#define SALLE_ID 207 // A304 - LEPRINCE-RINGUET

// ---------------- Liste des salles ----------------
// Tu peux ajouter / supprimer des paires id->nom si tu veux les utiliser localement.
struct SalleInfo { int id; const char* name; };
static const SalleInfo salles[] = {
    {169, "B008/B009 - FERMI-DIRAC"}, {170, "B008 - FERMI"}, {172, "B009 - DIRAC"}, {173, "A107 - TURING"},
    {174, "A108 - BABBAGE"}, {175, "B005 - EULER"}, {176, "B006 - WEIERSTRASS"}, {177, "B108 - TAYLOR"},
    {178, "B111 - RIEMANN"}, {179, "B113 - PASCAL"}, {180, "B114 - CAUCHY"}, {181, "B115 - FOURIER"},
    {182, "B116 - DESCARTES"}, {183, "B118 - GAUSS"}, {184, "B404 - RAMANUJAN"}, {185, "B405 - GALOIS"},
    {186, "A204 - WATT"}, {187, "A205 - BLONDEL"}, {188, "A206 - FLOYD"}, {189, "A207 - HOARE"},
    {190, "A208 - von NEUMANN"}, {191, "A209 - BOOLE"}, {192, "B119 - BRAGG"}, {193, "B204 - BODE"},
    {195, "B209 - VOLTA"}, {196, "B211 - CARNOT"}, {197, "B219 - MAXWELL"}, {198, "B304 - AMPERE"},
    {200, "B305 - FRESNEL"}, {201, "B306 - MICHELSON"}, {202, "B308 - FARADAY"}, {203, "B309 - LAPLACE"},
    {204, "B313 - BRANLY"}, {205, "B315 - SHAKESPEARE"}, {206, "A303 - HEISENBERG"},
    {207, "A304 - LEPRINCE-RINGUET"}, {208, "A305 - SHANNON"}, {209, "A306 - PLANCK"}, {210, "A307 - EINSTEIN"},
    {211, "A308 - SCHRODINGER"}, {212, "A314 - LANDAU"}, {213, "A315 - NEWTON"}, {214, "A316 - LANGEVIN"},
    {215, "A401 - BELL"}, {216, "A402 - BERNOULLI"}, {217, "A403 - COULOMB"}, {218, "A404 - CURIE"},
    {219, "A405 - MEITNER"}, {220, "A411 - BOHR"}, {222, "A412 - KELVIN"}, {223, "A413 - GALILEE"},
    {224, "B007 - BROGLIE"}, {225, "B104 - KEPLER"}, {226, "B310 - WIENER"}, {227, "B311 - JOULE"},
    {228, "B312 - SIEMENS"}, {229, "B314 - EDISON"}, {230, "B316 - DICKENS"}, {231, "B317 - TESLA"},
    {232, "C205 - MARCONI"}, {233, "C206 - LAENNEC"}, {234, "C207 - KALMAN"}, {235, "D002 - JEANNETEAU"},
    {236, "DS02 - Auditorium de l'ANJOU"}, {247, "B205 - NYQUIST"}, {252, "A401 - BELL - T1"}, {253, "A401 - BELL - T2"},
    {254, "A402 - BERNOULLI - T1"}, {255, "A402 - BERNOULLI - T2"}, {256, "A403 - COULOMB - T1"}, {257, "A403 - COULOMB - T2"},
    {258, "A404 - CURIE - T1"}, {259, "A404 - CURIE - T2"}, {260, "A304 - LEPRINCE-RINGUET - T1"},
    {261, "A304 - LEPRINCE-RINGUET - T2"}, {262, "A305 - SHANNON - T1"}, {263, "A305 - SHANNON - T2"},
    {264, "A306 - PLANCK - T1"}, {265, "A306 - PLANCK - T2"}, {266, "A307 - EINSTEIN - T1"},
    {267, "A307 - EINSTEIN - T2"}, {268, "A308 - SCHRODINGER - T1"}, {269, "A308 - SCHRODINGER - T2"},
    {277, "B309 - LAPLACE T2"}, {278, "B309 - LAPLACE T1"}
};

// URL DE L'API 
#define TO_STR2(x) #x
#define TO_STR(x) TO_STR2(x)

#define API_URL "http://reverse-proxy.eseo.fr/API-SP/API/planning/salles/" TO_STR(SALLE_ID)

// ---- Structure pour un cours ----
struct Cours {
    string libelle;
    string prof;
    string debut;
    string fin;
};

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

// ---- Téléchargement XML ----
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR: ESP_LOGE(TAG,"HTTP_EVENT_ERROR"); break;
        case HTTP_EVENT_ON_CONNECTED: ESP_LOGI(TAG,"HTTP_EVENT_ON_CONNECTED"); break;
        case HTTP_EVENT_HEADER_SENT: ESP_LOGI(TAG,"HTTP_EVENT_HEADER_SENT"); break;
        case HTTP_EVENT_ON_HEADER: ESP_LOGI(TAG,"HTTP_EVENT_ON_HEADER"); break;
        case HTTP_EVENT_ON_DATA: ESP_LOGI(TAG,"HTTP_EVENT_ON_DATA"); break;
        case HTTP_EVENT_ON_FINISH: ESP_LOGI(TAG,"HTTP_EVENT_ON_FINISH"); break;
        case HTTP_EVENT_DISCONNECTED: ESP_LOGI(TAG,"HTTP_EVENT_DISCONNECTED"); break;
        case HTTP_EVENT_REDIRECT: ESP_LOGI(TAG,"HTTP_EVENT_REDIRECT"); break; // <-- ajouté
        default: ESP_LOGW(TAG,"HTTP_EVENT_UNKNOWN"); break; // pour attraper tout le reste
    }
    return ESP_OK;
}

#include "esp_http_client.h"
#include "esp_log.h"

#define TAG "EPD_PLANNING"


// Fonction pour récupérer le planning depuis le serveur
std::string getPlanningFromServer() {
    std::string buffer; // Ici on va accumuler toutes les données reçues

    esp_http_client_config_t config = {};
    config.url = API_URL;
    config.event_handler = [](esp_http_client_event_t *evt) -> esp_err_t {
        switch(evt->event_id) {
            case HTTP_EVENT_ON_DATA:
                if (evt->data_len) {
                    // Accumuler les chunks reçus
                    std::string* resp = (std::string*)evt->user_data;
                    resp->append((char*)evt->data, evt->data_len);
                }
                break;
            case HTTP_EVENT_ERROR:
                ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
                break;
            case HTTP_EVENT_ON_CONNECTED:
                ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
                break;
            case HTTP_EVENT_HEADER_SENT:
                ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
                break;
            case HTTP_EVENT_ON_HEADER:
                ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
                break;
            case HTTP_EVENT_ON_FINISH:
                ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
                break;
            case HTTP_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
                break;
            default:
                break;
        }
        return ESP_OK;
    };
    config.user_data = &buffer;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Erreur HTTP : %s", esp_err_to_name(err));
        buffer.clear(); // Pas de données si erreur
    } else {
        ESP_LOGI(TAG, "HTTP OK, taille des données: %d", buffer.length());
        ESP_LOGI(TAG, "Réponse: %s", buffer.c_str()); // Affiche le JSON reçu
    }

    esp_http_client_cleanup(client);
    return buffer;
}

// ---- Parsing XML ----
#include "cJSON.h"

vector<Cours> parsePlanning(const std::string &jsonStr, std::string &salle) {
    vector<Cours> coursList;

    cJSON *root = cJSON_Parse(jsonStr.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Erreur parsing JSON");
        return coursList;
    }

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

//extraire l'heure du fichier xml au format "HH:MM"
std::string extractHour(const std::string &datetime) {
    size_t pos = datetime.find('T');
    if (pos == std::string::npos || pos + 6 > datetime.size()) return "";
    return datetime.substr(pos + 1, 5); // "17:00"  
}

//chercher à couper le texte si trop long 
std::string wrapSimple(const std::string &txt) { 
    size_t cut = txt.rfind(' '); 
    if (cut == std::string::npos) 
    return txt; // aucun espace → pas de wrap 
    return txt.substr(0, cut) + "\n" + txt.substr(cut + 1); 
}

// ---- Affichage e-paper ----
void displayPlanning(const string &salle, const vector<Cours> &coursList) {
    display.fillScreen(EPD_WHITE);
    display.setTextColor(EPD_BLACK);

    // Taille écran Gdew042t2
    const int W = 400;
    const int H = 300;

    // ---- TITRE : salle ----
    display.setTextSize(2);
    std::string titre = salle;
    // Mesurer largeur du texte
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(titre.c_str(), 0, 0, &x1, &y1, &w, &h);
    // Centrage horizontal (400 px = largeur écran)
    int x_center = (400 - w) / 2;
    int y = 25;
    display.setCursor(x_center, y);
    display.println(titre); 

    // ---- Cadre principal ----
    int top = 50; //Espace pour le titre
    int margin = 10;
    int frameH = H - top - margin;
    //int half = frameH / 2;
    int third = top + int(0.35 * frameH);

    // Rectangle total
    display.drawRect(margin, top, W - 2*margin, frameH, EPD_BLACK);

    // Ligne au milieu
    display.drawLine(margin, top + third, W - margin, top + third, EPD_BLACK);
   
    // ---- Contenu des cadres de cours ----
    display.setTextSize(2);

    // Fonction lambda pour centrer du texte
    auto printCentered = [&](const std::string &text, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    int x_center = (400 - w) / 2;
    display.setCursor(x_center, y);
    display.println(text.c_str());
    };

    // Cours actuel = index 0
    if (coursList.size() >= 1) {
        const Cours& c = coursList[0];
        std::string hDebut = extractHour(c.debut);
        std::string hFin   = extractHour(c.fin);

        std::string libelleClean = removeAccents(c.libelle);
        std::string profClean    = removeAccents(c.prof);

        int x = margin + 10;
        int y = top + 5;

        printCentered("COURS ACTUEL:", y);
       
        // ---- Nom du cours sur 2 lignes ----
        std::string wrapped = wrapSimple(libelleClean);
        std::vector<std::string> lines;
        size_t pos = 0;
        size_t next;
        while ((next = wrapped.find('\n', pos)) != std::string::npos) {
            lines.push_back(wrapped.substr(pos, next - pos));
            pos = next + 1;
        }
        lines.push_back(wrapped.substr(pos));

        // Affichage du cours centré
        int yText = y + 25;
        for (const auto &line : lines) {
            printCentered(line, yText);
            yText += 25; // espacement vertical entre lignes du cours
        }

        // ---- Nom du professeur centré ----
        yText += 5; // petit espace après le cours
        printCentered(profClean, yText);

        // ---- Heures ----
        display.setCursor(x, yText + 30);
        display.println(hDebut.c_str());
        display.setCursor(W - margin - 70, yText + 30);
        display.println(hFin.c_str());  
    }

    // Cours suivant = index 1
    if (coursList.size() >= 2) {
        const Cours& c = coursList[1];
        std::string hDebut = extractHour(c.debut);
        std::string hFin   = extractHour(c.fin);

        std::string libelleClean = removeAccents(c.libelle);
        std::string profClean    = removeAccents(c.prof);

        int x = margin + 10;
        int y = top + third + 5;

        printCentered("COURS SUIVANT:", y);
       
        // ---- Nom du cours sur 2 lignes ----
        std::string wrapped = wrapSimple(libelleClean);
        std::vector<std::string> lines;
        size_t pos = 0;
        size_t next;
        while ((next = wrapped.find('\n', pos)) != std::string::npos) {
            lines.push_back(wrapped.substr(pos, next - pos));
            pos = next + 1;
        }
        lines.push_back(wrapped.substr(pos));

        // Affichage du cours centré
        int yText = y + 25;
        for (const auto &line : lines) {
            printCentered(line, yText);
            yText += 25; // espacement vertical entre lignes du cours
        }

          // ---- Heures ----
        display.setCursor(x, yText + 5);
        display.println(hDebut.c_str());
        display.setCursor(W - margin - 70, yText + 5);
        display.println(hFin.c_str());
    }

    display.update();
}


// ---- Main ----
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initialisation...");

    nvs_flash_init();
    wifi_init_sta();

    esp_task_wdt_deinit();
    display.init();
    display.fillScreen(EPD_WHITE);

    while(true) {
        string response = getPlanningFromServer();
        if(response.empty()) {
            ESP_LOGE(TAG, "Aucune donnée reçue ou erreur HTTP");
            display.fillScreen(EPD_WHITE);
            display.setCursor(10,40);
            display.setTextSize(2);
            display.println("Erreur API ou connexion");
            display.update();
        } else {
            string salle;
            vector<Cours> coursList = parsePlanning(response, salle);
            if(coursList.empty()) {
                display.fillScreen(EPD_WHITE);
                display.setCursor(10,40);
                display.setTextSize(2);
                display.println("Planning vide ou XML invalide");
                display.update();
            } else {
                displayPlanning(salle, coursList);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300000)); // 5 minutes
    }
}

