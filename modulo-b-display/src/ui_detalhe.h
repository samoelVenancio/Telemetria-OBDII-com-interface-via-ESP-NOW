/**
 * ui_detalhe.h — Tela de detalhe de uma grandeza (Módulo B): valor atual,
 * gráfico dos últimos 5 min e mínimo / média / máximo desde o boot (ou desde
 * o último "Zerar").
 *
 * Abre ao tocar num campo do Painel principal. O histórico e as estatísticas
 * são gravados para TODAS as grandezas o tempo todo, então o gráfico já
 * aparece cheio ao abrir, qualquer que seja o campo tocado.
 *
 * Contrato com o EEZ (nomes obrigatórios, ver eez/LEIA-ME.md):
 *   tela painel_detalhe (a ÚLTIMA da lista de telas):
 *     return_detalhe (Imagebutton), icon_detalhe (Image 48x48),
 *     titulo_detalhe, valor_detalhe, min_detalhe, med_detalhe, max_detalhe
 *     (Labels), grafico_detalhe (Container), btn_zerar_detalhe (Button)
 *
 * TUDO aqui roda na task do LVGL (chamado de ui_ponte.c).
 */
#pragma once

#include <stdbool.h>

#include "telemetria_protocolo.h"

/* Uma vez, depois de ui_icones_iniciar(), com o lock do LVGL segurado */
void ui_detalhe_iniciar(void);

/* Mostra a tela de detalhe da grandeza (índice do grandeza_t) */
void ui_detalhe_abrir(int grandeza);

/* A cada tick da ponte (10 Hz). dados=false (sem enlace ou sem CAN) grava um
 * buraco no gráfico e não entra nas estatísticas. */
void ui_detalhe_amostrar(const telem_pacote_t *p, bool dados);
