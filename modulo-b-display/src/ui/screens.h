/*
 * PLACEHOLDER — este arquivo será SOBRESCRITO pelo Build do EEZ Studio.
 *
 * As telas do projeto EEZ precisam ter ESTES nomes, NESTA ordem (a primeira
 * da lista é a que abre no boot): principal, alarme, dtc, enlace, config.
 * A ponte (../ui_ponte.c) navega com loadScreen(SCREEN_ID_...).
 */
#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_PRINCIPAL = 1,
    SCREEN_ID_ALARME = 2,
    SCREEN_ID_DTC = 3,
    SCREEN_ID_ENLACE = 4,
    SCREEN_ID_CONFIG = 5,
    _SCREEN_ID_LAST = 5
};

/* Como no EEZ: as telas vêm primeiro, na ordem do enum (ui.c indexa por
 * posição); os widgets nomeados vêm depois. */
typedef struct _objects_t {
    lv_obj_t *principal;
    lv_obj_t *alarme;
    lv_obj_t *dtc;
    lv_obj_t *enlace;
    lv_obj_t *config;
} objects_t;

extern objects_t objects;

void create_screen_principal();
void tick_screen_principal();

void create_screen_alarme();
void tick_screen_alarme();

void create_screen_dtc();
void tick_screen_dtc();

void create_screen_enlace();
void tick_screen_enlace();

void create_screen_config();
void tick_screen_config();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
