/**
 * ui_menu.h — Menu principal em blocos (código, sem EEZ): Cockpit, Painel,
 * Layout, Alarmes, Configurações e um bloco de enlace com a entrega e o RSSI
 * ao vivo. Cada bloco chama est_ir() com o destino.
 */
#pragma once

#include "ui_estilo.h"

void ui_menu_iniciar(void);
void ui_menu_abrir(void);
void ui_menu_atualizar(const ui_dados_t *d);   /* 10 Hz; só se ativa */
