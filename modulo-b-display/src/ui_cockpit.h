/**
 * ui_cockpit.h — Tela "cockpit" (código, sem EEZ): arco de RPM com a
 * velocidade no centro e, à direita, cinco linhas com barra: motor, bateria,
 * combustível, etanol e consumo. Toque em qualquer ponto vai para o menu.
 */
#pragma once

#include "ui_estilo.h"

void ui_cockpit_iniciar(void);                     /* cria (não carrega)  */
void ui_cockpit_abrir(void);
void ui_cockpit_atualizar(const ui_dados_t *d);    /* 10 Hz; só se ativa  */
