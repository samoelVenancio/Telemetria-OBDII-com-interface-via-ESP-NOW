/**
 * ui_detalhe.c — ver ui_detalhe.h.
 *
 * Memória: 10 grandezas × 300 pontos × 4 B = 12 KB de histórico, alocado
 * estático. O gráfico não tem cópia própria: aponta (ext_y_array) para um
 * vetor linear que é remontado do anel uma vez por segundo, só com a tela
 * aberta.
 *
 * Consumo: o gráfico e as estatísticas usam km/L. Parado (km/L = 0, pelo
 * contrato) a amostra é tratada como inválida, senão a média despencaria a
 * cada semáforo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "lvgl.h"

#include "telemetria_protocolo.h"
#include "grandeza.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui_icones.h"
#include "ui_estilo.h"   /* fonte e cor da unidade */
#include "ui_detalhe.h"

static const char *TAG = "ui_detalhe";

#define HIST_PONTOS         300   /* 5 min a 1 amostra/s */
#define TICKS_POR_AMOSTRA   10    /* ponte a 10 Hz -> 1 amostra/s no gráfico */
#define TXT_MAX             40

#define COR_SERIE           0x46A758   /* verde OK da paleta dos ícones */
#define COR_GRADE           0x333333

/* Nome e formato de cada grandeza. MESMA ORDEM do grandeza_t. Com acentos:
 * ui_tema.c põe o título e os rótulos na Orbitron, que tem Latin-1. Os nomes
 * seguem est_nome() (ui_estilo.c), mas o combustível mantém (L)/(%) para os
 * dois detalhes não ficarem com o mesmo título. */
typedef struct {
    const char *nome;
    const char *prefixo;
    const char *unidade;
    uint8_t casas;      /* casas decimais do valor escalado */
} descricao_t;

static const descricao_t DESCRICAO[G_TOTAL] = {
    [G_RPM]        = { "ROTAÇÃO",              "",  " rpm",  0 },
    [G_VELOCIDADE] = { "VELOCIDADE",           "",  " km/h", 0 },
    [G_CONSUMO]    = { "CONSUMO INSTANTÂNEO",  "",  " km/L", 1 },
    [G_ETANOL]     = { "ETANOL NA MISTURA",    "E", "",      0 },
    [G_TEMP_MOTOR] = { "TEMPERATURA DO MOTOR", "",  "°C",    1 },
    [G_TEMP_AR]    = { "TEMPERATURA DO AR",    "",  "°C",    1 },
    [G_MAP]        = { "PRESSÃO NO COLETOR",   "",  " kPa",  0 },
    [G_TANQUE_L]   = { "COMBUSTÍVEL (L)",      "",  " L",    1 },
    [G_TANQUE_PCT] = { "COMBUSTÍVEL (%)",      "",  "%",     0 },
    [G_BATERIA]    = { "BATERIA",              "",  " V",    2 },
};

typedef struct {
    int32_t min;
    int32_t max;
    int64_t soma;
    uint32_t n;
} estatistica_t;

static estatistica_t s_estat[G_TOTAL];
static int32_t s_hist[G_TOTAL][HIST_PONTOS];  /* anel; LV_CHART_POINT_NONE = buraco */
static uint16_t s_hist_pos;                   /* próxima posição a escrever */
static uint8_t s_ticks;

static int s_atual = G_RPM;
static lv_obj_t *s_grafico;
static lv_chart_series_t *s_serie;
/* A Orbitron grande do valor (ui_font_orb_56) só tem dígitos e maiúsculas:
 * "km/h", "rpm", "kPa" sairiam como caixas vazias. A unidade vai num label
 * próprio, na fonte de 22 px (alfabeto completo), colado à direita do valor
 * — como no cockpit e no painel. */
static lv_obj_t *s_unidade;
static int32_t s_y[HIST_PONTOS];              /* anel desenrolado, para o gráfico */

static telem_pacote_t s_ultimo;
static bool s_ultimo_valido;

/* Valor escalado da grandeza no pacote. false = sem leitura útil. */
static bool valor_de(int g, const telem_pacote_t *p, int32_t *v)
{
    switch (g) {
    case G_RPM:        *v = p->rpm; return true;
    case G_VELOCIDADE: *v = p->velocidade; return true;
    case G_CONSUMO:    *v = p->consumo_kml_c / 10; return p->consumo_kml_c > 0;
    case G_ETANOL:     *v = p->etanol_pct; return true;
    case G_TEMP_MOTOR: *v = p->temp_arref_d; return true;
    case G_TEMP_AR:    *v = p->iat_d; return true;
    case G_MAP:        *v = p->map_kpa; return true;
    case G_TANQUE_L:   *v = p->litros_d; return true;
    case G_TANQUE_PCT: *v = p->nivel_pct; return true;
    case G_BATERIA:    *v = p->tensao_c; return true;
    default:           return false;
    }
}

