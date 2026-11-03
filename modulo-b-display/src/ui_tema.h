/**
 * ui_tema.h — Aplica a identidade visual das telas novas às telas que ainda
 * vêm do EEZ (Layout, Configurações, Alarmes e Detalhe), SEM abrir o EEZ:
 *
 *  - fundo preto, fontes Orbitron, cartões, cabeçalho padrão com voltar;
 *  - textos traduzidos (os labels fixos do EEZ são achados pelo texto);
 *  - widgets reposicionados em cartões, usando os ponteiros de objects.*.
 *
 * A lógica não muda: os mesmos objetos continuam ligados pela ponte
 * (dropdowns, sliders, switches, botão Retry, gráfico do detalhe). Só mudam
 * aparência, posição e textos.
 *
 * Chamar UMA vez, no fim de ui_ponte_iniciar() — depois de a ponte ligar os
 * eventos e de ui_detalhe_iniciar() criar o gráfico.
 */
#pragma once

#include "ui_estilo.h"

void ui_tema_aplicar(void);

/* 10 Hz: atualiza ícones de status/sinal dos cabeçalhos e os ícones da tela
 * Layout (mostram a grandeza escolhida em cada campo). */
void ui_tema_atualizar(const ui_dados_t *d);
