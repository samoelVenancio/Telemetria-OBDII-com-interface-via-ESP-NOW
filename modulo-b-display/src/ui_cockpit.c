/**
 * ui_cockpit.c — ver ui_cockpit.h.
 *
 * Por que arco para o RPM e barras para o resto: o RPM muda o tempo todo e o
 * arco dá a leitura de "ponteiro"; temperatura, bateria e combustível
 * importam pela distância até um limite, e uma barra com faixas mostra isso
 * de relance.
 *
 * Geometria (800 × 480):
 *   barra superior (0..48)   status | sinal
 *   esquerda                 arco de RPM, centro (224, 266), diâmetro 400
 *   direita (x 478..784)     5 linhas de 80 px
 */
#include <ctype.h>
#include <string.h>

#include "ui_cockpit.h"

#define RPM_MAX        7000
#define RPM_ROT        135
#define RPM_VARREDURA  270
#define RPM_X          24
#define RPM_Y          66
#define RPM_D          400

#define COL_X          478
#define COL_W          306
#define LIN_Y0         62
#define LIN_PASSO      82
#define N_LINHAS       5

#define ANIM_MS        150     /* 0 desliga a animação do arco */

static const int LINHAS[N_LINHAS] = { G_TEMP_MOTOR, G_BATERIA, G_TANQUE_L, G_ETANOL, G_CONSUMO };

typedef struct {
    lv_obj_t *icone, *valor, *unid;
    est_barra_t barra;
    char txt[16];
    uint32_t cor;
} linha_t;

static lv_obj_t *s_tela;
static lv_obj_t *s_rpm_ind, *s_rpm_val, *s_vel_val;
static lv_obj_t *s_st_icone, *s_st_txt, *s_sinal;
static linha_t s_lin[N_LINHAS];

static struct {
    bool valido;
    int32_t rpm, vel;
    uint32_t cor_rpm, cor_num;
    char msg[48];
    int status;
    const void *sinal;
} s_ant;

/* ---------- construção ---------- */