/* "<rótulo><prefixo>12.3<unidade>" a partir do valor escalado; sem a
 * unidade quando ela vai no label separado (valor grande) */
static void formatar(char *dest, const char *rotulo, int g, int32_t v, bool com_unidade)
{
    const descricao_t *d = &DESCRICAO[g];
    const char *unidade = com_unidade ? d->unidade : "";
    if (d->casas == 0) {
        snprintf(dest, TXT_MAX, "%s%s%ld%s", rotulo, d->prefixo, (long)v, unidade);
        return;
    }
    int32_t divisor = (d->casas == 2) ? 100 : 10;
    int32_t abs_v = labs(v);
    snprintf(dest, TXT_MAX, (d->casas == 2) ? "%s%s%s%ld.%02ld%s" : "%s%s%s%ld.%ld%s",
             rotulo, d->prefixo, (v < 0) ? "-" : "", (long)(abs_v / divisor),
             (long)(abs_v % divisor), unidade);
}

/* lv_label_set_text sempre invalida a área; só chama se o texto mudou */
static bool escrever(lv_obj_t *label, const char *texto)
{
    if (label != NULL && strcmp(lv_label_get_text(label), texto) != 0) {
        lv_label_set_text(label, texto);
        return true;
    }
    return false;
}

/* Valor grande + unidade ao lado, alinhados pela base do texto */
static void escrever_valor(const char *valor, const char *unidade)
{
    bool mudou = escrever(objects.valor_detalhe, valor);
    mudou |= escrever(s_unidade, unidade);
    if (mudou && s_unidade != NULL && objects.valor_detalhe != NULL) {
        lv_obj_update_layout(objects.valor_detalhe);   /* largura do texto novo */
        lv_obj_align_to(s_unidade, objects.valor_detalhe, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, -10);
    }
}

static void atualizar_textos(void)
{
    char txt[TXT_MAX];
    int32_t v;
    if (s_ultimo_valido && valor_de(s_atual, &s_ultimo, &v)) {
        formatar(txt, "", s_atual, v, false);
        const char *u = DESCRICAO[s_atual].unidade;
        escrever_valor(txt, (*u == ' ') ? u + 1 : u);
    } else {
        escrever_valor("--", "");
    }

    const estatistica_t *e = &s_estat[s_atual];
    if (e->n == 0) {
        escrever(objects.min_detalhe, "Mín --");
        escrever(objects.med_detalhe, "Média --");
        escrever(objects.max_detalhe, "Máx --");
        return;
    }
    formatar(txt, "Mín ", s_atual, e->min, true);
    escrever(objects.min_detalhe, txt);
    formatar(txt, "Média ", s_atual, (int32_t)(e->soma / (int64_t)e->n), true);
    escrever(objects.med_detalhe, txt);
    formatar(txt, "Máx ", s_atual, e->max, true);
    escrever(objects.max_detalhe, txt);
}

/* Desenrola o anel (mais antigo à esquerda) e ajusta a escala vertical aos
 * pontos visíveis, com 10 % de folga em cima e embaixo. */
static void atualizar_grafico(void)
{
    if (s_grafico == NULL) {
        return;
    }
    int32_t lo = INT32_MAX;
    int32_t hi = INT32_MIN;
    for (int i = 0; i < HIST_PONTOS; i++) {
        int32_t v = s_hist[s_atual][(s_hist_pos + i) % HIST_PONTOS];
        s_y[i] = v;
        if (v != LV_CHART_POINT_NONE) {
            lo = (v < lo) ? v : lo;
            hi = (v > hi) ? v : hi;
        }
    }
    if (lo > hi) {          /* nenhum ponto ainda */
        lo = 0;
        hi = 1;
    }
    int32_t folga = (hi - lo) / 10;
    if (folga < 1) {
        folga = 1;
    }
    lv_chart_set_axis_range(s_grafico, LV_CHART_AXIS_PRIMARY_Y, lo - folga, hi + folga);
    lv_chart_refresh(s_grafico);
}

static bool tela_aberta(void)
{
    return objects.painel_detalhe != NULL && lv_screen_active() == objects.painel_detalhe;
}

