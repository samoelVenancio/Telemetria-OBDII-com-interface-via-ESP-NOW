/**
 * enlace_espnow.h — Enlace ESP-NOW do Módulo B: recebe telemetria do
 * Módulo A e envia a ele comandos de configuração.
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

/* Sobe Wi-Fi STA no canal fixo do contrato e registra o callback de recepção.
 * Pré-requisito: NVS inicializada e estado_iniciar() já chamado. */
esp_err_t enlace_espnow_iniciar(void);

/* Monta um telem_comando_t (magic, versão, seq, CRC) e envia ao Módulo A.
 * Pode ser chamada da task do LVGL (esp_now_send é thread-safe). */
esp_err_t enlace_espnow_enviar_comando(uint8_t comando, uint16_t valor);
