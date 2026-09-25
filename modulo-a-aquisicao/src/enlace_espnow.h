/**
 * enlace_espnow.h — Enlace ESP-NOW do Módulo A: transmite telemetria para o
 * Módulo B e recebe dele comandos de configuração.
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "telemetria_protocolo.h"

/* Sobe Wi-Fi em modo STA no canal fixo do contrato e inicializa o ESP-NOW.
 * Pré-requisito: NVS já inicializada (o Wi-Fi guarda calibração lá). */
esp_err_t enlace_espnow_iniciar(void);

/* Completa o pacote (magic, versão, seq incremental e CRC-16) e transmite.
 * O chamador preenche apenas os campos de dados. */
esp_err_t enlace_espnow_enviar(telem_pacote_t *pacote);

/* Taxa CAN (kbit/s) pedida pelo Módulo B e ainda não consumida, ou 0 se
 * não há pedido. Ler consome o pedido. */
uint16_t enlace_espnow_consumir_taxa_pedida(void);
