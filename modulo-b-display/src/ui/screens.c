#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_principal() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.principal = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // btn_settings
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_settings = obj;
            lv_obj_set_pos(obj, 36, 138);
            lv_obj_set_size(obj, 727, 69);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Settings");
                }
            }
        }
        {
            // btn_panel
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_panel = obj;
            lv_obj_set_pos(obj, 37, 218);
            lv_obj_set_size(obj, 727, 69);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Main panel");
                }
            }
        }
        {
            // btn_layout
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_layout = obj;
            lv_obj_set_pos(obj, 37, 297);
            lv_obj_set_size(obj, 727, 69);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Layout");
                }
            }
        }
        {
            // btn_alarms
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.btn_alarms = obj;
            lv_obj_set_pos(obj, 38, 377);
            lv_obj_set_size(obj, 727, 69);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Alarms");
                }
            }
        }
    }
    
    tick_screen_principal();
}

void tick_screen_principal() {
}

void create_screen_painel_principal() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.painel_principal = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // rpm_txt_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.rpm_txt_1 = obj;
            lv_obj_set_pos(obj, 103, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // vel_txt_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.vel_txt_1 = obj;
            lv_obj_set_pos(obj, 103, 236);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // temp_txt_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.temp_txt_1 = obj;
            lv_obj_set_pos(obj, 103, 317);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // rpm_txt_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.rpm_txt_2 = obj;
            lv_obj_set_pos(obj, 447, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // vel_txt_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.vel_txt_2 = obj;
            lv_obj_set_pos(obj, 449, 236);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // temp_txt_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.temp_txt_2 = obj;
            lv_obj_set_pos(obj, 449, 317);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 292, 54);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text_static(obj, "Painel de leitura");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 23, 422);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text_static(obj, "Status: ");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 123, 422);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text(obj, "");
        }
        {
            // return_painel
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.return_painel = obj;
            lv_obj_set_pos(obj, 34, 28);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_btn_back, NULL);
        }
    }
    
    tick_screen_painel_principal();
}

void tick_screen_painel_principal() {
    {
        const char *new_val = get_var_value_1();
        const char *cur_val = lv_label_get_text(objects.rpm_txt_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.rpm_txt_1;
            lv_label_set_text(objects.rpm_txt_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_value_2();
        const char *cur_val = lv_label_get_text(objects.vel_txt_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.vel_txt_1;
            lv_label_set_text(objects.vel_txt_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_value_3();
        const char *cur_val = lv_label_get_text(objects.temp_txt_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.temp_txt_1;
            lv_label_set_text(objects.temp_txt_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_value_4();
        const char *cur_val = lv_label_get_text(objects.rpm_txt_2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.rpm_txt_2;
            lv_label_set_text(objects.rpm_txt_2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_value_5();
        const char *cur_val = lv_label_get_text(objects.vel_txt_2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.vel_txt_2;
            lv_label_set_text(objects.vel_txt_2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_value_6();
        const char *cur_val = lv_label_get_text(objects.temp_txt_2);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.temp_txt_2;
            lv_label_set_text(objects.temp_txt_2, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_msg_status();
        const char *cur_val = lv_label_get_text(objects.obj0);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj0;
            lv_label_set_text(objects.obj0, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_painel_layout() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.painel_layout = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 326, 73);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Layout info");
        }
        {
            // dropdown_v1
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.dropdown_v1 = obj;
            lv_obj_set_pos(obj, 202, 183);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, -292, -36);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Value 1");
        }
        {
            // dropdown_v2
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.dropdown_v2 = obj;
            lv_obj_set_pos(obj, 202, 249);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, -292, 30);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Value 2");
        }
        {
            // dropdown_v3
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.dropdown_v3 = obj;
            lv_obj_set_pos(obj, 202, 322);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, -292, 103);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Value 3");
        }
        {
            // dropdown_v4
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.dropdown_v4 = obj;
            lv_obj_set_pos(obj, 598, 184);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 86, -42);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Value 4");
        }
        {
            // dropdown_v5
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.dropdown_v5 = obj;
            lv_obj_set_pos(obj, 598, 253);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 86, 30);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Value 5");
        }
        {
            // dropdown_v6
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.dropdown_v6 = obj;
            lv_obj_set_pos(obj, 598, 321);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 86, 101);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Value 6");
        }
        {
            // return_layout
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.return_layout = obj;
            lv_obj_set_pos(obj, 34, 28);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_btn_back, NULL);
        }
    }
    
    tick_screen_painel_layout();
}

void tick_screen_painel_layout() {
}

void create_screen_painel_config() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.painel_config = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 356, 48);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Config");
        }
        {
            // bps_can_config
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.bps_can_config = obj;
            lv_obj_set_pos(obj, 337, 185);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options_static(obj, "Option 1\nOption 2\nOption 3");
            lv_dropdown_set_selected(obj, 0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, -243, -35);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "CAN baudrate");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, -318, 35);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Rev.");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj11 = obj;
            lv_obj_set_pos(obj, -216, 105);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Status ESP-NOW:");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj12 = obj;
            lv_obj_set_pos(obj, 337, 329);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // retry_config
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.retry_config = obj;
            lv_obj_set_pos(obj, 605, 321);
            lv_obj_set_size(obj, 147, 50);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x17ae82), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Retry");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj13 = obj;
            lv_obj_set_pos(obj, 337, 259);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // return_config
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.return_config = obj;
            lv_obj_set_pos(obj, 34, 28);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_btn_back, NULL);
        }
    }
    
    tick_screen_painel_config();
}

