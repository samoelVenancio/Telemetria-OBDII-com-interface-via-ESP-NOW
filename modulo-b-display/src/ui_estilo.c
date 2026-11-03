/**
 * ui_estilo.c — ver ui_estilo.h.
 */
#include <stdio.h>
#include <string.h>

#include "ui_estilo.h"

#define RPM_LIGADO      600

/* ===================================================================== */
/* Navegação                                                             */
/* ===================================================================== */

static void (*s_ir)(ui_destino_t);

void est_definir_navegacao(void (*ir)(ui_destino_t)) { s_ir = ir; }

void est_ir(ui_destino_t destino)
{
    if (s_ir != NULL) {
        s_ir(destino);
    }
}

void est_ao_voltar_menu(lv_event_t *e)
{
    (void)e;
    est_ir(UI_DEST_MENU);
}

/* ===================================================================== */
/* Construção                                                            */
/* ===================================================================== */

lv_obj_t *est_tela(void)
{
    lv_obj_t *t = lv_obj_create(NULL);
    lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(t, lv_color_hex(EST_FUNDO), 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    return t;
}

lv_obj_t *est_texto(lv_obj_t *pai, const lv_font_t *f, uint32_t cor, const char *t)
{
    lv_obj_t *l = lv_label_create(pai);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(cor), 0);
    lv_label_set_text(l, t);
    return l;
}

void est_pintar_icone(lv_obj_t *img, uint32_t cor)
{
    lv_obj_set_style_image_recolor(img, lv_color_hex(cor), 0);
    lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
}

lv_obj_t *est_icone(lv_obj_t *pai, const lv_image_dsc_t *img, uint32_t cor)
{
    lv_obj_t *i = lv_image_create(pai);
    lv_image_set_src(i, img);
    est_pintar_icone(i, cor);
    return i;
}

