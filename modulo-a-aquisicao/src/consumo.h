/**
 * consumo.h — Cálculo do consumo instantâneo por speed-density.
 * Isolado do resto para ser testável fora do alvo (não depende de driver).
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "pids.h"

typedef struct {
    uint16_t kml_c;  /* km/L × 100; 0 = parado ou inválido (usar lh_c) */
    uint16_t lh_c;   /* L/h × 100 */
    bool     valido; /* false se faltou insumo (RPM/MAP/IAT/etanol inválidos) */
} consumo_saida_t;

/* Carrega o VE (eficiência volumétrica) da NVS; usa o padrão se vazia */
void consumo_iniciar(void);

/* VE atual (adimensional, ex.: 0.85) */
float consumo_ve(void);

/* Grava um novo VE na NVS e passa a usá-lo. É o principal parâmetro de
 * calibração do projeto — ver comentário em consumo.c. */
esp_err_t consumo_definir_ve(float ve);

/* Calcula consumo instantâneo a partir do estado atual do veículo */
consumo_saida_t consumo_calcular(const dados_veiculo_t *d);
