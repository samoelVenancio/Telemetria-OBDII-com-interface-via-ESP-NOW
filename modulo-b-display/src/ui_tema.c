/**
 * ui_tema.c — ver ui_tema.h.
 *
 * Se o EEZ for editado no futuro e um objeto deixar de existir, as funções
 * aqui ignoram ponteiros NULL e labels não encontrados: a tela fica com o
 * visual antigo naquele ponto, mas nada quebra.
 */
#include <string.h>

#include "ui/screens.h"
#include "ui_tema.h"

#define GRADE_X0   16
#define GRADE_Y0   80
#define CARTAO_W   248
#define CARTAO_H   186
#define FOLGA      12
#define LARGURA    768

static est_cabecalho_t s_cab_layout, s_cab_config, s_cab_alarmes, s_cab_detalhe;
static lv_obj_t *s_icone_campo[6];
static uint8_t s_layout_ant[6];
static bool s_layout_valido;

/* ---------- utilitários ---------- */

static lv_obj_t *achar_label(lv_obj_t *raiz, const char *texto)
{
    if (raiz == NULL) return NULL;
    if (lv_obj_check_type(raiz, &lv_label_class) && strcmp(lv_label_get_text(raiz), texto) == 0) {
        return raiz;
    }
    uint32_t n = lv_obj_get_child_count(raiz);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *r = achar_label(lv_obj_get_child(raiz, (int32_t)i), texto);
        if (r != NULL) return r;
    }
    return NULL;
}

static lv_obj_t *primeiro_filho(lv_obj_t *o, const lv_obj_class_t *classe)
{
    if (o == NULL) return NULL;
    uint32_t n = lv_obj_get_child_count(o);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *c = lv_obj_get_child(o, (int32_t)i);
        if (lv_obj_check_type(c, classe)) return c;
    }
    return NULL;
}

