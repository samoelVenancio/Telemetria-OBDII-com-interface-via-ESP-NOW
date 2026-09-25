/**
 * nvm_config.h — Configuração persistente do Módulo B (NVS).
 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int16_t  limiar_temp_d;   /* limiar do alarme, décimos de °C */
    int16_t  histerese_d;     /* histerese do alarme, décimos de °C */
    uint8_t  janela_media;    /* tamanho da média móvel, em amostras */
    uint16_t tanque_dl;       /* capacidade do tanque, décimos de litro */
    uint16_t ve_milesimos;    /* eficiência volumétrica × 1000 (espelho do Módulo A) */
} nvm_config_t;

/* Limites de sanidade usados nos ajustes por toque */
#define NVM_LIMIAR_TEMP_MIN_D   800    /* 80,0 °C */
#define NVM_LIMIAR_TEMP_MAX_D   1300   /* 130,0 °C */
#define NVM_HISTERESE_MIN_D     5      /* 0,5 °C */
#define NVM_HISTERESE_MAX_D     100    /* 10,0 °C */
#define NVM_JANELA_MIN          1
#define NVM_JANELA_MAX          32

/* Carrega da NVS; campos ausentes recebem os padrões (primeiro boot) */
void nvm_config_iniciar(void);

/* Configuração corrente (cache em RAM; leitura sem custo de flash) */
const nvm_config_t *nvm_config(void);

/* Valida, grava na NVS e atualiza o cache */
esp_err_t nvm_config_salvar(const nvm_config_t *nova);
