/**
 * grandeza.h — Grandezas que podem ocupar um campo do painel.
 *
 * A ORDEM é a das opções nas listas da tela Painel layout e é o que fica
 * gravado na NVS — acrescentar no fim é seguro; reordenar embaralha o layout
 * salvo de quem já usa. As tabelas de ui_icones.c e ui_detalhe.c seguem esta
 * mesma ordem.
 */
#pragma once

typedef enum {
    G_RPM = 0,
    G_VELOCIDADE,
    G_CONSUMO,
    G_ETANOL,
    G_TEMP_MOTOR,
    G_TEMP_AR,
    G_MAP,
    G_TANQUE_L,
    G_TANQUE_PCT,
    G_BATERIA,
    G_TOTAL,
} grandeza_t;
