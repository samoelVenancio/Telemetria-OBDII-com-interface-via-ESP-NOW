/**
 * ui_icones.c — ver ui_icones.h.
 *
 * Por que só troca quando muda: lv_image_set_src() e a troca de estilo
 * invalidam a área do ícone e forçam redesenho. A 10 Hz, trocar sempre
 * custaria banda de PSRAM à toa. Cada ícone guarda o que está mostrando e
 * só mexe no LVGL se o pedido for diferente.
 */
#include <stddef.h>

#include "esp_log.h"
#include "lvgl.h"

#include "ui/screens.h"
#include "ui/images.h"
#include "ui_icones.h"

static const char *TAG = "ui_icones";

/* Paleta dos estados (mesma do HTML de ícones) */
#define COR_BRANCO   0xFFFFFF
#define COR_OK       0x46A758
#define COR_ATENCAO  0xE5A04D
#define COR_ALARME   0xE5484D

#define CAMPOS 6

/* Ícone de cada grandeza. MESMA ORDEM do grandeza_t em ui_ponte.c:
 * RPM, Velocidade, Consumo, Etanol, Temp. motor, Temp. ar, MAP,
 * Tanque (L), Tanque (%), Bateria */
static const lv_image_dsc_t *const ICONE_GRANDEZA[UI_ICONES_GRANDEZAS] = {
    &img_ic_rpm,
    &img_ic_velocidade,
    &img_ic_consumo,
    &img_ic_etanol,
    &img_ic_temp_motor,
    &img_ic_temp_ar,
    &img_ic_map,
    &img_ic_tanque,
    &img_ic_tanque,
    &img_ic_bateria,
};

typedef struct {
    const lv_image_dsc_t *icone;
    uint32_t cor;
} visual_t;

/* MESMA ORDEM de ui_status_t */
static const visual_t VISUAL_STATUS[] = {
    [UI_STATUS_SEM_ENLACE]       = { &img_ic_sem_sinal_32,        COR_ALARME  },
    [UI_STATUS_SEM_CAN]          = { &img_ic_can_erro_32,         COR_ATENCAO },
    [UI_STATUS_SUPERAQUECIMENTO] = { &img_ic_superaquecimento_32, COR_ALARME  },
    [UI_STATUS_MOTOR_FRIO]       = { &img_ic_motor_frio_32,       COR_ATENCAO },
    [UI_STATUS_ECO]              = { &img_ic_trocar_marcha_32,    COR_ATENCAO },
    [UI_STATUS_OK]               = { &img_ic_can_ok_32,           COR_OK      },
};

/* O que cada ícone dinâmico está mostrando agora (para só trocar se mudar) */
typedef struct {
    lv_obj_t *obj;
    const lv_image_dsc_t *icone;
    uint32_t cor;
    bool valido;
} slot_t;

static slot_t s_campo[CAMPOS];
static slot_t s_status;
static slot_t s_sinal;

static void pintar(lv_obj_t *obj, uint32_t cor)
{
    lv_obj_set_style_image_recolor(obj, lv_color_hex(cor), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void aplicar(slot_t *s, const lv_image_dsc_t *icone, uint32_t cor)
{
    if (s->obj == NULL) {
        return;
    }
    if (!s->valido || s->icone != icone) {
        lv_image_set_src(s->obj, icone);
        s->icone = icone;
    }
    if (!s->valido || s->cor != cor) {
        pintar(s->obj, cor);
        s->cor = cor;
    }
    s->valido = true;
}

/* Um bitmap A8 sem recolor sai PRETO no LVGL 9 — invisível no fundo preto.
 * Em vez de exigir um estilo em cada ícone no EEZ, percorre as telas e pinta
 * de branco toda lv_image que encontrar. Os dinâmicos são repintados depois
 * por aplicar(). Imagebuttons (voltar) continuam com o colorir_icone() da
 * ponte, que também define a cor de toque. */
static int pintar_imagens(lv_obj_t *raiz)
{
    if (raiz == NULL) {
        return 0;
    }
    int n = 0;
    if (lv_obj_check_type(raiz, &lv_image_class)) {
        pintar(raiz, COR_BRANCO);
        n++;
    }
    uint32_t filhos = lv_obj_get_child_count(raiz);
    for (uint32_t i = 0; i < filhos; i++) {
        n += pintar_imagens(lv_obj_get_child(raiz, (int32_t)i));
    }
    return n;
}

void ui_icones_iniciar(void)
{
    lv_obj_t *telas[] = {
        objects.principal, objects.painel_principal, objects.painel_layout,
        objects.painel_config, objects.painel_alarms, objects.painel_detalhe,
    };
    int pintados = 0;
    for (size_t i = 0; i < sizeof(telas) / sizeof(telas[0]); i++) {
        pintados += pintar_imagens(telas[i]);
    }

    lv_obj_t *campos[CAMPOS] = {
        objects.icon_v1, objects.icon_v2, objects.icon_v3,
        objects.icon_v4, objects.icon_v5, objects.icon_v6,
    };
    for (int i = 0; i < CAMPOS; i++) {
        s_campo[i] = (slot_t){ .obj = campos[i] };
    }
    s_status = (slot_t){ .obj = objects.icon_status };
    s_sinal = (slot_t){ .obj = objects.icon_sinal };

    /* Estado inicial: ainda não chegou nada do Módulo A */
    ui_icones_status(UI_STATUS_SEM_ENLACE);
    ui_icones_sinal(false, 0);

    ESP_LOGI(TAG, "%d imagem(ns) pintada(s) de branco", pintados);
}

const lv_image_dsc_t *ui_icones_da_grandeza(int grandeza)
{
    if (grandeza < 0 || grandeza >= UI_ICONES_GRANDEZAS) {
        return ICONE_GRANDEZA[0];
    }
    return ICONE_GRANDEZA[grandeza];
}

void ui_icones_campo(int campo, int grandeza, bool alerta)
{
    if (campo < 0 || campo >= CAMPOS || grandeza < 0 || grandeza >= UI_ICONES_GRANDEZAS) {
        return;
    }
    aplicar(&s_campo[campo], ICONE_GRANDEZA[grandeza], alerta ? COR_ALARME : COR_BRANCO);
}

void ui_icones_status(ui_status_t status)
{
    if ((unsigned)status >= sizeof(VISUAL_STATUS) / sizeof(VISUAL_STATUS[0])) {
        return;
    }
    aplicar(&s_status, VISUAL_STATUS[status].icone, VISUAL_STATUS[status].cor);
}

/* Faixas de RSSI: no ensaio de bancada o enlace bom ficou em torno de
 * -62 dBm (artigo, seção 4.2). Ajuste se o habitáculo mostrar outra coisa. */
void ui_icones_sinal(bool enlace_ok, int rssi_dbm)
{
    if (!enlace_ok) {
        aplicar(&s_sinal, &img_ic_sem_sinal_32, COR_ALARME);
    } else if (rssi_dbm >= -60) {
        aplicar(&s_sinal, &img_ic_sinal_3_32, COR_OK);
    } else if (rssi_dbm >= -70) {
        aplicar(&s_sinal, &img_ic_sinal_2_32, COR_OK);
    } else if (rssi_dbm >= -80) {
        aplicar(&s_sinal, &img_ic_sinal_1_32, COR_ATENCAO);
    } else {
        aplicar(&s_sinal, &img_ic_sinal_0_32, COR_ALARME);
    }
}
