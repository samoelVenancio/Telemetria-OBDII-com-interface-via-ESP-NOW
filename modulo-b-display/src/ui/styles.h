#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: texto
lv_style_t *get_style_texto_MAIN_DEFAULT();
void add_style_texto(lv_obj_t *obj);
void remove_style_texto(lv_obj_t *obj);

// Style: redswitch
lv_style_t *get_style_redswitch_MAIN_DEFAULT();
lv_style_t *get_style_redswitch_INDICATOR_CHECKED();
void add_style_redswitch(lv_obj_t *obj);
void remove_style_redswitch(lv_obj_t *obj);

// Style: backbtn
lv_style_t *get_style_backbtn_MAIN_DEFAULT();
void add_style_backbtn(lv_obj_t *obj);
void remove_style_backbtn(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/