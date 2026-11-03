/**
 * ui_menu.c — ver ui_menu.h. Mesma grade 3 × 2 do painel.
 */
#include <stdio.h>
#include <string.h>

#include "ui_menu.h"

#define BLOCO_W   248
#define BLOCO_H   186
#define GRADE_X0  16
#define GRADE_Y0  80
#define FOLGA     12

typedef struct {
    const char *titulo;
    const char *sub;
    const lv_image_dsc_t *icone;
    ui_destino_t destino;
} bloco_t;

static const bloco_t BLOCOS[6] = {
    { "COCKPIT",       "Mostradores",         &img_k64_rpm,           UI_DEST_COCKPIT },
    { "PAINEL",        "Seis campos",         &img_k64_painel,        UI_DEST_PAINEL  },
    { "LAYOUT",        "Escolher os campos",  &img_k64_layout,        UI_DEST_LAYOUT  },
    { "ALARMES",       "Limites e avisos",    &img_k64_alarmes,       UI_DEST_ALARMES },
    { "CONFIGURAÇÕES", "CAN e versão",        &img_k64_configuracoes, UI_DEST_CONFIG  },
    { "ENLACE",        "--",                  &img_k64_sem_sinal,     UI_DEST_CONFIG  },
};

static lv_obj_t *s_tela;
static est_cabecalho_t s_cab;
static lv_obj_t *s_enl_icone, *s_enl_sub;
static char s_enl_txt[32];
static const void *s_enl_img;

static void ao_tocar(lv_event_t *e)
{
    est_ir((ui_destino_t)(intptr_t)lv_event_get_user_data(e));
}

void ui_menu_iniciar(void)
{
    s_tela = est_tela();
    est_cabecalho(s_tela, "TELEMETRIA  |  NEW FIESTA 1.6 FLEX", NULL, &s_cab);

    for (int i = 0; i < 6; i++) {
        const bloco_t *b = &BLOCOS[i];
        int x = GRADE_X0 + (i % 3) * (BLOCO_W + FOLGA);
        int y = GRADE_Y0 + (i / 3) * (BLOCO_H + FOLGA);
        lv_obj_t *c = est_cartao(s_tela, x, y, BLOCO_W, BLOCO_H);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c, ao_tocar, LV_EVENT_CLICKED, (void *)(intptr_t)b->destino);

        lv_obj_t *ic = est_icone(c, b->icone, i == 0 ? EST_LARANJA : EST_TEXTO);
        lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 26);
        lv_obj_t *t = est_texto(c, &ui_font_orb_22, EST_TEXTO, b->titulo);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 106);
        lv_obj_t *s = est_texto(c, &ui_font_orb_16, EST_ROTULO, b->sub);
        lv_obj_align(s, LV_ALIGN_TOP_MID, 0, 142);
        if (i == 5) {
            s_enl_icone = ic;
            s_enl_sub = s;
            est_pintar_icone(ic, EST_VERMELHO);
        }
    }
}

void ui_menu_abrir(void)
{
    s_enl_txt[0] = '\0';
    s_enl_img = NULL;
    lv_screen_load(s_tela);
}

void ui_menu_atualizar(const ui_dados_t *d)
{
    if (s_tela == NULL || lv_screen_active() != s_tela) return;
    est_cabecalho_atualizar(&s_cab, d);

    /* Bloco do enlace: barras + entrega/RSSI ao vivo */
    const lv_image_dsc_t *img;
    uint32_t cor;
    char txt[32];
    if (!d->enlace_ok) {
        img = &img_k64_sem_sinal; cor = EST_VERMELHO;
        snprintf(txt, sizeof(txt), "Sem sinal");
    } else {
        if (d->rssi_dbm >= -60)      { img = &img_k64_sinal_3; cor = EST_VERDE; }
        else if (d->rssi_dbm >= -70) { img = &img_k64_sinal_2; cor = EST_VERDE; }
        else if (d->rssi_dbm >= -80) { img = &img_k64_sinal_1; cor = EST_AMBAR; }
        else                         { img = &img_k64_sinal_0; cor = EST_VERMELHO; }
        snprintf(txt, sizeof(txt), "%u %%  |  %d dBm", (unsigned)d->entrega_pct, d->rssi_dbm);
    }
    if (s_enl_img != img) {
        lv_image_set_src(s_enl_icone, img);
        est_pintar_icone(s_enl_icone, cor);
        s_enl_img = img;
    }
    if (strcmp(txt, s_enl_txt) != 0) {
        lv_label_set_text(s_enl_sub, txt);
        lv_obj_align(s_enl_sub, LV_ALIGN_TOP_MID, 0, 142);
        strcpy(s_enl_txt, txt);
    }
}
