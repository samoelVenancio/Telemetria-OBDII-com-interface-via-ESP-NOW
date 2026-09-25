/**
 * display_init.h — Subida do painel RGB, do touch GT911 e do esp_lvgl_port.
 */
#pragma once

#include "esp_err.h"

/* Inicializa painel RGB (duplo framebuffer + bounce buffer), touch GT911 e o
 * esp_lvgl_port com a task do LVGL fixada no core 1. Depois disso, qualquer
 * chamada LVGL de fora da task exige lvgl_port_lock()/unlock(). */
esp_err_t display_iniciar(void);