lv_obj_t *est_cartao(lv_obj_t *pai, int x, int y, int w, int h)
{
    lv_obj_t *c = lv_obj_create(pai);
    lv_obj_remove_style_all(c);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_bg_color(c, lv_color_hex(EST_CARTAO), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(c, lv_color_hex(EST_BORDA), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_radius(c, 14, 0);
    /* toque: borda acende (o cartão só é clicável se quem criou quiser) */
    lv_obj_set_style_border_color(c, lv_color_hex(EST_AZUL), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(c, 2, LV_STATE_PRESSED);
    lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
    return c;
}

lv_obj_t *est_linha(lv_obj_t *pai, int x, int y, int w, int h)
{
    lv_obj_t *o = lv_obj_create(pai);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(EST_BORDA), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    return o;
}

void est_status_visual(ui_status_t st, const lv_image_dsc_t **img, uint32_t *cor)
{
    switch (st) {
    case UI_STATUS_SEM_ENLACE:       *img = &img_k32_sem_sinal;        *cor = EST_VERMELHO; break;
    case UI_STATUS_SEM_CAN:          *img = &img_k32_can_erro;         *cor = EST_AMBAR;    break;
    case UI_STATUS_SUPERAQUECIMENTO: *img = &img_k32_superaquecimento; *cor = EST_VERMELHO; break;
    case UI_STATUS_MOTOR_FRIO:       *img = &img_k32_motor_frio;       *cor = EST_AMBAR;    break;
    case UI_STATUS_ECO:              *img = &img_k32_trocar_marcha;    *cor = EST_AMBAR;    break;
    default:                         *img = &img_k32_can_ok;           *cor = EST_VERDE;    break;
    }
}

static void sinal_visual(const ui_dados_t *d, const lv_image_dsc_t **img, uint32_t *cor)
{
    if (!d->enlace_ok)           { *img = &img_k32_sem_sinal; *cor = EST_VERMELHO; }
    else if (d->rssi_dbm >= -60) { *img = &img_k32_sinal_3;   *cor = EST_VERDE; }
    else if (d->rssi_dbm >= -70) { *img = &img_k32_sinal_2;   *cor = EST_VERDE; }
    else if (d->rssi_dbm >= -80) { *img = &img_k32_sinal_1;   *cor = EST_AMBAR; }
    else                         { *img = &img_k32_sinal_0;   *cor = EST_VERMELHO; }
}

void est_cabecalho(lv_obj_t *tela, const char *titulo, lv_event_cb_t voltar,
                   est_cabecalho_t *out)
{
    int x_titulo = 24;
    if (voltar != NULL) {
        /* Área de toque de 64×64 com a seta no meio: dedo não erra */
        lv_obj_t *b = lv_obj_create(tela);
        lv_obj_remove_style_all(b);
        lv_obj_set_pos(b, 4, 0);
        lv_obj_set_size(b, 64, 64);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(b, lv_color_hex(EST_TRILHO), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_set_style_radius(b, 12, 0);
        lv_obj_add_event_cb(b, voltar, LV_EVENT_CLICKED, NULL);
        lv_obj_t *i = est_icone(b, &img_k32_voltar, EST_TEXTO);
        lv_obj_center(i);
        x_titulo = 76;
    }
    lv_obj_t *t = est_texto(tela, &ui_font_orb_22, EST_TEXTO, titulo);
    lv_obj_set_pos(t, x_titulo, 20);
    est_linha(tela, 16, 63, 768, 1);

    out->status = est_icone(tela, &img_k32_sem_sinal, EST_VERMELHO);
    lv_obj_set_pos(out->status, 704, 16);
    out->sinal = est_icone(tela, &img_k32_sem_sinal, EST_VERMELHO);
    lv_obj_set_pos(out->sinal, 752, 16);
}

void est_cabecalho_atualizar(est_cabecalho_t *c, const ui_dados_t *d)
{
    const lv_image_dsc_t *img;
    uint32_t cor;
    est_status_visual(d->status, &img, &cor);
    if (lv_image_get_src(c->status) != img) {
        lv_image_set_src(c->status, img);
        est_pintar_icone(c->status, cor);
    }
    sinal_visual(d, &img, &cor);
    if (lv_image_get_src(c->sinal) != img) {
        lv_image_set_src(c->sinal, img);
        est_pintar_icone(c->sinal, cor);
    }
}

/* ===================================================================== */
/* Grandezas                                                             */
/* ===================================================================== */

typedef struct {
    const char *nome;
    const char *curto;
    const lv_image_dsc_t *icone;
    est_tipo_barra_t barra;
    int32_t min, max;          /* faixa da barra, nas unidades de nivel() */
} descr_t;

/* MESMA ORDEM do grandeza_t */
static const descr_t D[G_TOTAL] = {
    [G_RPM]        = { "ROTAÇÃO",              "ROTAÇÃO",     &img_k32_rpm,        BARRA_FAIXAS,    0, 7000 },
    [G_VELOCIDADE] = { "VELOCIDADE",           "VELOCIDADE",  &img_k32_velocidade, BARRA_SIMPLES,   0, 160  },
    [G_CONSUMO]    = { "CONSUMO INSTANTÂNEO",  "CONSUMO",     &img_k32_consumo,    BARRA_SIMPLES,   0, 250  },
    [G_ETANOL]     = { "ETANOL NA MISTURA",    "ETANOL",      &img_k32_etanol,     BARRA_MISTURA,   0, 100  },
    [G_TEMP_MOTOR] = { "TEMPERATURA DO MOTOR", "MOTOR",       &img_k32_temp_motor, BARRA_FAIXAS,    40, 120 },
    [G_TEMP_AR]    = { "TEMPERATURA DO AR",    "AR ADMITIDO", &img_k32_temp_ar,    BARRA_SIMPLES,   -10, 60 },
    [G_MAP]        = { "PRESSÃO NO COLETOR",   "PRESSÃO MAP", &img_k32_map,        BARRA_SIMPLES,   20, 105 },
    [G_TANQUE_L]   = { "COMBUSTÍVEL",          "COMBUSTÍVEL", &img_k32_tanque,     BARRA_SEGMENTOS, 0, 100  },
    [G_TANQUE_PCT] = { "COMBUSTÍVEL",          "COMBUSTÍVEL", &img_k32_tanque,     BARRA_SEGMENTOS, 0, 100  },
    [G_BATERIA]    = { "BATERIA",              "BATERIA",     &img_k32_bateria,    BARRA_FAIXAS,    110, 160 },
};

static bool g_ok(int g) { return g >= 0 && g < G_TOTAL; }

const char *est_nome(int g)            { return g_ok(g) ? D[g].nome : "--"; }
const char *est_nome_curto(int g)      { return g_ok(g) ? D[g].curto : "--"; }
const lv_image_dsc_t *est_icone_32(int g) { return g_ok(g) ? D[g].icone : &img_k32_alerta; }
est_tipo_barra_t est_tipo_barra(int g) { return g_ok(g) ? D[g].barra : BARRA_SIMPLES; }

static bool motor_ligado(const ui_dados_t *d) { return d->dados && d->rpm > RPM_LIGADO; }

/* Valor da barra, na unidade da faixa da tabela */
static int32_t nivel(int g, const ui_dados_t *d)
{
    switch (g) {
    case G_RPM:        return d->rpm;
    case G_VELOCIDADE: return d->velocidade;
    case G_CONSUMO:    return d->consumo_kml_c ? d->consumo_kml_c / 10 : d->consumo_lh_c / 10;
    case G_ETANOL:     return d->etanol_pct;
    case G_TEMP_MOTOR: return d->temp_d / 10;
    case G_TEMP_AR:    return d->iat_d / 10;
    case G_MAP:        return d->map_kpa;
    case G_TANQUE_L:
    case G_TANQUE_PCT: return d->nivel_pct;
    case G_BATERIA:    return d->tensao_c / 10;
    default:           return 0;
    }
}

void est_valor(int g, const ui_dados_t *d, char *v, size_t nv, char *u, size_t nu)
{
    switch (g) {
    case G_RPM:        snprintf(v, nv, "%u", (unsigned)d->rpm);        snprintf(u, nu, "RPM");  break;
    case G_VELOCIDADE: snprintf(v, nv, "%u", (unsigned)d->velocidade); snprintf(u, nu, "KM/H"); break;
    case G_CONSUMO: {
        bool kml = d->consumo_kml_c > 0;
        unsigned c = kml ? d->consumo_kml_c : d->consumo_lh_c;
        snprintf(v, nv, "%u.%u", c / 100, (c % 100) / 10);
        snprintf(u, nu, kml ? "KM/L" : "L/H");
        break;
    }
    case G_ETANOL:     snprintf(v, nv, "E%u", (unsigned)d->etanol_pct); snprintf(u, nu, "%s", ""); break;
    case G_TEMP_MOTOR: snprintf(v, nv, "%d", d->temp_d / 10);           snprintf(u, nu, "°C");   break;
    case G_TEMP_AR:    snprintf(v, nv, "%d", d->iat_d / 10);            snprintf(u, nu, "°C");   break;
    case G_MAP:        snprintf(v, nv, "%u", (unsigned)d->map_kpa);     snprintf(u, nu, "KPA");  break;
    case G_TANQUE_L:
        snprintf(v, nv, "%u.%u", (unsigned)(d->litros_d / 10), (unsigned)(d->litros_d % 10));
        snprintf(u, nu, "L");
        break;
    case G_TANQUE_PCT: snprintf(v, nv, "%u", (unsigned)d->nivel_pct);   snprintf(u, nu, "%%");   break;
    case G_BATERIA:
        snprintf(v, nv, "%u.%u", (unsigned)(d->tensao_c / 100), (unsigned)((d->tensao_c % 100) / 10));
        snprintf(u, nu, "V");
        break;
    default: snprintf(v, nv, "--"); snprintf(u, nu, "%s", ""); break;
    }
}

uint32_t est_cor(int g, const ui_dados_t *d)
{
    if (!d->dados) {
        return EST_APAGADO;
    }
    switch (g) {
    case G_RPM:
        return d->rpm >= 5000 ? EST_VERMELHO : d->rpm >= 3000 ? EST_LARANJA : EST_AZUL;
    case G_TEMP_MOTOR:
        if (d->alarme_temp || d->temp_d >= d->limiar_temp_d) return EST_VERMELHO;
        return d->temp_d < 600 ? EST_AZUL : EST_TEXTO;
    case G_BATERIA:
        if (motor_ligado(d)) {
            return (d->tensao_c < 1300 || d->tensao_c > 1500) ? EST_VERMELHO : EST_VERDE;
        }
        return d->tensao_c < 1220 ? EST_AMBAR : EST_TEXTO;
    case G_TANQUE_L:
    case G_TANQUE_PCT:
        return d->nivel_pct <= 10 ? EST_VERMELHO : d->nivel_pct <= 20 ? EST_AMBAR : EST_VERDE;
    case G_ETANOL:
        return EST_VERDE;
    case G_CONSUMO:
        return EST_AZUL;
    default:
        return EST_TEXTO;
    }
}

/* ===================================================================== */
/* Barras                                                                */
/* ===================================================================== */

#define BARRA_H        12
#define REGUA_Y        16
#define REGUA_H        3
#define SEG_N          10
#define SEG_FOLGA      4

static int px(const est_barra_t *b, int32_t v)
{
    const descr_t *e = &D[b->grandeza];
    if (v < e->min) v = e->min;
    if (v > e->max) v = e->max;
    return (int)((v - e->min) * b->w / (e->max - e->min));
}

static void faixa(const est_barra_t *b, int32_t de, int32_t ate, uint32_t cor)
{
    int x0 = px(b, de), x1 = px(b, ate);
    if (x1 - x0 < 2) return;
    lv_obj_t *r = lv_obj_create(b->raiz);
    lv_obj_remove_style_all(r);
    lv_obj_set_pos(r, x0 + 1, REGUA_Y);
    lv_obj_set_size(r, x1 - x0 - 2, REGUA_H);
    lv_obj_set_style_bg_color(r, lv_color_hex(cor), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_CLICKABLE);
}

static lv_obj_t *barra_lv(lv_obj_t *pai, int w, uint32_t fundo, uint32_t ind, int32_t min, int32_t max)
{
    lv_obj_t *br = lv_bar_create(pai);
    lv_obj_remove_style_all(br);
    lv_obj_set_pos(br, 0, 0);
    lv_obj_set_size(br, w, BARRA_H);
    lv_obj_set_style_bg_color(br, lv_color_hex(fundo), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(br, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(br, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(br, lv_color_hex(ind), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(br, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(br, 3, LV_PART_INDICATOR);
    lv_bar_set_range(br, min, max);
    lv_obj_remove_flag(br, LV_OBJ_FLAG_CLICKABLE);
    return br;
}

void est_barra_criar(est_barra_t *b, lv_obj_t *pai, int x, int y, int w, int grandeza,
                     const ui_dados_t *d)
{
    memset(b, 0, sizeof(*b));
    if (!g_ok(grandeza)) grandeza = G_RPM;
    b->pai = pai;
    b->x = x;
    b->y = y;
    b->limiar = d ? d->limiar_temp_d : 1050;
    b->grandeza = grandeza;
    b->w = w;
    b->raiz = lv_obj_create(pai);
    lv_obj_remove_style_all(b->raiz);
    lv_obj_set_pos(b->raiz, x, y);
    lv_obj_set_size(b->raiz, w, REGUA_Y + REGUA_H);
    lv_obj_remove_flag(b->raiz, LV_OBJ_FLAG_CLICKABLE);

    const descr_t *e = &D[grandeza];
    switch (e->barra) {
    case BARRA_SEGMENTOS: {
        int sw = (w - (SEG_N - 1) * SEG_FOLGA) / SEG_N;
        for (int i = 0; i < SEG_N; i++) {
            lv_obj_t *s = lv_obj_create(b->raiz);
            lv_obj_remove_style_all(s);
            lv_obj_set_pos(s, i * (sw + SEG_FOLGA), 0);
            lv_obj_set_size(s, sw, BARRA_H + 4);
            lv_obj_set_style_bg_color(s, lv_color_hex(EST_TRILHO), 0);
            lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(s, 2, 0);
            lv_obj_remove_flag(s, LV_OBJ_FLAG_CLICKABLE);
            b->seg[i] = s;
        }
        /* reserva marcada embaixo do 1º bloco */
        lv_obj_t *r = lv_obj_create(b->raiz);
        lv_obj_remove_style_all(r);
        lv_obj_set_pos(r, 0, REGUA_Y + 2);
        lv_obj_set_size(r, sw, REGUA_H);
        lv_obj_set_style_bg_color(r, lv_color_hex(EST_VERMELHO), 0);
        lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
        break;
    }
    case BARRA_MISTURA:
        /* fundo = gasolina (âmbar), indicador = etanol (verde) */
        b->ind = barra_lv(b->raiz, w, EST_AMBAR, EST_VERDE, 0, 100);
        break;
    case BARRA_FAIXAS:
        b->ind = barra_lv(b->raiz, w, EST_TRILHO, EST_TEXTO, e->min, e->max);
        if (grandeza == G_RPM) {
            faixa(b, 0, 3000, EST_AZUL);
            faixa(b, 3000, 5000, EST_LARANJA);
            faixa(b, 5000, 7000, EST_VERMELHO);
        } else if (grandeza == G_BATERIA) {
            faixa(b, 110, 130, EST_VERMELHO);
            faixa(b, 130, 150, EST_VERDE);
            faixa(b, 150, 160, EST_VERMELHO);
        } else if (grandeza == G_TEMP_MOTOR) {
            int32_t lim = d ? d->limiar_temp_d / 10 : 105;
            faixa(b, 40, 60, EST_AZUL);
            faixa(b, 60, lim, EST_ESCALA);
            faixa(b, lim, 120, EST_VERMELHO);
            b->marca = lv_obj_create(b->raiz);
            lv_obj_remove_style_all(b->marca);
            lv_obj_set_size(b->marca, 2, BARRA_H + 8);
            lv_obj_set_pos(b->marca, px(b, lim) - 1, -4);
            lv_obj_set_style_bg_color(b->marca, lv_color_hex(EST_TEXTO), 0);
            lv_obj_set_style_bg_opa(b->marca, LV_OPA_COVER, 0);
        }
        break;
    case BARRA_SIMPLES:
    default:
        b->ind = barra_lv(b->raiz, w, EST_TRILHO, EST_TEXTO, e->min, e->max);
        break;
    }
}

void est_barra_atualizar(est_barra_t *b, const ui_dados_t *d)
{
    if (b->raiz == NULL) return;
    if (b->grandeza == G_TEMP_MOTOR && d->limiar_temp_d != b->limiar) {
        /* limiar mudou na tela de alarmes: refaz a régua e a marca */
        lv_obj_t *pai = b->pai;
        int x = b->x, y = b->y, w = b->w;
        lv_obj_delete(b->raiz);
        est_barra_criar(b, pai, x, y, w, G_TEMP_MOTOR, d);
    }
    int32_t v = nivel(b->grandeza, d);
    uint32_t cor = est_cor(b->grandeza, d);
    if (b->valido && v == b->ultimo && cor == b->ultima_cor) return;

    if (D[b->grandeza].barra == BARRA_SEGMENTOS) {
        int acesos = (v * SEG_N + 50) / 100;   /* arredonda para o bloco mais próximo */
        if (v > 0 && acesos == 0) acesos = 1;  /* sempre mostra que ainda tem algo */
        for (int i = 0; i < SEG_N; i++) {
            lv_obj_set_style_bg_color(b->seg[i], lv_color_hex(i < acesos ? cor : EST_TRILHO), 0);
        }
    } else {
        lv_bar_set_value(b->ind, v, LV_ANIM_OFF);
        if (D[b->grandeza].barra != BARRA_MISTURA) {
            lv_obj_set_style_bg_color(b->ind, lv_color_hex(cor), LV_PART_INDICATOR);
        } else {
            /* mistura: etanol (indicador) × gasolina (fundo); cinza sem dados */
            lv_obj_set_style_bg_color(b->ind, lv_color_hex(d->dados ? EST_VERDE : EST_APAGADO),
                                      LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(b->ind, lv_color_hex(d->dados ? EST_AMBAR : EST_TRILHO),
                                      LV_PART_MAIN);
        }
    }
    b->ultimo = v;
    b->ultima_cor = cor;
    b->valido = true;
}
