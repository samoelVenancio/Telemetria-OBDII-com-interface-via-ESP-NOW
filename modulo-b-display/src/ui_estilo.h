/**
 * ui_estilo.h — Identidade visual das telas feitas em código (Módulo B).
 *
 * Paleta, fontes Orbitron, componentes (cabeçalho, cartão, barras) e a
 * descrição de cada grandeza: nome, unidade, faixa, tipo de barra e cor de
 * estado. Cockpit, painel e menu usam tudo daqui, então mudar uma cor ou uma
 * faixa aqui muda todas as telas de uma vez.
 *
 * TUDO aqui roda na task do LVGL.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"
#include "grandeza.h"
#include "ui_icones.h"     /* ui_status_t */
#include "ui_icones_k.h"   /* img_k32_*, img_k64_* */

/* ---------- Paleta ---------- */
#define EST_FUNDO       0x000000
#define EST_CARTAO      0x0D1114
#define EST_BORDA       0x232A30
#define EST_TRILHO      0x1A1F24
#define EST_ROTULO      0x8A96A3
#define EST_ESCALA      0x56606A
#define EST_TEXTO       0xF2F4F5
#define EST_APAGADO     0x4A525A
#define EST_AZUL        0x2F7BF5
#define EST_LARANJA     0xF28C28
#define EST_VERMELHO    0xE5484D
#define EST_VERDE       0x46A758
#define EST_AMBAR       0xE5A04D

/* ---------- Fontes (src/fontes/, Orbitron OFL 1.1) ---------- */
LV_FONT_DECLARE(ui_font_orb_80);
LV_FONT_DECLARE(ui_font_orb_64);
LV_FONT_DECLARE(ui_font_orb_56);
LV_FONT_DECLARE(ui_font_orb_40);
LV_FONT_DECLARE(ui_font_orb_22);
LV_FONT_DECLARE(ui_font_orb_16);

/* ---------- Dados que as telas mostram ---------- */
typedef struct {
    bool        enlace_ok;      /* pacote válido há menos de 1 s            */
    bool        dados;          /* enlace ok e TELEM_FLAG_DADOS_VALIDOS     */
    uint16_t    rpm;
    uint8_t     velocidade;     /* km/h                                     */
    int16_t     temp_d;         /* líquido de arrefecimento, décimos de °C  */
    int16_t     iat_d;          /* ar admitido, décimos de °C               */
    uint16_t    map_kpa;
    uint16_t    tensao_c;       /* centésimos de V                          */
    uint16_t    consumo_kml_c;  /* km/L × 100 (0 = parado, usar L/h)        */
    uint16_t    consumo_lh_c;   /* L/h × 100                                */
    uint8_t     etanol_pct;
    uint8_t     nivel_pct;
    uint16_t    litros_d;       /* décimos de litro                         */
    int16_t     limiar_temp_d;  /* limiar do alarme (NVS), décimos de °C    */
    bool        alarme_temp;    /* alarme de superaquecimento ativo         */
    ui_status_t status;         /* mesma prioridade da linha de status      */
    const char *msg_status;     /* texto da linha de status da ponte        */
    int         rssi_dbm;
    uint8_t     entrega_pct;    /* entrega do enlace desde o último Retry   */
    uint8_t     layout[6];      /* grandeza de cada campo do painel         */
} ui_dados_t;

/* ---------- Navegação ----------
 * As telas não sabem como abrir as outras: chamam est_ir(), e a ponte
 * (ui_ponte.c) registra quem atende com est_definir_navegacao(). */
typedef enum {
    UI_DEST_MENU = 0,
    UI_DEST_COCKPIT,
    UI_DEST_PAINEL,
    UI_DEST_LAYOUT,
    UI_DEST_ALARMES,
    UI_DEST_CONFIG,
    UI_DEST_PIDS,       /* PIDs suportados pelo ECM (botão em Configurações) */
} ui_destino_t;
void est_definir_navegacao(void (*ir)(ui_destino_t));
void est_ir(ui_destino_t destino);
void est_ao_voltar_menu(lv_event_t *e);   /* pronto para usar como callback */

/* ---------- Construção ---------- */
lv_obj_t *est_tela(void);
lv_obj_t *est_texto(lv_obj_t *pai, const lv_font_t *f, uint32_t cor, const char *t);
lv_obj_t *est_icone(lv_obj_t *pai, const lv_image_dsc_t *img, uint32_t cor);
lv_obj_t *est_cartao(lv_obj_t *pai, int x, int y, int w, int h);
lv_obj_t *est_linha(lv_obj_t *pai, int x, int y, int w, int h);
void      est_pintar_icone(lv_obj_t *img, uint32_t cor);

/* Cabeçalho padrão (64 px): seta de voltar, título, ícones de status e de
 * sinal à direita. voltar == NULL: sem seta (ex.: menu). */
typedef struct {
    lv_obj_t *status;
    lv_obj_t *sinal;
} est_cabecalho_t;
void est_cabecalho(lv_obj_t *tela, const char *titulo, lv_event_cb_t voltar,
                   est_cabecalho_t *out);
void est_cabecalho_atualizar(est_cabecalho_t *c, const ui_dados_t *d);

/* ---------- Barras ----------
 * Um único objeto "barra" serve aos quatro tipos; o tipo vem da grandeza:
 *   FAIXAS    trilho + indicador colorido + régua de faixas embaixo (+ marca)
 *   SIMPLES   trilho + indicador
 *   SEGMENTOS 10 blocos, como marcador de combustível
 *   MISTURA   barra inteira dividida etanol × gasolina                      */
typedef enum { BARRA_SIMPLES, BARRA_FAIXAS, BARRA_SEGMENTOS, BARRA_MISTURA } est_tipo_barra_t;

typedef struct {
    lv_obj_t *pai;
    int x, y;
    int16_t limiar;         /* FAIXAS da temperatura: refaz a régua se mudar */
    lv_obj_t *raiz;
    lv_obj_t *ind;          /* SIMPLES/FAIXAS/MISTURA */
    lv_obj_t *seg[10];      /* SEGMENTOS */
    lv_obj_t *marca;        /* FAIXAS com limiar */
    int grandeza;
    int w;
    int32_t ultimo;
    uint32_t ultima_cor;
    bool valido;
} est_barra_t;

void est_barra_criar(est_barra_t *b, lv_obj_t *pai, int x, int y, int w, int grandeza,
                     const ui_dados_t *d);
void est_barra_atualizar(est_barra_t *b, const ui_dados_t *d);

/* ---------- Grandezas ---------- */
const char *est_nome(int g);                     /* "TEMPERATURA DO MOTOR" */
const char *est_nome_curto(int g);               /* "MOTOR"                */
const lv_image_dsc_t *est_icone_32(int g);
est_tipo_barra_t est_tipo_barra(int g);

/* Valor e unidade formatados para exibir ("90" / "°C"). */
void est_valor(int g, const ui_dados_t *d, char *valor, size_t nv, char *unid, size_t nu);

/* Cor de estado do valor (vermelho no alarme, verde na faixa boa...) */
uint32_t est_cor(int g, const ui_dados_t *d);

/* Visual da linha de status */
void est_status_visual(ui_status_t st, const lv_image_dsc_t **img, uint32_t *cor);
