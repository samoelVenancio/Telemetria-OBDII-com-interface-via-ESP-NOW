/**
 * estado.h — Estado compartilhado do Módulo B: último pacote de telemetria
 * válido + estatísticas do enlace. Única ponte entre o callback do ESP-NOW
 * (contexto da task do Wi-Fi, core 0) e a UI (task do LVGL, core 1).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "telemetria_protocolo.h"

/* Enlace considerado caído se o último pacote válido tem mais que isto */
#define ESTADO_ENLACE_TIMEOUT_MS  1000

typedef struct {
    int8_t   rssi_dbm;      /* do último pacote recebido */
    uint32_t recebidos;     /* pacotes válidos */
    uint32_t perdidos;      /* buracos detectados no campo seq */
    uint32_t invalidos;     /* descartados por tamanho/magic/versão/CRC */
    int64_t  ultimo_rx_ms;  /* timestamp local do último pacote válido */
} estado_enlace_t;

void estado_iniciar(void);

/* Chamado PELO CALLBACK do ESP-NOW. Valida tamanho, magic, versão e CRC;
 * em caso de sucesso copia o pacote para o estado sob seção crítica.
 * NÃO chama nada de LVGL — ver comentário em estado.c. */
bool estado_processar_rx(const uint8_t *dados, int tamanho, int rssi_dbm);

/* Copia atômica do estado para a UI. Retorna false se nunca chegou pacote. */
bool estado_copiar(telem_pacote_t *pacote, estado_enlace_t *enlace);

/* true se o último pacote válido chegou dentro do timeout do enlace */
bool estado_enlace_ok(void);

/* Zera recebidos/perdidos/inválidos (botão Retry da tela Config): recomeça a
 * medição de entrega sem precisar reiniciar o módulo. */
void estado_zerar_estatisticas(void);
