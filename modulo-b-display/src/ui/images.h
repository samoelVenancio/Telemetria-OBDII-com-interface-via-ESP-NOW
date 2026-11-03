#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_btn_back;
extern const lv_img_dsc_t img_ic_rpm;
extern const lv_img_dsc_t img_ic_velocidade;
extern const lv_img_dsc_t img_ic_consumo;
extern const lv_img_dsc_t img_ic_etanol;
extern const lv_img_dsc_t img_ic_temp_motor;
extern const lv_img_dsc_t img_ic_temp_ar;
extern const lv_img_dsc_t img_ic_map;
extern const lv_img_dsc_t img_ic_tanque;
extern const lv_img_dsc_t img_ic_bateria;
extern const lv_img_dsc_t img_ic_painel;
extern const lv_img_dsc_t img_ic_layout;
extern const lv_img_dsc_t img_ic_configuracoes;
extern const lv_img_dsc_t img_ic_alarmes;
extern const lv_img_dsc_t img_ic_superaquecimento;
extern const lv_img_dsc_t img_ic_motor_frio;
extern const lv_img_dsc_t img_ic_eco;
extern const lv_img_dsc_t img_ic_can_ok_32;
extern const lv_img_dsc_t img_ic_can_erro_32;
extern const lv_img_dsc_t img_ic_sem_sinal_32;
extern const lv_img_dsc_t img_ic_superaquecimento_32;
extern const lv_img_dsc_t img_ic_motor_frio_32;
extern const lv_img_dsc_t img_ic_trocar_marcha_32;
extern const lv_img_dsc_t img_ic_alerta_32;
extern const lv_img_dsc_t img_ic_sinal_0_32;
extern const lv_img_dsc_t img_ic_sinal_1_32;
extern const lv_img_dsc_t img_ic_sinal_2_32;
extern const lv_img_dsc_t img_ic_sinal_3_32;
extern const lv_img_dsc_t img_ic_tentar_denovo_32;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[29];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/