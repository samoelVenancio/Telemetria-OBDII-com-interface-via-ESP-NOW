/**
 * ui_pids.h — Tela "PIDS DO ECM" (código, sem EEZ): resultado da varredura
 * J1979 que o Módulo A faz ao conectar no carro. Mostra se cada parâmetro de
 * interesse existe neste ECM (avanço de ignição, sondas O2, lambda de banda
 * larga, ajustes de mistura...) e a grade completa dos PIDs 01..80 do Modo 01.
 * Abre pelo botão "PIDS DO ECM" da tela Configurações; voltar leva a ela.
 *
 * A varredura é MANUAL: o botão VARRER AGORA chama o callback registrado (a
 * ponte manda TELEM_CMD_VARRER_PIDS ao Módulo A). Sem resposta em 8 s, a tela
 * avisa que o Módulo A não atendeu (dormindo ou fora de alcance).
 */
#pragma once

#include <stdbool.h>

#include "telemetria_protocolo.h"
#include "ui_estilo.h"

void ui_pids_iniciar(void);
void ui_pids_abrir(void);

/* Quem envia o pedido ao Módulo A. Retorna false se não conseguiu enviar. */
void ui_pids_ao_pedir_varredura(bool (*pedir)(void));

/* 10 Hz; só trabalha com a tela ativa e só redesenha se a varredura mudou.
 * tem == false: nenhum pacote de PIDs chegou ainda. */
void ui_pids_atualizar(const ui_dados_t *d, const telem_suporte_t *s, bool tem);
