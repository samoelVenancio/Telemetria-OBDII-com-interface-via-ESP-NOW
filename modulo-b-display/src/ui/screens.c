/*
 * PLACEHOLDER — este arquivo será SOBRESCRITO pelo Build do EEZ Studio.
 *
 * Interface mínima, feita à mão no MESMO formato que o EEZ gera (objects,
 * create_screen_x, tick_screen_x lendo get_var_*()), para dar para testar o
 * enlace ESP-NOW e a ponte de dados antes de desenhar as telas de verdade.
 * Sem capricho visual de propósito: é andaime, vai ser substituído.
 */
#include <string.h>

#include "screens.h"
#include "actions.h"
#include "vars.h"
#include "ui.h"

objects_t objects;

/* ---------- andaime genérico: "título: valor" ligado a uma variável ---------- */

typedef const char *(*getter_txt_t)();

typedef struct {
    lv_obj_t *rotulo;
    getter_txt_t ler;
} vinculo_txt_t;

typedef struct {
    lv_obj_t *obj;
    bool (*ler)();
} vinculo_oculto_t;

#define MAX_VINCULOS_POR_TELA 10

static vinculo_txt_t s_txt[_SCREEN_ID_LAST][MAX_VINCULOS_POR_TELA];
static uint8_t s_n_txt[_SCREEN_ID_LAST];
static vinculo_oculto_t s_oculto[_SCREEN_ID_LAST][2];
static uint8_t s_n_oculto[_SCREEN_ID_LAST];
static lv_obj_t *s_barra_rpm;

static lv_obj_t *criar_tela(const char *titulo) {
    lv_obj_t *tela = lv_obj_create(0);
    lv_obj_set_size(tela, 800, 480);
    lv_obj_t *t = lv_label_create(tela);
    lv_label_set_text_static(t, titulo);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(t, 16, 10);
    lv_obj_t *aviso = lv_label_create(tela);
    lv_label_set_text_static(aviso, "placeholder - exporte a UI do EEZ Studio");
    lv_obj_set_style_text_color(aviso, lv_color_hex(0x888888), 0);
    lv_obj_align(aviso, LV_ALIGN_TOP_RIGHT, -16, 16);
    return tela;
}

static lv_obj_t *criar_botao(lv_obj_t *pai, const char *texto, int x, int y, int w,
                             lv_event_cb_t acao) {
    lv_obj_t *b = lv_button_create(pai);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, 50);
    lv_obj_add_event_cb(b, acao, LV_EVENT_CLICKED, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text_static(l, texto);
    lv_obj_center(l);
    return b;
}

static void criar_barra_nav(lv_obj_t *tela) {
    static const char *nomes[] = { "Painel", "Alarme", "DTC", "Enlace", "Config" };
    static const lv_event_cb_t acoes[] = {
        action_ir_principal, action_ir_alarme, action_ir_dtc,
        action_ir_enlace, action_ir_config,
    };
    for (int i = 0; i < 5; i++) {
        criar_botao(tela, nomes[i], 8 + i * 158, 422, 150, acoes[i]);
    }
}