void ui_detalhe_amostrar(const telem_pacote_t *p, bool dados)
{
    bool nova_amostra = (++s_ticks >= TICKS_POR_AMOSTRA);
    if (nova_amostra) {
        s_ticks = 0;
    }

    for (int g = 0; g < G_TOTAL; g++) {
        int32_t v = 0;
        bool valido = dados && valor_de(g, p, &v);
        if (valido) {
            estatistica_t *e = &s_estat[g];
            if (e->n == 0 || v < e->min) {
                e->min = v;
            }
            if (e->n == 0 || v > e->max) {
                e->max = v;
            }
            e->soma += v;
            e->n++;
        }
        if (nova_amostra) {
            s_hist[g][s_hist_pos] = valido ? v : LV_CHART_POINT_NONE;
        }
    }
    if (nova_amostra) {
        s_hist_pos = (s_hist_pos + 1) % HIST_PONTOS;
    }

    s_ultimo = *p;
    s_ultimo_valido = dados;

    if (tela_aberta()) {
        atualizar_textos();
        if (nova_amostra) {
            atualizar_grafico();
        }
    }
}

void ui_detalhe_abrir(int grandeza)
{
    if (objects.painel_detalhe == NULL || grandeza < 0 || grandeza >= G_TOTAL) {
        return;
    }
    s_atual = grandeza;
    if (objects.titulo_detalhe != NULL) {
        lv_label_set_text_static(objects.titulo_detalhe, DESCRICAO[grandeza].nome);
    }
    if (objects.icon_detalhe != NULL) {
        lv_image_set_src(objects.icon_detalhe, ui_icones_da_grandeza(grandeza));
    }
    atualizar_textos();
    atualizar_grafico();
    loadScreen(SCREEN_ID_PAINEL_DETALHE);
    ESP_LOGI(TAG, "detalhe: %s", DESCRICAO[grandeza].nome);
}

/* Zera só as estatísticas da grandeza mostrada; o gráfico (5 min) fica */
static void ao_zerar(lv_event_t *e)
{
    (void)e;
    memset(&s_estat[s_atual], 0, sizeof(s_estat[s_atual]));
    atualizar_textos();
    ESP_LOGI(TAG, "estatisticas de %s zeradas", DESCRICAO[s_atual].nome);
}

void ui_detalhe_iniciar(void)
{
    if (objects.painel_detalhe != NULL) {
        s_unidade = lv_label_create(objects.painel_detalhe);
        lv_label_set_text_static(s_unidade, "");
        lv_obj_set_style_text_font(s_unidade, &ui_font_orb_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(s_unidade, lv_color_hex(EST_ROTULO), LV_PART_MAIN);
    }

    for (int g = 0; g < G_TOTAL; g++) {
        for (int i = 0; i < HIST_PONTOS; i++) {
            s_hist[g][i] = LV_CHART_POINT_NONE;
        }
    }

    lv_obj_t *area = objects.grafico_detalhe;
    if (area == NULL) {
        ESP_LOGW(TAG, "grafico_detalhe nao existe no EEZ — tela de detalhe sem grafico");
        return;
    }
    /* O Container do EEZ é só a moldura de posição: transparente e sem
     * padding, o gráfico ocupa ele inteiro */
    lv_obj_set_style_bg_opa(area, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(area, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(area, 0, LV_PART_MAIN);
    lv_obj_remove_flag(area, LV_OBJ_FLAG_SCROLLABLE);

    s_grafico = lv_chart_create(area);
    lv_obj_set_size(s_grafico, lv_pct(100), lv_pct(100));
    lv_chart_set_type(s_grafico, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_grafico, HIST_PONTOS);
    lv_chart_set_div_line_count(s_grafico, 5, 0);
    lv_obj_set_style_bg_opa(s_grafico, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_grafico, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_grafico, lv_color_hex(COR_GRADE), LV_PART_MAIN);
    lv_obj_set_style_line_color(s_grafico, lv_color_hex(COR_GRADE), LV_PART_MAIN);
    lv_obj_set_style_line_width(s_grafico, 3, LV_PART_ITEMS);
    lv_obj_set_style_size(s_grafico, 0, 0, LV_PART_INDICATOR);  /* sem bolinha em cada ponto */
    lv_obj_remove_flag(s_grafico, LV_OBJ_FLAG_CLICKABLE);

    s_serie = lv_chart_add_series(s_grafico, lv_color_hex(COR_SERIE), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < HIST_PONTOS; i++) {
        s_y[i] = LV_CHART_POINT_NONE;
    }
    lv_chart_set_series_ext_y_array(s_grafico, s_serie, s_y);

    if (objects.btn_zerar_detalhe != NULL) {
        lv_obj_add_event_cb(objects.btn_zerar_detalhe, ao_zerar, LV_EVENT_CLICKED, NULL);
    }
}
