#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_PRINCIPAL = 1,
    SCREEN_ID_PAINEL_PRINCIPAL = 2,
    SCREEN_ID_PAINEL_LAYOUT = 3,
    SCREEN_ID_PAINEL_CONFIG = 4,
    SCREEN_ID_PAINEL_ALARMS = 5,
    _SCREEN_ID_LAST = 5
};

typedef struct _objects_t {
    lv_obj_t *principal;
    lv_obj_t *painel_principal;
    lv_obj_t *painel_layout;
    lv_obj_t *painel_config;
    lv_obj_t *painel_alarms;
    lv_obj_t *btn_settings;
    lv_obj_t *btn_panel;
    lv_obj_t *btn_layout;
    lv_obj_t *btn_alarms;
    lv_obj_t *rpm_txt_1;
    lv_obj_t *vel_txt_1;
    lv_obj_t *temp_txt_1;
    lv_obj_t *rpm_txt_2;
    lv_obj_t *vel_txt_2;
    lv_obj_t *temp_txt_2;
    lv_obj_t *return_painel;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *dropdown_v1;
    lv_obj_t *obj2;
    lv_obj_t *dropdown_v2;
    lv_obj_t *obj3;
    lv_obj_t *dropdown_v3;
    lv_obj_t *obj4;
    lv_obj_t *dropdown_v4;
    lv_obj_t *obj5;
    lv_obj_t *dropdown_v5;
    lv_obj_t *obj6;
    lv_obj_t *dropdown_v6;
    lv_obj_t *obj7;
    lv_obj_t *return_layout;
    lv_obj_t *obj8;
    lv_obj_t *bps_can_config;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *retry_config;
    lv_obj_t *obj13;
    lv_obj_t *return_config;
    lv_obj_t *max_temp_alarm_sw;
    lv_obj_t *slide_max_temp;
    lv_obj_t *cold_engine_alarm_sw;
    lv_obj_t *eco_mode_sw;
    lv_obj_t *slide_eco_mode_rpm;
    lv_obj_t *return_alarms;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
} objects_t;

extern objects_t objects;

void create_screen_principal();
void tick_screen_principal();

void create_screen_painel_principal();
void tick_screen_painel_principal();

void create_screen_painel_layout();
void tick_screen_painel_layout();

void create_screen_painel_config();
void tick_screen_painel_config();

void create_screen_painel_alarms();
void tick_screen_painel_alarms();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/