static void linha(int tela_idx, lv_obj_t *tela, int y, const char *titulo, getter_txt_t ler) {
    lv_obj_t *t = lv_label_create(tela);
    lv_label_set_text_static(t, titulo);
    lv_obj_set_pos(t, 24, y);
    lv_obj_t *v = lv_label_create(tela);
    lv_label_set_text(v, "");
    lv_obj_set_style_text_font(v, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(v, 220, y - 4);
    uint8_t n = s_n_txt[tela_idx]++;
    s_txt[tela_idx][n].rotulo = v;
    s_txt[tela_idx][n].ler = ler;
}

static void faixa(int tela_idx, lv_obj_t *tela, int y, const char *texto, uint32_t cor,
                  bool (*oculto)()) {
    lv_obj_t *l = lv_label_create(tela);
    lv_label_set_text_static(l, texto);
    lv_obj_set_style_text_color(l, lv_color_hex(cor), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(l, 420, y);
    uint8_t n = s_n_oculto[tela_idx]++;
    s_oculto[tela_idx][n].obj = l;
    s_oculto[tela_idx][n].ler = oculto;
}

/* Mesmo padrão do código gerado pelo EEZ: só toca no widget se mudou */
static void tick_generico(int tela_idx) {
    for (uint8_t i = 0; i < s_n_txt[tela_idx]; i++) {
        const char *novo = s_txt[tela_idx][i].ler();
        const char *atual = lv_label_get_text(s_txt[tela_idx][i].rotulo);
        if (strcmp(novo, atual) != 0) {
            lv_label_set_text(s_txt[tela_idx][i].rotulo, novo);
        }
    }
    for (uint8_t i = 0; i < s_n_oculto[tela_idx]; i++) {
        bool novo = s_oculto[tela_idx][i].ler();
        bool atual = lv_obj_has_flag(s_oculto[tela_idx][i].obj, LV_OBJ_FLAG_HIDDEN);
        if (novo != atual) {
            if (novo) lv_obj_add_flag(s_oculto[tela_idx][i].obj, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_remove_flag(s_oculto[tela_idx][i].obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

//
// Screens
//

void create_screen_principal() {
    const int i = SCREEN_ID_PRINCIPAL - 1;
    lv_obj_t *obj = criar_tela("Painel");
    objects.principal = obj;
    linha(i, obj, 60, "RPM", get_var_rpm_txt);
    s_barra_rpm = lv_bar_create(obj);
    lv_obj_set_pos(s_barra_rpm, 24, 96);
    lv_obj_set_size(s_barra_rpm, 360, 14);
    lv_bar_set_range(s_barra_rpm, 0, 7000);
    linha(i, obj, 125, "Velocidade (km/h)", get_var_vel_txt);
    linha(i, obj, 165, "Consumo", get_var_consumo_txt);
    linha(i, obj, 205, "Unidade", get_var_consumo_unid_txt);
    linha(i, obj, 245, "Etanol", get_var_etanol_txt);
    linha(i, obj, 285, "Arrefecimento", get_var_temp_txt);
    linha(i, obj, 325, "Tanque", get_var_litros_txt);
    linha(i, obj, 365, "Bateria", get_var_tensao_txt);
    faixa(i, obj, 60, "ALARME DE TEMPERATURA", 0xE5484D, get_var_alarme_oculto);
    faixa(i, obj, 100, "SEM SINAL DO MODULO A", 0xE5A04D, get_var_aviso_enlace_oculto);
    criar_barra_nav(obj);
    tick_screen_principal();
}

void tick_screen_principal() {
    tick_generico(SCREEN_ID_PRINCIPAL - 1);
    int32_t rpm = get_var_rpm();
    if (lv_bar_get_value(s_barra_rpm) != rpm) {
        lv_bar_set_value(s_barra_rpm, rpm, LV_ANIM_OFF);
    }
}

void create_screen_alarme() {
    const int i = SCREEN_ID_ALARME - 1;
    lv_obj_t *obj = criar_tela("Alarme de temperatura");
    objects.alarme = obj;
    linha(i, obj, 75, "Limiar", get_var_limiar_txt);
    linha(i, obj, 145, "Histerese", get_var_histerese_txt);
    linha(i, obj, 215, "Janela media", get_var_janela_txt);
    criar_botao(obj, "-", 420, 60, 70, action_limiar_menos);
    criar_botao(obj, "+", 500, 60, 70, action_limiar_mais);
    criar_botao(obj, "-", 420, 130, 70, action_histerese_menos);
    criar_botao(obj, "+", 500, 130, 70, action_histerese_mais);
    criar_botao(obj, "-", 420, 200, 70, action_janela_menos);
    criar_botao(obj, "+", 500, 200, 70, action_janela_mais);
    criar_botao(obj, "Salvar", 24, 270, 150, action_salvar_alarme);
    linha(i, obj, 345, "Media atual", get_var_media_txt);
    linha(i, obj, 380, "Estado", get_var_estado_alarme_txt);
    faixa(i, obj, 283, "salvo na NVS", 0x46A758, get_var_salvo_oculto);
    criar_barra_nav(obj);
    tick_screen_alarme();
}

void tick_screen_alarme() {
    tick_generico(SCREEN_ID_ALARME - 1);
}

void create_screen_dtc() {
    lv_obj_t *obj = criar_tela("Codigos de falha (DTC)");
    objects.dtc = obj;
    lv_obj_t *l = lv_label_create(obj);
    lv_label_set_text_static(l, "Nenhum codigo lido.\n"
                                "Depende do Modo 03 no Modulo A (fase 2).");
    lv_obj_set_pos(l, 24, 80);
    criar_barra_nav(obj);
    tick_screen_dtc();
}

void tick_screen_dtc() {
}

void create_screen_enlace() {
    const int i = SCREEN_ID_ENLACE - 1;
    lv_obj_t *obj = criar_tela("Enlace ESP-NOW");
    objects.enlace = obj;
    linha(i, obj, 70, "Estado", get_var_estado_enlace_txt);
    linha(i, obj, 120, "RSSI", get_var_rssi_txt);
    linha(i, obj, 170, "Recebidos", get_var_recebidos_txt);
    linha(i, obj, 220, "Perdidos", get_var_perdidos_txt);
    linha(i, obj, 270, "Entrega", get_var_entrega_txt);
    linha(i, obj, 320, "Taxa efetiva", get_var_taxa_txt);
    criar_barra_nav(obj);
    tick_screen_enlace();
}

void tick_screen_enlace() {
    tick_generico(SCREEN_ID_ENLACE - 1);
}

void create_screen_config() {
    const int i = SCREEN_ID_CONFIG - 1;
    lv_obj_t *obj = criar_tela("Configuracao");
    objects.config = obj;
    linha(i, obj, 70, "CAN do Modulo A", get_var_can_taxa_txt);
    linha(i, obj, 120, "Firmware", get_var_can_firmware_txt);
    linha(i, obj, 170, "Pedido", get_var_can_pedido_txt);
    criar_botao(obj, "250 kbit/s", 24, 230, 180, action_can_250k);
    criar_botao(obj, "500 kbit/s", 220, 230, 180, action_can_500k);
    criar_barra_nav(obj);
    tick_screen_config();
}

void tick_screen_config() {
    tick_generico(SCREEN_ID_CONFIG - 1);
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_principal,
    tick_screen_alarme,
    tick_screen_dtc,
    tick_screen_enlace,
    tick_screen_config,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < _SCREEN_ID_LAST) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

void create_screens() {
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), true,
                                              LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);

    create_screen_principal();
    create_screen_alarme();
    create_screen_dtc();
    create_screen_enlace();
    create_screen_config();
}
