#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: texto
//

void init_style_texto_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_text_color(style, lv_color_hex(0xffffff));
    lv_style_set_text_font(style, &lv_font_montserrat_26);
};

lv_style_t *get_style_texto_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_texto_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_texto(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_texto_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_texto(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_texto_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
// Style: redswitch
//

void init_style_redswitch_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xe0e0e0));
};

lv_style_t *get_style_redswitch_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_redswitch_MAIN_DEFAULT(style);
    }
    return style;
};

void init_style_redswitch_INDICATOR_CHECKED(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0xff0000));
};

lv_style_t *get_style_redswitch_INDICATOR_CHECKED() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_redswitch_INDICATOR_CHECKED(style);
    }
    return style;
};

void add_style_redswitch(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_redswitch_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, get_style_redswitch_INDICATOR_CHECKED(), LV_PART_INDICATOR | LV_STATE_CHECKED);
};

void remove_style_redswitch(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_redswitch_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_style(obj, get_style_redswitch_INDICATOR_CHECKED(), LV_PART_INDICATOR | LV_STATE_CHECKED);
};

//
// Style: backbtn
//

void init_style_backbtn_MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_bg_color(style, lv_color_hex(0x000000));
    lv_style_set_border_color(style, lv_color_hex(0xffffff));
    lv_style_set_border_width(style, 4);
};

lv_style_t *get_style_backbtn_MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style_backbtn_MAIN_DEFAULT(style);
    }
    return style;
};

void add_style_backbtn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style_backbtn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style_backbtn(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style_backbtn_MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_texto,
        add_style_redswitch,
        add_style_backbtn,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_texto,
        remove_style_redswitch,
        remove_style_backbtn,
    };
    remove_style_funcs[styleIndex](obj);
}