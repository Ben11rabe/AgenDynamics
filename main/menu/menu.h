#pragma once 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"

void menu_task(void *arg);
void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data);