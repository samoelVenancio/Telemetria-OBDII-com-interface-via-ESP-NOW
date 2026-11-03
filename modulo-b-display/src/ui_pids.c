/**
 * ui_pids.c — ver ui_pids.h.
 *
 * A grade usa um label por PID (fundo do próprio label), 128 objetos ao todo:
 * metade do que seria com caixa + texto. Grade e lista só são repintadas
 * quando o pacote de PIDs muda; a linha de estado, quando o texto muda.
 */
#include <stdio.h>
#include <string.h>

#include "ui_pids.h"

#define LARGURA      768
#define GRADE_X0     16
#define PAD          12

#define BOTAO_W      220
#define BOTAO_H      44
#define BOTAO_Y      70

#define CARTAO1_Y    122
#define CARTAO1_H    156
#define LINHA_H      28
#define COLUNA_W     (LARGURA / 2)

#define CARTAO2_Y    288
#define CARTAO2_H    180
#define GRADE_COLS   16
#define GRADE_LINHAS 8
#define CELULA_W     44
#define CELULA_H     19
#define CELULA_FOLGA 2

/* Pedido sem resposta: a varredura leva até ~2,5 s e o A reenvia o resultado
 * a cada 2 s; passou disso, o A não atendeu (dormindo, fora de alcance) */
#define PEDIDO_TIMEOUT_MS  8000

/* Parâmetros que interessam ao projeto: o avanço e a lambda (pedidos), e os
 * que ajudariam o cálculo de consumo (ajustes de mistura, carga, MAF/vazão,
 * sabidamente ausentes neste carro — ficam aqui como prova). */
typedef struct {
    const char *nome;
    uint8_t pid;
    uint8_t pid_alt;    /* 0 = sem alternativa */
} interesse_t;

static const interesse_t INTERESSE[] = {
    { "AVANÇO DE IGNIÇÃO",    0x0E, 0    },
    { "SONDA O2 PRÉ-CAT.",    0x14, 0    },
    { "SONDA O2 PÓS-CAT.",    0x15, 0    },
    { "LAMBDA BANDA LARGA",   0x24, 0x34 },
    { "BORBOLETA",            0x11, 0    },
    { "AJUSTE CURTO (STFT)",  0x06, 0    },
    { "AJUSTE LONGO (LTFT)",  0x07, 0    },
    { "CARGA CALCULADA",      0x04, 0    },
    { "MAF",                  0x10, 0    },
    { "VAZÃO COMBUSTÍVEL",    0x5E, 0    },
};
#define N_INTERESSE (sizeof(INTERESSE) / sizeof(INTERESSE[0]))

static lv_obj_t *s_tela;
static est_cabecalho_t s_cab;
static lv_obj_t *s_estado;
static lv_obj_t *s_resp[N_INTERESSE];
static lv_obj_t *s_celula[GRADE_COLS * GRADE_LINHAS];

static telem_suporte_t s_ultimo;
static bool s_ultimo_tem;
static bool s_desenhado;      /* false: redesenha no próximo atualizar */

static bool (*s_pedir)(void);
static bool s_pedido_ativo;
static uint32_t s_pedido_tick;
static uint8_t s_pedido_numero;  /* número da última varredura quando pediu */
static bool s_pedido_falhou;     /* último pedido não foi atendido */

static void voltar_config(lv_event_t *e)
{
    (void)e;
    est_ir(UI_DEST_CONFIG);
}

static void ao_varrer(lv_event_t *e)
{
    (void)e;
    if (s_pedir == NULL || s_pedido_ativo) {
        return;
    }
    s_pedido_numero = s_ultimo_tem ? s_ultimo.numero : 0;
    s_pedido_falhou = false;
    s_pedido_ativo = s_pedir();
    s_pedido_tick = lv_tick_get();
    s_pedido_falhou = !s_pedido_ativo;
}

