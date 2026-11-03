/**
 * ui_icones.h — Ícones do painel (Módulo B): qual ícone aparece em cada
 * campo, ícone colorido da linha de status e barras de sinal do ESP-NOW.
 *
 * Os bitmaps vêm do EEZ Studio (img_ic_*, formato A8: só alfa). A cor é
 * aplicada aqui em tempo de execução pelo "image recolor", então um único
 * PNG branco serve para OK (verde), atenção (âmbar) e alarme (vermelho).
 *
 * Contrato com o EEZ (nomes obrigatórios, ver eez/GUIA_ICONES.md):
 *   tela Painel principal: icon_v1..icon_v6 (Image 48x48),
 *                          icon_status (Image 32x32), icon_sinal (Image 32x32)
 *   bitmaps com "Always build": ver ferramentas/icones/injetar_bitmaps_eez.py
 *
 * TUDO aqui roda na task do LVGL (chamado de ui_ponte.c). Nada de chamar do
 * callback do ESP-NOW.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

/* Número de grandezas selecionáveis em cada campo. Tem que bater com G_TOTAL
 * de ui_ponte.c — lá há um _Static_assert que acusa a divergência. */
#define UI_ICONES_GRANDEZAS 10

/* Estados da linha de status, na MESMA prioridade de atualizar_painel() */
typedef enum {
    UI_STATUS_SEM_ENLACE = 0,   /* sem pacote do Módulo A há > 1 s   */
    UI_STATUS_SEM_CAN,          /* enlace ok, mas dados do CAN inválidos */
    UI_STATUS_SUPERAQUECIMENTO, /* alarme de temperatura ativo        */
    UI_STATUS_MOTOR_FRIO,       /* < 60 °C e > 3000 rpm               */
    UI_STATUS_ECO,              /* sugestão de troca de marcha        */
    UI_STATUS_OK,
} ui_status_t;

/* Uma vez, depois de ui_init(), com o lock do LVGL segurado. Pinta de branco
 * todos os ícones fixos colocados no EEZ (menu, alarmes, Retry...) e põe os
 * dinâmicos num estado inicial neutro. */
void ui_icones_iniciar(void);

/* Ícone do campo 0..5 conforme a grandeza escolhida no layout (índice do
 * grandeza_t). alerta=true pinta de vermelho (temperatura em alarme). */
void ui_icones_campo(int campo, int grandeza, bool alerta);

/* Bitmap 48 px da grandeza (índice do grandeza_t) — usado na tela de detalhe */
const lv_image_dsc_t *ui_icones_da_grandeza(int grandeza);

void ui_icones_status(ui_status_t status);

/* Barras de sinal a partir do RSSI do último pacote */
void ui_icones_sinal(bool enlace_ok, int rssi_dbm);
