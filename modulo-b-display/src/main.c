/**
 * main.c — Módulo B (exibição) — Sunton ESP32-S3 800x480.
 *
 * Ordem de subida (importa):
 *  1. NVS (configuração e calibração do Wi-Fi dependem dela);
 *  2. nvm_config + alarme (a UI lê ambos ao ser criada);
 *  3. estado (a ponte precisa existir antes de qualquer pacote chegar);
 *  4. display + LVGL (task no core 1) e criação das telas SOB lvgl_port_lock —
 *     app_main roda fora da task do LVGL, e LVGL não é thread-safe. As telas
 *     vêm do EEZ Studio (src/ui/, gerado) e os dados chegam a elas pela
 *     ponte (ui_ponte.c);
 *  5. ESP-NOW por último: só depois da UI pronta é que faz sentido receber.
 *
 * Divisão de cores: LVGL fixado no core 1 (display_init.c); Wi-Fi/ESP-NOW no
 * core 0 (padrão do IDF). Renderização nunca disputa CPU com recepção.
 */
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_lvgl_port.h"

#include "display_init.h"
#include "enlace_espnow.h"
#include "estado.h"
#include "nvm_config.h"
#include "alarme.h"
#include "ui/ui.h"
#include "ui_ponte.h"

static const char *TAG = "principal";

void app_main(void)
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    nvm_config_iniciar();
    alarme_iniciar();
    estado_iniciar();

    ESP_ERROR_CHECK(display_iniciar());

    /* Criação das telas fora da task do LVGL exige o lock do port */
    if (lvgl_port_lock(0)) {
        ui_init();          /* gerado pelo EEZ: cria as telas e abre a primeira */
        ui_ponte_iniciar(); /* lv_timer de 10 Hz: dados -> variáveis -> ui_tick() */
        lvgl_port_unlock();
    }

    ESP_ERROR_CHECK(enlace_espnow_iniciar());

    ESP_LOGI(TAG, "modulo B no ar — aguardando telemetria");
}