static lv_obj_t *botao(lv_obj_t *pai, const char *rotulo, lv_event_cb_t cb)
{
    lv_obj_t *b = lv_button_create(pai);
    lv_obj_set_size(b, BOTAO_W, BOTAO_H);
    lv_obj_set_style_bg_color(b, lv_color_hex(EST_TRILHO), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(EST_BORDA), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(EST_AZUL), LV_STATE_PRESSED);
    lv_obj_t *l = est_texto(b, &ui_font_orb_16, EST_TEXTO, rotulo);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

void ui_pids_iniciar(void)
{
    s_tela = est_tela();
    est_cabecalho(s_tela, "PIDS DO ECM", voltar_config, &s_cab);

    s_estado = est_texto(s_tela, &ui_font_orb_16, EST_ROTULO, "Aguardando o Módulo A");
    lv_obj_set_pos(s_estado, GRADE_X0 + 8, BOTAO_Y + (BOTAO_H - 16) / 2);

    lv_obj_t *b = botao(s_tela, "VARRER AGORA", ao_varrer);
    lv_obj_set_pos(b, GRADE_X0 + LARGURA - BOTAO_W, BOTAO_Y);

    /* Parâmetros de interesse: 2 colunas × 5 linhas */
    lv_obj_t *c1 = est_cartao(s_tela, GRADE_X0, CARTAO1_Y, LARGURA, CARTAO1_H);
    for (unsigned i = 0; i < N_INTERESSE; i++) {
        int x = 14 + (int)(i / 5) * COLUNA_W;
        int y = PAD + (int)(i % 5) * LINHA_H;
        lv_obj_t *nome = est_texto(c1, &ui_font_orb_16, EST_TEXTO, INTERESSE[i].nome);
        lv_obj_set_pos(nome, x, y);
        char cod[16];
        if (INTERESSE[i].pid_alt) {
            snprintf(cod, sizeof(cod), "%02X/%02X", INTERESSE[i].pid, INTERESSE[i].pid_alt);
        } else {
            snprintf(cod, sizeof(cod), "%02X", INTERESSE[i].pid);
        }
        lv_obj_t *l = est_texto(c1, &ui_font_orb_16, EST_ESCALA, cod);
        lv_obj_set_pos(l, x + 240, y);
        s_resp[i] = est_texto(c1, &ui_font_orb_16, EST_APAGADO, "--");
        lv_obj_set_pos(s_resp[i], x + COLUNA_W - 28 - 44, y);
    }

    /* Grade 01..80: 16 colunas × 8 linhas, PID = linha*16 + coluna + 1 */
    lv_obj_t *c2 = est_cartao(s_tela, GRADE_X0, CARTAO2_Y, LARGURA, CARTAO2_H);
    int x0 = (LARGURA - (GRADE_COLS * (CELULA_W + CELULA_FOLGA) - CELULA_FOLGA)) / 2;
    int y0 = (CARTAO2_H - (GRADE_LINHAS * (CELULA_H + CELULA_FOLGA) - CELULA_FOLGA)) / 2;
    for (int i = 0; i < GRADE_COLS * GRADE_LINHAS; i++) {
        char t[4];
        snprintf(t, sizeof(t), "%02X", i + 1);
        lv_obj_t *l = est_texto(c2, &ui_font_orb_16, EST_APAGADO, t);
        lv_obj_set_size(l, CELULA_W, CELULA_H);
        lv_obj_set_pos(l, x0 + (i % GRADE_COLS) * (CELULA_W + CELULA_FOLGA),
                       y0 + (i / GRADE_COLS) * (CELULA_H + CELULA_FOLGA));
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_radius(l, 4, 0);
        lv_obj_set_style_bg_color(l, lv_color_hex(EST_TRILHO), 0);
        lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
        s_celula[i] = l;
    }
}

void ui_pids_abrir(void)
{
    s_desenhado = false;
    lv_screen_load(s_tela);
}

void ui_pids_ao_pedir_varredura(bool (*pedir)(void))
{
    s_pedir = pedir;
}

/* "B1S1, B1S2" a partir do byte do PID 0x13 */
static void listar_sondas(uint8_t sondas, char *dest, size_t n)
{
    size_t pos = 0;
    dest[0] = '\0';
    for (int b = 0; b < 8 && pos < n; b++) {
        if (sondas & (1u << b)) {
            pos += (size_t)snprintf(dest + pos, n - pos, "%sB%dS%d", pos ? ", " : "",
                                    b / 4 + 1, b % 4 + 1);
        }
    }
}

static bool mapa_valido(const telem_suporte_t *s, bool tem)
{
    return tem && (s->estado == TELEM_VARREDURA_COMPLETA || s->estado == TELEM_VARREDURA_DEMO);
}

/* Linha de estado: o pedido em andamento tem prioridade sobre o resultado */
static void texto_estado(const telem_suporte_t *s, bool tem, char *txt, size_t n, uint32_t *cor)
{
    *cor = EST_ROTULO;
    if (s_pedido_ativo || (tem && s->estado == TELEM_VARREDURA_EM_ANDAMENTO)) {
        snprintf(txt, n, "Varrendo os PIDs do ECM...");
        *cor = EST_TEXTO;
    } else if (s_pedido_falhou) {
        snprintf(txt, n, "Módulo A não atendeu o pedido");
        *cor = EST_AMBAR;
    } else if (!tem) {
        snprintf(txt, n, "Aguardando o Módulo A");
    } else if (s->estado == TELEM_VARREDURA_NAO_FEITA) {
        snprintf(txt, n, "Ignição ligada? Toque em VARRER AGORA");
    } else if (s->estado == TELEM_VARREDURA_SEM_ECM) {
        snprintf(txt, n, "Varredura %u: ECM não respondeu", (unsigned)s->numero);
        *cor = EST_AMBAR;
    } else {
        unsigned total = 0;
        for (unsigned pid = 1; pid <= 0x80; pid++) {
            /* 0x20, 0x40... só encadeiam os bitmaps: não contam */
            total += (pid % 0x20u) != 0 && telem_pid_suportado(s, (uint8_t)pid);
        }
        char sondas[40];
        listar_sondas(s->sondas, sondas, sizeof(sondas));
        snprintf(txt, n, "Varredura %u: %u PIDs %s%s%s", (unsigned)s->numero, total,
                 s->estado == TELEM_VARREDURA_DEMO ? "simulados (demo)" : "suportados",
                 sondas[0] ? "  |  " : "", sondas);
        *cor = EST_TEXTO;
    }
}

static void desenhar_mapa(const telem_suporte_t *s, bool tem)
{
    bool ok = mapa_valido(s, tem);
    for (unsigned i = 0; i < N_INTERESSE; i++) {
        bool sim = ok && (telem_pid_suportado(s, INTERESSE[i].pid) ||
                          (INTERESSE[i].pid_alt && telem_pid_suportado(s, INTERESSE[i].pid_alt)));
        lv_label_set_text(s_resp[i], !ok ? "--" : sim ? "SIM" : "NÃO");
        lv_obj_set_style_text_color(s_resp[i],
                                    lv_color_hex(!ok ? EST_APAGADO : sim ? EST_VERDE : EST_ROTULO), 0);
    }
    for (int i = 0; i < GRADE_COLS * GRADE_LINHAS; i++) {
        bool sim = ok && telem_pid_suportado(s, (uint8_t)(i + 1));
        lv_obj_set_style_bg_color(s_celula[i], lv_color_hex(sim ? EST_AZUL : EST_TRILHO), 0);
        lv_obj_set_style_text_color(s_celula[i], lv_color_hex(sim ? EST_TEXTO : EST_APAGADO), 0);
    }
}

void ui_pids_atualizar(const ui_dados_t *d, const telem_suporte_t *s, bool tem)
{
    /* O pedido é acompanhado mesmo com a tela fechada */
    if (s_pedido_ativo) {
        if (tem && s->numero != s_pedido_numero && s->estado != TELEM_VARREDURA_EM_ANDAMENTO) {
            s_pedido_ativo = false;            /* resultado novo chegou */
        } else if (lv_tick_elaps(s_pedido_tick) > PEDIDO_TIMEOUT_MS) {
            s_pedido_ativo = false;
            s_pedido_falhou = true;
        }
    }

    if (s_tela == NULL || lv_screen_active() != s_tela) return;
    est_cabecalho_atualizar(&s_cab, d);

    char txt[96];
    uint32_t cor;
    texto_estado(s, tem, txt, sizeof(txt), &cor);
    if (!s_desenhado || strcmp(lv_label_get_text(s_estado), txt) != 0) {
        lv_label_set_text(s_estado, txt);
        lv_obj_set_style_text_color(s_estado, lv_color_hex(cor), 0);
    }

    if (s_desenhado && tem == s_ultimo_tem &&
        (!tem || memcmp(s, &s_ultimo, sizeof(*s)) == 0)) {
        return;
    }
    desenhar_mapa(s, tem);
    s_ultimo = *s;
    s_ultimo_tem = tem;
    s_desenhado = true;
}
