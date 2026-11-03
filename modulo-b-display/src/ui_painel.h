/**
 * ui_painel.h — Painel de 6 cartões (código, sem EEZ). Cada cartão mostra a
 * grandeza escolhida na tela Layout: ícone, nome, valor grande e a barra que
 * combina com ela (faixas, segmentos ou mistura). Tocar num cartão chama o
 * callback registrado (a ponte abre a tela de detalhe com o gráfico).
 */
#pragma once

#include "ui_estilo.h"

void ui_painel_iniciar(void);
void ui_painel_abrir(void);
void ui_painel_atualizar(const ui_dados_t *d);   /* 10 Hz; só se ativa */
void ui_painel_ao_tocar_campo(void (*cb)(int campo, int grandeza));