static lv_obj_t *arco(lv_obj_t *pai, int x, int y, int d, int a0, int a1, int larg, uint32_t cor)
{
    lv_obj_t *a = lv_arc_create(pai);
    lv_obj_set_pos(a, x, y);
    lv_obj_set_size(a, d, d);
    lv_arc_set_rotation(a, RPM_ROT);
    lv_arc_set_bg_angles(a, a0, a1);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_all(a, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, larg, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, lv_color_hex(cor), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(a, false, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    return a;
}

static void criar_rpm(lv_obj_t *t)
{
    int a3 = 3000 * RPM_VARREDURA / RPM_MAX, a5 = 5000 * RPM_VARREDURA / RPM_MAX;
    arco(t, RPM_X, RPM_Y, RPM_D, 0, a3 - 1, 6, EST_AZUL);
    arco(t, RPM_X, RPM_Y, RPM_D, a3 + 1, a5 - 1, 6, EST_LARANJA);
    arco(t, RPM_X, RPM_Y, RPM_D, a5 + 1, RPM_VARREDURA, 6, EST_VERMELHO);

    s_rpm_ind = arco(t, RPM_X + 16, RPM_Y + 16, RPM_D - 32, 0, RPM_VARREDURA, 26, EST_TRILHO);
    lv_arc_set_range(s_rpm_ind, 0, RPM_MAX);
    lv_arc_set_value(s_rpm_ind, 0);
    lv_obj_set_style_arc_opa(s_rpm_ind, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_rpm_ind, 26, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_rpm_ind, false, LV_PART_INDICATOR);

    lv_obj_t *e = lv_scale_create(t);
    lv_obj_set_size(e, RPM_D - 92, RPM_D - 92);
    lv_obj_set_pos(e, RPM_X + 46, RPM_Y + 46);
    lv_obj_remove_flag(e, LV_OBJ_FLAG_CLICKABLE);
    lv_scale_set_mode(e, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_angle_range(e, RPM_VARREDURA);
    lv_scale_set_rotation(e, RPM_ROT);
    lv_scale_set_range(e, 0, 7);
    lv_scale_set_total_tick_count(e, 15);
    lv_scale_set_major_tick_every(e, 2);
    lv_scale_set_label_show(e, false);
    lv_obj_set_style_bg_opa(e, LV_OPA_TRANSP, 0);
    lv_obj_set_style_arc_width(e, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(e, lv_color_hex(EST_ESCALA), LV_PART_ITEMS);
    lv_obj_set_style_line_color(e, lv_color_hex(EST_ROTULO), LV_PART_INDICATOR);
    lv_obj_set_style_length(e, 6, LV_PART_ITEMS);
    lv_obj_set_style_length(e, 12, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(e, 2, LV_PART_ITEMS);
    lv_obj_set_style_line_width(e, 3, LV_PART_INDICATOR);

    int cx = RPM_X + RPM_D / 2, cy = RPM_Y + RPM_D / 2;
    s_rpm_val = est_texto(t, &ui_font_orb_80, EST_TEXTO, "0");
    lv_obj_align(s_rpm_val, LV_ALIGN_TOP_MID, cx - 400, cy - 62);
    lv_obj_t *l = est_texto(t, &ui_font_orb_16, EST_ROTULO, "RPM");
    lv_obj_align(l, LV_ALIGN_TOP_MID, cx - 400, cy + 32);

    s_vel_val = est_texto(t, &ui_font_orb_64, EST_TEXTO, "0");
    lv_obj_align(s_vel_val, LV_ALIGN_TOP_MID, cx - 400, cy + 88);
    l = est_texto(t, &ui_font_orb_16, EST_ROTULO, "KM/H");
    lv_obj_align(l, LV_ALIGN_TOP_MID, cx - 400, cy + 160);
}

static void criar_linha(lv_obj_t *t, linha_t *li, int g, int y)
{
    li->icone = est_icone(t, est_icone_32(g), EST_ROTULO);
    lv_obj_set_pos(li->icone, COL_X, y);
    lv_obj_t *n = est_texto(t, &ui_font_orb_16, EST_ROTULO, est_nome_curto(g));
    lv_obj_set_pos(n, COL_X + 42, y + 9);

    li->unid = est_texto(t, &ui_font_orb_16, EST_ROTULO, "");
    lv_obj_align(li->unid, LV_ALIGN_TOP_RIGHT, -(800 - COL_X - COL_W), y + 18);
    li->valor = est_texto(t, &ui_font_orb_40, EST_TEXTO, "--");
    lv_obj_align_to(li->valor, li->unid, LV_ALIGN_OUT_LEFT_BOTTOM, -6, 4);

    est_barra_criar(&li->barra, t, COL_X, y + 48, COL_W, g, NULL);
}

static void ao_tocar(lv_event_t *e)
{
    (void)e;
    est_ir(UI_DEST_MENU);
}

void ui_cockpit_iniciar(void)
{
    s_tela = est_tela();
    lv_obj_add_event_cb(s_tela, ao_tocar, LV_EVENT_CLICKED, NULL);

    s_st_icone = est_icone(s_tela, &img_k32_sem_sinal, EST_VERMELHO);
    lv_obj_set_pos(s_st_icone, 16, 8);
    s_st_txt = est_texto(s_tela, &ui_font_orb_16, EST_VERMELHO, "AGUARDANDO MÓDULO A");
    lv_obj_set_pos(s_st_txt, 58, 15);
    s_sinal = est_icone(s_tela, &img_k32_sem_sinal, EST_VERMELHO);
    lv_obj_set_pos(s_sinal, 752, 8);
    est_linha(s_tela, 16, 48, 768, 1);

    criar_rpm(s_tela);
    est_linha(s_tela, 456, 66, 1, 398);
    for (int i = 0; i < N_LINHAS; i++) {
        criar_linha(s_tela, &s_lin[i], LINHAS[i], LIN_Y0 + i * LIN_PASSO);
    }
    memset(&s_ant, 0, sizeof(s_ant));
}

void ui_cockpit_abrir(void)
{
    s_ant.valido = false;
    for (int i = 0; i < N_LINHAS; i++) {
        s_lin[i].txt[0] = '\0';
        s_lin[i].cor = 0xFFFFFFFF;
        s_lin[i].barra.valido = false;
    }
    lv_screen_load(s_tela);
}

/* ---------- atualização ---------- */

static void anima_cb(void *arc, int32_t v) { lv_arc_set_value((lv_obj_t *)arc, v); }

static void mover(lv_obj_t *arc, int32_t v)
{
#if ANIM_MS > 0
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_exec_cb(&a, anima_cb);
    lv_anim_set_values(&a, lv_arc_get_value(arc), v);
    lv_anim_set_duration(&a, ANIM_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
#else
    lv_arc_set_value(arc, v);
#endif
}

static void atualizar_linha(linha_t *li, int g, const ui_dados_t *d)
{
    char v[16], u[8];
    est_valor(g, d, v, sizeof(v), u, sizeof(u));
    uint32_t cor = est_cor(g, d);
    if (strcmp(v, li->txt) != 0) {
        lv_label_set_text(li->valor, v);
        lv_label_set_text(li->unid, u);
        lv_obj_align_to(li->valor, li->unid, LV_ALIGN_OUT_LEFT_BOTTOM, -6, 4);
        strcpy(li->txt, v);
    }
    if (cor != li->cor) {
        /* número branco no normal; colorido só quando o estado pede atenção */
        uint32_t cor_num = (cor == EST_VERMELHO || cor == EST_AMBAR || cor == EST_APAGADO)
                           ? cor : EST_TEXTO;
        lv_obj_set_style_text_color(li->valor, lv_color_hex(cor_num), 0);
        est_pintar_icone(li->icone, cor == EST_APAGADO ? EST_APAGADO
                                    : (cor == EST_TEXTO ? EST_ROTULO : cor));
        li->cor = cor;
    }
    est_barra_atualizar(&li->barra, d);
}

void ui_cockpit_atualizar(const ui_dados_t *d)
{
    if (s_tela == NULL || lv_screen_active() != s_tela) return;

    /* RPM e velocidade */
    uint32_t cor = est_cor(G_RPM, d);
    if (!s_ant.valido || s_ant.cor_rpm != cor) {
        lv_obj_set_style_arc_color(s_rpm_ind, lv_color_hex(cor), LV_PART_INDICATOR);
        s_ant.cor_rpm = cor;
    }
    if (!s_ant.valido || s_ant.rpm != d->rpm) {
        mover(s_rpm_ind, d->rpm);
        lv_label_set_text_fmt(s_rpm_val, "%u", (unsigned)d->rpm);
        s_ant.rpm = d->rpm;
    }
    if (!s_ant.valido || s_ant.vel != d->velocidade) {
        lv_label_set_text_fmt(s_vel_val, "%u", (unsigned)d->velocidade);
        s_ant.vel = d->velocidade;
    }
    uint32_t cor_num = d->dados ? EST_TEXTO : EST_APAGADO;
    if (!s_ant.valido || s_ant.cor_num != cor_num) {
        lv_obj_set_style_text_color(s_rpm_val, lv_color_hex(cor_num), 0);
        lv_obj_set_style_text_color(s_vel_val, lv_color_hex(cor_num), 0);
        s_ant.cor_num = cor_num;
    }

    for (int i = 0; i < N_LINHAS; i++) {
        atualizar_linha(&s_lin[i], LINHAS[i], d);
    }

    /* Status: ícone + texto em maiúsculas */
    const lv_image_dsc_t *img;
    uint32_t c;
    est_status_visual(d->status, &img, &c);
    if (!s_ant.valido || s_ant.status != (int)d->status) {
        lv_image_set_src(s_st_icone, img);
        est_pintar_icone(s_st_icone, c);
        lv_obj_set_style_text_color(s_st_txt, lv_color_hex(d->status == UI_STATUS_OK ? EST_TEXTO : c), 0);
        s_ant.status = (int)d->status;
    }
    if (d->msg_status != NULL) {
        char buf[48];
        size_t i = 0;
        for (; d->msg_status[i] != '\0' && i < sizeof(buf) - 1; i++) {
            unsigned char ch = (unsigned char)d->msg_status[i];
            buf[i] = (ch < 0x80) ? (char)toupper(ch) : (char)ch;
        }
        buf[i] = '\0';
        if (!s_ant.valido || strcmp(buf, s_ant.msg) != 0) {
            lv_label_set_text(s_st_txt, buf);
            strcpy(s_ant.msg, buf);
        }
    }

    /* Sinal */
    if (!d->enlace_ok)           { img = &img_k32_sem_sinal; c = EST_VERMELHO; }
    else if (d->rssi_dbm >= -60) { img = &img_k32_sinal_3;   c = EST_VERDE; }
    else if (d->rssi_dbm >= -70) { img = &img_k32_sinal_2;   c = EST_VERDE; }
    else if (d->rssi_dbm >= -80) { img = &img_k32_sinal_1;   c = EST_AMBAR; }
    else                         { img = &img_k32_sinal_0;   c = EST_VERMELHO; }
    if (!s_ant.valido || s_ant.sinal != img) {
        lv_image_set_src(s_sinal, img);
        est_pintar_icone(s_sinal, c);
        s_ant.sinal = img;
    }
    s_ant.valido = true;
}