void tick_screen_painel_config() {
    {
        const char *new_val = get_var_status_espnow();
        const char *cur_val = lv_label_get_text(objects.obj12);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj12;
            lv_label_set_text(objects.obj12, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_rev_sys();
        const char *cur_val = lv_label_get_text(objects.obj13);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj13;
            lv_label_set_text(objects.obj13, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_painel_alarms() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.painel_alarms = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // max_temp_alarm_sw
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.max_temp_alarm_sw = obj;
            lv_obj_set_pos(obj, 42, 155);
            lv_obj_set_size(obj, 50, 25);
            lv_obj_add_state(obj, LV_STATE_CHECKED);
            add_style_redswitch(obj);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 101, 151);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text_static(obj, "Max temp. alarm");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj14 = obj;
            lv_obj_set_pos(obj, 416, 114);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text(obj, "");
        }
        {
            // slide_max_temp
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slide_max_temp = obj;
            lv_obj_set_pos(obj, 341, 163);
            lv_obj_set_size(obj, 401, 10);
            lv_slider_set_value(obj, 25, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0404), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfdfdfd), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        {
            // cold_engine_alarm_sw
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.cold_engine_alarm_sw = obj;
            lv_obj_set_pos(obj, 42, 265);
            lv_obj_set_size(obj, 50, 25);
            lv_obj_add_state(obj, LV_STATE_CHECKED);
            add_style_redswitch(obj);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 101, 263);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text_static(obj, "Cold engine Alarm");
        }
        {
            // eco_mode_sw
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.eco_mode_sw = obj;
            lv_obj_set_pos(obj, 42, 379);
            lv_obj_set_size(obj, 50, 25);
            lv_obj_add_state(obj, LV_STATE_CHECKED);
            add_style_redswitch(obj);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 101, 375);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text_static(obj, "ECO mode");
        }
        {
            // slide_eco_mode_rpm
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slide_eco_mode_rpm = obj;
            lv_obj_set_pos(obj, 341, 393);
            lv_obj_set_size(obj, 401, 10);
            lv_slider_set_range(obj, 0, 6500);
            lv_slider_set_value(obj, 25, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff0404), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xfdfdfd), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj15 = obj;
            lv_obj_set_pos(obj, 428, 346);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_texto(obj);
            lv_label_set_text(obj, "");
        }
        {
            // return_alarms
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.return_alarms = obj;
            lv_obj_set_pos(obj, 34, 28);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 48);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, &img_btn_back, NULL);
        }
    }
    
    tick_screen_painel_alarms();
}

void tick_screen_painel_alarms() {
    {
        const char *new_val = get_var_temp_max_alarm();
        const char *cur_val = lv_label_get_text(objects.obj14);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj14;
            lv_label_set_text(objects.obj14, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_eco_mode_rpm();
        const char *cur_val = lv_label_get_text(objects.obj15);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj15;
            lv_label_set_text(objects.obj15, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_principal,
    tick_screen_painel_principal,
    tick_screen_painel_layout,
    tick_screen_painel_config,
    tick_screen_painel_alarms,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 5) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_principal();
    create_screen_painel_principal();
    create_screen_painel_layout();
    create_screen_painel_config();
    create_screen_painel_alarms();
}