static void esconder(lv_obj_t *o)
{
    if (o != NULL) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static void colocar(lv_obj_t *o, lv_obj_t *pai, int x, int y)
{
    if (o == NULL) return;
    lv_obj_set_parent(o, pai);
    lv_obj_set_align(o, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(o, x, y);
}

static void texto(lv_obj_t *l, const char *novo, const lv_font_t *f, uint32_t cor)
{
    if (l == NULL) return;
    if (novo != NULL) lv_label_set_text(l, novo);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(cor), 0);
}

static void fundo(lv_obj_t *tela)
{
    if (tela == NULL) return;
    lv_obj_set_style_bg_color(tela, lv_color_hex(EST_FUNDO), 0);
    lv_obj_set_style_bg_opa(tela, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tela, LV_OBJ_FLAG_SCROLLABLE);
}

/* ---------- widgets ---------- */

static void estilo_dropdown(lv_obj_t *dd)
{
    if (dd == NULL) return;
    lv_obj_set_style_bg_color(dd, lv_color_hex(EST_TRILHO), 0);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(EST_BORDA), 0);
    lv_obj_set_style_border_width(dd, 1, 0);
    lv_obj_set_style_border_color(dd, lv_color_hex(EST_AZUL), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(dd, 10, 0);
    lv_obj_set_style_shadow_width(dd, 0, 0);
    lv_obj_set_style_pad_hor(dd, 14, 0);
    lv_obj_set_style_pad_ver(dd, 14, 0);
    lv_obj_set_style_text_font(dd, &ui_font_orb_16, 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(EST_TEXTO), 0);
    /* a seta é um símbolo: tem que continuar na Montserrat */
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_16, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(dd, lv_color_hex(EST_ROTULO), LV_PART_INDICATOR);

    lv_obj_t *lista = lv_dropdown_get_list(dd);
    if (lista != NULL) {
        lv_obj_set_style_bg_color(lista, lv_color_hex(EST_CARTAO), 0);
        lv_obj_set_style_border_color(lista, lv_color_hex(EST_BORDA), 0);
        lv_obj_set_style_border_width(lista, 1, 0);
        lv_obj_set_style_radius(lista, 10, 0);
        lv_obj_set_style_text_font(lista, &ui_font_orb_16, 0);
        lv_obj_set_style_text_color(lista, lv_color_hex(EST_TEXTO), 0);
        lv_obj_set_style_text_line_space(lista, 18, 0);
        lv_obj_set_style_bg_color(lista, lv_color_hex(EST_AZUL), LV_PART_SELECTED | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(lista, lv_color_hex(EST_TRILHO), LV_PART_SELECTED | LV_STATE_PRESSED);
    }
}

static void estilo_botao(lv_obj_t *b, const char *rotulo)
{
    if (b == NULL) return;
    lv_obj_set_style_bg_color(b, lv_color_hex(EST_TRILHO), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(EST_BORDA), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(EST_AZUL), LV_STATE_PRESSED);
    lv_obj_t *l = primeiro_filho(b, &lv_label_class);
    if (l != NULL) {
        texto(l, rotulo, &ui_font_orb_16, EST_TEXTO);
        lv_obj_center(l);
    }
}

static void estilo_slider(lv_obj_t *s, uint32_t cor)
{
    if (s == NULL) return;
    lv_obj_set_style_bg_color(s, lv_color_hex(EST_TRILHO), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s, lv_color_hex(cor), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s, lv_color_hex(EST_TEXTO), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s, 8, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(s, 0, LV_PART_KNOB);
}

static void estilo_switch(lv_obj_t *s)
{
    if (s == NULL) return;
    lv_obj_set_size(s, 64, 34);
    lv_obj_set_style_bg_color(s, lv_color_hex(EST_TRILHO), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(s, lv_color_hex(EST_BORDA), LV_PART_MAIN);
    lv_obj_set_style_border_width(s, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s, lv_color_hex(EST_VERDE), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(s, lv_color_hex(EST_TEXTO), LV_PART_KNOB);
    lv_obj_set_style_shadow_width(s, 0, LV_PART_KNOB);
}

/* ---------- telas ---------- */

static void tema_layout(void)
{
    lv_obj_t *t = objects.painel_layout;
    if (t == NULL) return;
    fundo(t);
    esconder(objects.return_layout);
    esconder(achar_label(t, "Layout info"));
    est_cabecalho(t, "LAYOUT DO PAINEL", est_ao_voltar_menu, &s_cab_layout);

    lv_obj_t *dd[6] = { objects.dropdown_v1, objects.dropdown_v2, objects.dropdown_v3,
                        objects.dropdown_v4, objects.dropdown_v5, objects.dropdown_v6 };
    static const char *const NOMES_EEZ[6] = { "Value 1", "Value 2", "Value 3",
                                              "Value 4", "Value 5", "Value 6" };
    static const char *const NOMES[6] = { "CAMPO 1", "CAMPO 2", "CAMPO 3",
                                          "CAMPO 4", "CAMPO 5", "CAMPO 6" };
    for (int i = 0; i < 6; i++) {
        /* mesma posição do cartão correspondente no Painel */
        int x = GRADE_X0 + (i % 3) * (CARTAO_W + FOLGA);
        int y = GRADE_Y0 + (i / 3) * (CARTAO_H + FOLGA);
        lv_obj_t *c = est_cartao(t, x, y, CARTAO_W, CARTAO_H);
        lv_obj_t *rot = achar_label(t, NOMES_EEZ[i]);
        if (rot == NULL) {
            rot = est_texto(c, &ui_font_orb_16, EST_ROTULO, NOMES[i]);
        }
        texto(rot, NOMES[i], &ui_font_orb_16, EST_ROTULO);
        colocar(rot, c, 14, 22);
        s_icone_campo[i] = est_icone(c, &img_k32_rpm, EST_ROTULO);
        lv_obj_set_pos(s_icone_campo[i], CARTAO_W - 14 - 32, 14);
        if (dd[i] != NULL) {
            colocar(dd[i], c, 14, 96);
            lv_obj_set_size(dd[i], CARTAO_W - 28, 56);
            estilo_dropdown(dd[i]);
        }
    }
}

static lv_obj_t *cartao_linha(lv_obj_t *t, int y, int h, const lv_image_dsc_t *icone,
                              lv_obj_t *titulo, const char *texto_titulo)
{
    lv_obj_t *c = est_cartao(t, GRADE_X0, y, LARGURA, h);
    int x_tit = 14;
    if (icone != NULL) {
        lv_obj_t *ic = est_icone(c, icone, EST_TEXTO);
        lv_obj_set_pos(ic, 14, 14);
        x_tit = 56;
    }
    if (titulo == NULL) {
        titulo = est_texto(c, &ui_font_orb_16, EST_TEXTO, texto_titulo);
    }
    texto(titulo, texto_titulo, &ui_font_orb_16, icone != NULL ? EST_TEXTO : EST_ROTULO);
    colocar(titulo, c, x_tit, 22);
    return c;
}

static void tema_config(void)
{
    lv_obj_t *t = objects.painel_config;
    if (t == NULL) return;
    fundo(t);
    esconder(objects.return_config);
    esconder(achar_label(t, "Config"));
    est_cabecalho(t, "CONFIGURAÇÕES", est_ao_voltar_menu, &s_cab_config);

    /* Taxa do CAN */
    lv_obj_t *c1 = cartao_linha(t, 80, 120, &img_k32_can_ok, achar_label(t, "CAN baudrate"),
                                "TAXA DO BARRAMENTO CAN");
    if (objects.bps_can_config != NULL) {
        colocar(objects.bps_can_config, c1, LARGURA - 14 - 260, 32);
        lv_obj_set_size(objects.bps_can_config, 260, 56);
        estilo_dropdown(objects.bps_can_config);
    }
    lv_obj_t *dica = est_texto(c1, &ui_font_orb_16, EST_ROTULO, "Carro: 500 kbit/s   Bancada: 250 kbit/s");
    lv_obj_set_pos(dica, 56, 66);

    /* Enlace */
    lv_obj_t *c2 = cartao_linha(t, 212, 120, &img_k32_sinal_3, achar_label(t, "Status ESP-NOW:"),
                                "ENLACE ESP-NOW");
    if (objects.obj12 != NULL) {          /* status_espnow */
        texto(objects.obj12, NULL, &ui_font_orb_22, EST_TEXTO);
        colocar(objects.obj12, c2, 56, 62);
    }
    if (objects.retry_config != NULL) {
        colocar(objects.retry_config, c2, LARGURA - 14 - 260, 32);
        lv_obj_set_size(objects.retry_config, 260, 56);
        estilo_botao(objects.retry_config, "ZERAR CONTAGEM");
    }

    /* Versões */
    lv_obj_t *c3 = cartao_linha(t, 344, 120, &img_k32_configuracoes, achar_label(t, "Rev."),
                                "VERSÃO DOS FIRMWARES");
    if (objects.obj13 != NULL) {          /* rev_sys */
        texto(objects.obj13, NULL, &ui_font_orb_22, EST_TEXTO);
        colocar(objects.obj13, c3, 56, 62);
    }
}

static void tema_alarmes(void)
{
    lv_obj_t *t = objects.painel_alarms;
    if (t == NULL) return;
    fundo(t);
    esconder(objects.return_alarms);
    est_cabecalho(t, "ALARMES", est_ao_voltar_menu, &s_cab_alarmes);

    const int H = 118, W_SL = 520;

    lv_obj_t *c1 = cartao_linha(t, 80, H, &img_k32_superaquecimento,
                                achar_label(t, "Max temp. alarm"), "SUPERAQUECIMENTO");
    colocar(objects.max_temp_alarm_sw, c1, LARGURA - 14 - 64, 14);
    estilo_switch(objects.max_temp_alarm_sw);
    colocar(objects.slide_max_temp, c1, 22, 82);
    if (objects.slide_max_temp) lv_obj_set_size(objects.slide_max_temp, W_SL, 8);
    estilo_slider(objects.slide_max_temp, EST_VERMELHO);
    if (objects.obj14 != NULL) {          /* temp_max_alarm */
        texto(objects.obj14, NULL, &ui_font_orb_22, EST_TEXTO);
        lv_obj_set_parent(objects.obj14, c1);
        lv_obj_align(objects.obj14, LV_ALIGN_TOP_RIGHT, -14, 70);
    }

    lv_obj_t *c2 = cartao_linha(t, 80 + H + FOLGA, H, &img_k32_motor_frio,
                                achar_label(t, "Cold engine Alarm"), "MOTOR FRIO");
    colocar(objects.cold_engine_alarm_sw, c2, LARGURA - 14 - 64, 14);
    estilo_switch(objects.cold_engine_alarm_sw);
    lv_obj_t *d = est_texto(c2, &ui_font_orb_16, EST_ROTULO,
                            "Avisa acima de 3000 rpm com o motor abaixo de 60 °C");
    lv_obj_set_pos(d, 56, 72);

    lv_obj_t *c3 = cartao_linha(t, 80 + 2 * (H + FOLGA), H, &img_k32_eco,
                                achar_label(t, "ECO mode"), "MODO ECO  |  TROCA DE MARCHA");
    colocar(objects.eco_mode_sw, c3, LARGURA - 14 - 64, 14);
    estilo_switch(objects.eco_mode_sw);
    colocar(objects.slide_eco_mode_rpm, c3, 22, 82);
    if (objects.slide_eco_mode_rpm) lv_obj_set_size(objects.slide_eco_mode_rpm, W_SL, 8);
    estilo_slider(objects.slide_eco_mode_rpm, EST_VERDE);
    if (objects.obj15 != NULL) {          /* eco_mode_rpm */
        texto(objects.obj15, NULL, &ui_font_orb_22, EST_TEXTO);
        lv_obj_set_parent(objects.obj15, c3);
        lv_obj_align(objects.obj15, LV_ALIGN_TOP_RIGHT, -14, 70);
    }
}

static void voltar_painel(lv_event_t *e)
{
    (void)e;
    est_ir(UI_DEST_PAINEL);
}

static void tema_detalhe(void)
{
    lv_obj_t *t = objects.painel_detalhe;
    if (t == NULL) return;
    fundo(t);
    esconder(objects.return_detalhe);
    esconder(objects.icon_detalhe);
    est_cabecalho(t, "", voltar_painel, &s_cab_detalhe);
    if (objects.titulo_detalhe != NULL) {
        texto(objects.titulo_detalhe, NULL, &ui_font_orb_22, EST_TEXTO);
        colocar(objects.titulo_detalhe, t, 76, 20);
    }
    if (objects.valor_detalhe != NULL) {
        texto(objects.valor_detalhe, NULL, &ui_font_orb_56, EST_TEXTO);
        colocar(objects.valor_detalhe, t, 20, 78);
    }
    if (objects.btn_zerar_detalhe != NULL) {
        colocar(objects.btn_zerar_detalhe, t, 800 - 16 - 200, 86);
        lv_obj_set_size(objects.btn_zerar_detalhe, 200, 52);
        estilo_botao(objects.btn_zerar_detalhe, "ZERAR");
    }

    lv_obj_t *g = objects.grafico_detalhe;
    if (g != NULL) {
        lv_obj_set_pos(g, GRADE_X0, 152);
        lv_obj_set_size(g, LARGURA, 214);
        lv_obj_set_style_bg_color(g, lv_color_hex(EST_CARTAO), 0);
        lv_obj_set_style_bg_opa(g, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(g, lv_color_hex(EST_BORDA), 0);
        lv_obj_set_style_border_width(g, 1, 0);
        lv_obj_set_style_radius(g, 14, 0);
        lv_obj_set_style_pad_all(g, 12, 0);
        lv_obj_t *ch = primeiro_filho(g, &lv_chart_class);
        if (ch != NULL) {
            lv_obj_set_size(ch, lv_pct(100), lv_pct(100));
            lv_obj_set_style_border_width(ch, 0, 0);
            lv_obj_set_style_line_color(ch, lv_color_hex(EST_BORDA), LV_PART_MAIN);
            lv_obj_set_style_line_width(ch, 1, LV_PART_MAIN);
            lv_chart_series_t *s = lv_chart_get_series_next(ch, NULL);
            if (s != NULL) {
                lv_chart_set_series_color(ch, s, lv_color_hex(EST_AZUL));
            }
        }
    }

    /* Mín / média / máx em três cartões */
    lv_obj_t *est[3] = { objects.min_detalhe, objects.med_detalhe, objects.max_detalhe };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *c = est_cartao(t, GRADE_X0 + i * (CARTAO_W + FOLGA), 378, CARTAO_W, 86);
        if (est[i] != NULL) {
            texto(est[i], NULL, &ui_font_orb_22, EST_TEXTO);
            lv_obj_set_parent(est[i], c);
            lv_obj_center(est[i]);
        }
    }
}

/* ---------- API ---------- */

void ui_tema_aplicar(void)
{
    tema_layout();
    tema_config();
    tema_alarmes();
    tema_detalhe();
    s_layout_valido = false;
}

void ui_tema_atualizar(const ui_dados_t *d)
{
    lv_obj_t *ativa = lv_screen_active();
    if (ativa == objects.painel_layout) {
        est_cabecalho_atualizar(&s_cab_layout, d);
        for (int i = 0; i < 6; i++) {
            if (!s_layout_valido || s_layout_ant[i] != d->layout[i]) {
                if (s_icone_campo[i] != NULL) {
                    lv_image_set_src(s_icone_campo[i], est_icone_32(d->layout[i]));
                }
                s_layout_ant[i] = d->layout[i];
            }
        }
        s_layout_valido = true;
    } else if (ativa == objects.painel_config) {
        est_cabecalho_atualizar(&s_cab_config, d);
    } else if (ativa == objects.painel_alarms) {
        est_cabecalho_atualizar(&s_cab_alarmes, d);
    } else if (ativa == objects.painel_detalhe) {
        est_cabecalho_atualizar(&s_cab_detalhe, d);
    }
}
