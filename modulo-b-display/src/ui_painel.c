/**
 * ui_painel.c — ver ui_painel.h.
 *
 * Grade 3 × 2 de cartões de 248 × 186. Quando o layout muda (tela Layout),
 * o cartão refaz o ícone, o nome e a barra da nova grandeza.
 */
#include <string.h>

#include "ui_painel.h"

#define N_CAMPOS   6
#define CARTAO_W   248
#define CARTAO_H   186
#define GRADE_X0   16
#define GRADE_Y0   80
#define FOLGA      12
#define PAD        14

typedef struct {
    lv_obj_t *cartao, *icone, *nome, *valor, *unid;
    est_barra_t barra;
    int grandeza;
    char txt[16];
    uint32_t cor;
} cartao_t;

static lv_obj_t *s_tela;
static est_cabecalho_t s_cab;
static cartao_t s_c[N_CAMPOS];
static void (*s_ao_tocar)(int campo, int grandeza);

static void ao_tocar(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (s_ao_tocar != NULL) {
        s_ao_tocar(i, s_c[i].grandeza);
    }
}

static void montar_conteudo(cartao_t *c, int g, const ui_dados_t *d)
{
    c->grandeza = g;
    lv_image_set_src(c->icone, est_icone_32(g));
    lv_label_set_text(c->nome, est_nome_curto(g));
    if (c->barra.raiz != NULL) {
        lv_obj_delete(c->barra.raiz);
    }
    est_barra_criar(&c->barra, c->cartao, PAD, CARTAO_H - PAD - 20, CARTAO_W - 2 * PAD, g, d);
    c->txt[0] = '\0';
    c->cor = 0xFFFFFFFF;
}

void ui_painel_iniciar(void)
{
    s_tela = est_tela();
    est_cabecalho(s_tela, "PAINEL", est_ao_voltar_menu, &s_cab);

    for (int i = 0; i < N_CAMPOS; i++) {
        cartao_t *c = &s_c[i];
        int x = GRADE_X0 + (i % 3) * (CARTAO_W + FOLGA);
        int y = GRADE_Y0 + (i / 3) * (CARTAO_H + FOLGA);
        c->cartao = est_cartao(s_tela, x, y, CARTAO_W, CARTAO_H);
        lv_obj_add_flag(c->cartao, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c->cartao, ao_tocar, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        c->icone = est_icone(c->cartao, &img_k32_rpm, EST_ROTULO);
        lv_obj_set_pos(c->icone, PAD, PAD);
        c->nome = est_texto(c->cartao, &ui_font_orb_16, EST_ROTULO, "");
        lv_obj_set_pos(c->nome, PAD + 42, PAD + 9);
        c->valor = est_texto(c->cartao, &ui_font_orb_56, EST_TEXTO, "--");
        lv_obj_set_pos(c->valor, PAD, 62);
        /* unidade embaixo do número: o valor usa a largura toda e o título
         * não disputa espaço com ela */
        c->unid = est_texto(c->cartao, &ui_font_orb_16, EST_ROTULO, "");
        lv_obj_set_pos(c->unid, PAD + 2, 124);
        c->grandeza = -1;
    }
}

void ui_painel_abrir(void)
{
    for (int i = 0; i < N_CAMPOS; i++) {
        s_c[i].txt[0] = '\0';
        s_c[i].cor = 0xFFFFFFFF;
        s_c[i].barra.valido = false;
    }
    lv_screen_load(s_tela);
}

void ui_painel_ao_tocar_campo(void (*cb)(int campo, int grandeza))
{
    s_ao_tocar = cb;
}

void ui_painel_atualizar(const ui_dados_t *d)
{
    if (s_tela == NULL || lv_screen_active() != s_tela) return;
    est_cabecalho_atualizar(&s_cab, d);

    for (int i = 0; i < N_CAMPOS; i++) {
        cartao_t *c = &s_c[i];
        int g = d->layout[i] < G_TOTAL ? d->layout[i] : G_RPM;
        if (g != c->grandeza) {
            montar_conteudo(c, g, d);
        }
        char v[16], u[8];
        est_valor(g, d, v, sizeof(v), u, sizeof(u));
        if (strcmp(v, c->txt) != 0) {
            lv_label_set_text(c->valor, v);
            lv_label_set_text(c->unid, u);
            strcpy(c->txt, v);
        }
        uint32_t cor = est_cor(g, d);
        if (cor != c->cor) {
            uint32_t cor_num = (cor == EST_VERMELHO || cor == EST_AMBAR || cor == EST_APAGADO)
                               ? cor : EST_TEXTO;
            lv_obj_set_style_text_color(c->valor, lv_color_hex(cor_num), 0);
            est_pintar_icone(c->icone, cor == EST_TEXTO ? EST_ROTULO : cor);
            /* cartão em alarme ganha borda vermelha */
            lv_obj_set_style_border_color(c->cartao, lv_color_hex(
                cor == EST_VERMELHO ? EST_VERMELHO : EST_BORDA), 0);
            c->cor = cor;
        }
        est_barra_atualizar(&c->barra, d);
    }
}
