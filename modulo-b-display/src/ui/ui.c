/*
 * PLACEHOLDER — este arquivo será SOBRESCRITO pelo Build do EEZ Studio.
 * Reproduz o ui.c que o EEZ gera para projetos LVGL sem Flow.
 */
#include "ui.h"
#include "screens.h"
#include "actions.h"
#include "vars.h"

#include <string.h>

static int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    lv_screen_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

void ui_init() {
    create_screens();
    loadScreen(SCREEN_ID_PRINCIPAL);
}

void ui_tick() {
    tick_screen(currentScreen);
}
