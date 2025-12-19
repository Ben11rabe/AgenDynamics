#pragma once 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

// Buttons
#define BUTTON_UP_PIN 26
#define BUTTON_DOWN_PIN 25
#define BUTTON_SELECT_PIN 17
#define BUTTON_BACK_PIN 16

// Mutex pour protéger l'état du menu
extern SemaphoreHandle_t menuMutex ;

void button_monitor_init_pin(int gpio);

void buttons_task(void *arg);

void handle_up_press();
void handle_down_press();
void handle_select_press();
void handle_back_press();
