/*
 * MIT License
 *
 * Copyright (c) 2025 G2Labs Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
//----------------------------------------------------------------------------------------------------------------------
#include "ui.h"
#include <autoconf.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include "app_version.h"
#include "zephyr/version.h"
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    lv_obj_t* channel_text_label;
    lv_obj_t* voltage_label;
    lv_obj_t* current_label;
} ui_channel_labels_t;
//----------------------------------------------------------------------------------------------------------------------
extern const lv_img_dsc_t logo2_1b;
//----------------------------------------------------------------------------------------------------------------------
static lv_obj_t* measurement_screen;
static lv_obj_t* measurement_title_label;
static ui_channel_labels_t* measurement_channel_labels = NULL;
static ui_config_t ui_config = {0};
static lv_obj_t* settings_screen = NULL;
static lv_obj_t* info_screen = NULL;
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(ui, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
static void screen_to_load_after_timeout_cb(lv_timer_t* timer) {
    lv_obj_t* screen_to_load = (lv_obj_t*)lv_timer_get_user_data(timer);
    lv_screen_load(screen_to_load);
    lv_timer_delete(timer);
}
//----------------------------------------------------------------------------------------------------------------------
static void set_default_style_for(lv_obj_t* obj) {
    static lv_style_t* style = NULL;
    if (!style) {
        style = calloc(1, sizeof(lv_style_t));
        lv_style_init(style);
        lv_style_set_bg_color(style, lv_color_white());
        lv_style_set_bg_opa(style, LV_OPA_100);
        lv_style_set_text_color(style, lv_color_black());
        lv_style_set_border_color(style, lv_color_black());
        lv_style_set_outline_color(style, lv_color_black());
    }
    lv_obj_add_style(obj, style, 0);
}
//----------------------------------------------------------------------------------------------------------------------
static void render_splash_screen(void) {
    lv_obj_t* splash_screen = lv_obj_create(NULL);
    set_default_style_for(splash_screen);

    lv_obj_t* logo = lv_img_create(splash_screen);
    lv_img_set_src(logo, &logo2_1b);
    lv_obj_center(logo);

    lv_screen_load(splash_screen);
    lv_timer_create(screen_to_load_after_timeout_cb, CONFIG_USB_PD_PSU_UI_SPLASH_SCREEN_TIMEOUT_MS, measurement_screen);
}
//----------------------------------------------------------------------------------------------------------------------
int ui_init(ui_config_t config) {
    const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready, aborting");
        return -1;
    }
    if (config.measurement_channel_count == 0) {
        LOG_ERR("Channel count must be greater than 0");
        return -1;
    }

    ui_config = config;

    measurement_screen = lv_obj_create(NULL);
    set_default_style_for(measurement_screen);

    measurement_title_label = lv_label_create(measurement_screen);
    lv_label_set_text(measurement_title_label, "USB-PD PSU");
    lv_obj_align(measurement_title_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(measurement_title_label, &lv_font_montserrat_14, 0);

    measurement_channel_labels = calloc(ui_config.measurement_channel_count, sizeof(ui_channel_labels_t));
    if (!measurement_channel_labels) {
        LOG_ERR("Failed to allocate memory for screens");
        return -1;
    }

    for (size_t i = 0; i < ui_config.measurement_channel_count; i++) {
        size_t channel_index = i + 1;
        measurement_channel_labels[i].channel_text_label = lv_label_create(measurement_screen);
        lv_label_set_text_fmt(measurement_channel_labels[i].channel_text_label, "CH%zu", channel_index);
        lv_obj_align(measurement_channel_labels[i].channel_text_label, LV_ALIGN_TOP_LEFT, 0, channel_index * 15);
        lv_obj_set_style_text_font(measurement_channel_labels[i].channel_text_label, &lv_font_montserrat_12, 0);

        measurement_channel_labels[i].voltage_label = lv_label_create(measurement_screen);
        lv_label_set_text(measurement_channel_labels[i].voltage_label, "? V");
        lv_obj_align(measurement_channel_labels[i].voltage_label, LV_ALIGN_TOP_RIGHT, -50, channel_index * 15);
        lv_obj_set_style_text_font(measurement_channel_labels[i].voltage_label, &lv_font_montserrat_12, 0);

        measurement_channel_labels[i].current_label = lv_label_create(measurement_screen);
        lv_label_set_text(measurement_channel_labels[i].current_label, "? A");
        lv_obj_align(measurement_channel_labels[i].current_label, LV_ALIGN_TOP_RIGHT, 0, channel_index * 15);
        lv_obj_set_style_text_font(measurement_channel_labels[i].current_label, &lv_font_montserrat_12, 0);
    }

    settings_screen = lv_obj_create(NULL);
    set_default_style_for(settings_screen);
    lv_obj_t* settings_title_label = lv_label_create(settings_screen);
    lv_label_set_text(settings_title_label, "Settings");
    lv_obj_align(settings_title_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(settings_title_label, &lv_font_montserrat_14, 0);

    info_screen = lv_obj_create(NULL);
    set_default_style_for(info_screen);
    lv_obj_t* info_title_label = lv_label_create(info_screen);
    lv_label_set_text(info_title_label, "Info");
    lv_obj_align(info_title_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(info_title_label, &lv_font_montserrat_14, 0);

    lv_obj_t* app_version_label = lv_label_create(info_screen);
    char version_text[64];
    sprintf(version_text, "App: %s", APP_VERSION_EXTENDED_STRING);
    lv_label_set_text(app_version_label, version_text);
    lv_obj_align(app_version_label, LV_ALIGN_TOP_LEFT, 0, 15);
    lv_obj_set_style_text_font(app_version_label, &lv_font_montserrat_12, 0);

    sprintf(version_text, "Zephyr: %s", KERNEL_VERSION_EXTENDED_STRING);
    lv_obj_t* zephyr_version_label = lv_label_create(info_screen);
    lv_label_set_text(zephyr_version_label, version_text);
    lv_obj_align(zephyr_version_label, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_set_style_text_font(zephyr_version_label, &lv_font_montserrat_12, 0);

    lv_obj_t* lvgl_version_label = lv_label_create(info_screen);
    sprintf(version_text, "LVGL: %d.%d.%d %s", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
    lv_label_set_text(lvgl_version_label, version_text);
    lv_obj_align(lvgl_version_label, LV_ALIGN_TOP_LEFT, 0, 45);
    lv_obj_set_style_text_font(lvgl_version_label, &lv_font_montserrat_12, 0);

    render_splash_screen();
    display_blanking_off(display_dev);
    lv_timer_handler();

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ui_update_measurements(ui_measurement_t* measurements, size_t channel_count) {
    if (!measurements || channel_count == 0) {
        return -1;
    }
    for (size_t i = 0; i < channel_count; i++) {
        if (!measurements[i].ok) {
            lv_label_set_text(measurement_channel_labels[i].voltage_label, "N/A V");
            lv_label_set_text(measurement_channel_labels[i].current_label, "N/A A");
            continue;
        }
        char buffer[32];
        sprintf(buffer, "%.3f V", measurements[i].voltage);
        lv_label_set_text(measurement_channel_labels[i].voltage_label, buffer);
        sprintf(buffer, "%.3f A", measurements[i].current);
        lv_label_set_text(measurement_channel_labels[i].current_label, buffer);
    }
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ui_update_button_pressed(int button_index) {
    if (button_index == 0) {
        lv_screen_load(measurement_screen);
    } else if (button_index == 1) {
        lv_screen_load(settings_screen);
    } else if (button_index == 2) {
        lv_screen_load(info_screen);
    } else {
        LOG_ERR("Invalid button index: %d", button_index);
        return -1;
    }
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ui_loop(void) {
    lv_timer_handler();
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------