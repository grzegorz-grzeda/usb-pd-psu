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
static lv_timer_t* lvgl_timer;
static ui_config_t ui_config = {0};
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(ui, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
static void ui_measurement_timer_callback(lv_timer_t* timer) {
    if (!ui_config.get_measurement_callback) {
        return;
    }
    for (size_t i = 0; i < ui_config.measurement_channel_count; i++) {
        ui_measurement_t measurement = ui_config.get_measurement_callback(i, ui_config.userdata);
        if (!measurement.ok) {
            lv_label_set_text(measurement_channel_labels[i].voltage_label, "N/A V");
            lv_label_set_text(measurement_channel_labels[i].current_label, "N/A A");
            continue;
        }
        char buffer[32];
        sprintf(buffer, "%.3f V", measurement.voltage);
        lv_label_set_text(measurement_channel_labels[i].voltage_label, buffer);
        sprintf(buffer, "%.3f A", measurement.current);
        lv_label_set_text(measurement_channel_labels[i].current_label, buffer);
    }
}
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
    if (config.get_measurement_callback == NULL) {
        LOG_ERR("Measurement callback must not be NULL");
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

    lvgl_timer = lv_timer_create(ui_measurement_timer_callback, 250, NULL);
    if (!lvgl_timer) {
        LOG_ERR("Failed to create LVGL timer");
        return -1;
    }

    render_splash_screen();
    display_blanking_off(display_dev);
    lv_timer_handler();

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ui_update_button_pressed(int button_index) {
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ui_loop(void) {
    lv_timer_handler();
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------