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
    uint8_t  layout[6];       /* grandeza exibida em cada campo do painel (ver ui_ponte.c) */
    uint8_t  alarme_temp_on;  /* 1 = alarme de superaquecimento habilitado */
    uint8_t  alarme_frio_on;  /* 1 = aviso de giro alto com motor frio habilitado */
    uint8_t  eco_on;          /* 1 = aviso de troca de marcha (ECO) habilitado */
    uint16_t eco_rpm;         /* RPM acima do qual o ECO pede troca de marcha */
} nvm_config_t;

#define NVM_LAYOUT_CAMPOS       6      /* campos Value_1..Value_6 do painel principal */

/* Limites de sanidade usados nos ajustes por toque */
#define NVM_LIMIAR_TEMP_MIN_D   800    /* 80,0 °C */
#define NVM_LIMIAR_TEMP_MAX_D   1300   /* 130,0 °C */
#define NVM_HISTERESE_MIN_D     5      /* 0,5 °C */
#define NVM_HISTERESE_MAX_D     100    /* 10,0 °C */
#define NVM_JANELA_MIN          1
#define NVM_JANELA_MAX          32
#define NVM_ECO_RPM_MIN         1500
#define NVM_ECO_RPM_MAX         4500

/* Carrega da NVS; campos ausentes recebem os padrões (primeiro boot) */
void nvm_config_iniciar(void);

/* Configuração corrente (cache em RAM; leitura sem custo de flash) */
const nvm_config_t *nvm_config(void);

/* Valida, grava na NVS e atualiza o cache */
esp_err_t nvm_config_salvar(const nvm_config_t *nova);

/* Grava só o layout do painel (chamado a cada troca numa lista da tela
 * Painel layout). A validade dos índices é responsabilidade de quem chama,
 * que é quem conhece a lista de grandezas. */
esp_err_t nvm_config_salvar_layout(const uint8_t layout[NVM_LAYOUT_CAMPOS]);
