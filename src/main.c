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
#include <autoconf.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "buttons.h"
#include "measurement.h"
#include "ui.h"
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    lv_obj_t* channel_text_label;
    lv_obj_t* voltage_label;
    lv_obj_t* current_label;
} ui_channel_labels_t;
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
static lv_obj_t* sensor_screen;
static lv_obj_t* sensor_title_label;
static ui_channel_labels_t* sensor_channel_labels = NULL;
static size_t sensor_channel_count = 0;
static lv_timer_t* lvgl_timer;
//----------------------------------------------------------------------------------------------------------------------
extern const lv_img_dsc_t logo2_1b;
//----------------------------------------------------------------------------------------------------------------------
static void splash_timeout_cb(lv_timer_t* timer) {
    lv_obj_t* screen_to_load = (lv_obj_t*)lv_timer_get_user_data(timer);
    lv_screen_load(screen_to_load);
}
//----------------------------------------------------------------------------------------------------------------------
static int create_ui(size_t screen_cnt) {
    const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready, aborting");
        return -1;
    }

    if (screen_cnt == 0) {
        LOG_ERR("No measurement channels defined");
        return -1;
    }

    static lv_style_t style_dark;
    lv_style_init(&style_dark);

    lv_style_set_bg_color(&style_dark, lv_color_white());
    lv_style_set_bg_opa(&style_dark, LV_OPA_100);

    lv_style_set_text_color(&style_dark, lv_color_black());
    lv_style_set_border_color(&style_dark, lv_color_black());
    lv_style_set_outline_color(&style_dark, lv_color_black());

    lv_obj_t* splash_screen = lv_obj_create(NULL);
    lv_obj_add_style(splash_screen, &style_dark, 0);
    lv_obj_t* logo = lv_img_create(splash_screen);
    lv_img_set_src(logo, &logo2_1b);
    lv_obj_center(logo);

    sensor_channel_count = screen_cnt;
    sensor_screen = lv_obj_create(NULL);
    lv_obj_add_style(sensor_screen, &style_dark, 0);

    sensor_title_label = lv_label_create(sensor_screen);
    lv_label_set_text(sensor_title_label, "USB-PD PSU");
    lv_obj_align(sensor_title_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(sensor_title_label, &lv_font_montserrat_14, 0);

    sensor_channel_labels = calloc(sensor_channel_count, sizeof(ui_channel_labels_t));
    if (!sensor_channel_labels) {
        LOG_ERR("Failed to allocate memory for screens");
        return -1;
    }

    for (size_t i = 0; i < sensor_channel_count; i++) {
        size_t channel_index = (sensor_channel_count - i);
        sensor_channel_labels[i].channel_text_label = lv_label_create(sensor_screen);
        lv_label_set_text_fmt(sensor_channel_labels[i].channel_text_label, "CH%zu", channel_index);
        lv_obj_align(sensor_channel_labels[i].channel_text_label, LV_ALIGN_TOP_LEFT, 0, channel_index * 15);
        lv_obj_set_style_text_font(sensor_channel_labels[i].channel_text_label, &lv_font_montserrat_12, 0);

        sensor_channel_labels[i].voltage_label = lv_label_create(sensor_screen);
        lv_label_set_text(sensor_channel_labels[i].voltage_label, "? V");
        lv_obj_align(sensor_channel_labels[i].voltage_label, LV_ALIGN_TOP_RIGHT, -50, channel_index * 15);
        lv_obj_set_style_text_font(sensor_channel_labels[i].voltage_label, &lv_font_montserrat_12, 0);

        sensor_channel_labels[i].current_label = lv_label_create(sensor_screen);
        lv_label_set_text(sensor_channel_labels[i].current_label, "? A");
        lv_obj_align(sensor_channel_labels[i].current_label, LV_ALIGN_TOP_RIGHT, 0, channel_index * 15);
        lv_obj_set_style_text_font(sensor_channel_labels[i].current_label, &lv_font_montserrat_12, 0);
    }

    lv_screen_load(splash_screen);
    lv_timer_create(splash_timeout_cb, CONFIG_USB_PD_PSU_UI_SPLASH_SCREEN_TIMEOUT_MS, sensor_screen);

    lv_timer_handler();
    display_blanking_off(display_dev);
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
static void buttons_activated_callback(int button_index, void* userdata) {}
//----------------------------------------------------------------------------------------------------------------------
static void measurement_callback(const measurement_channel_t* const channels, size_t channel_count, void* userdata) {
    if (channel_count != sensor_channel_count) {
        LOG_ERR("Channel count mismatch: expected %zu, got %zu", sensor_channel_count, channel_count);
        return;
    }

    for (size_t i = 0; i < channel_count; i++) {
        if (!channels[i].ok) {
            lv_label_set_text(sensor_channel_labels[i].voltage_label, "N/A");
            lv_label_set_text(sensor_channel_labels[i].current_label, "N/A");
            continue;
        }
        char buffer[32];
        sprintf(buffer, "%.3f V", channels[i].voltage);
        lv_label_set_text(sensor_channel_labels[i].voltage_label, buffer);
        sprintf(buffer, "%.3f A", channels[i].current);
        lv_label_set_text(sensor_channel_labels[i].current_label, buffer);
    }
}
//----------------------------------------------------------------------------------------------------------------------
static void measurement_timer_callback(lv_timer_t* timer) {
    measurement_perform();
}
//----------------------------------------------------------------------------------------------------------------------
int main(void) {
    LOG_INF("Starting USB-PD PSU application");

    int err = measurement_init(measurement_callback, NULL);
    if (err) {
        LOG_ERR("Failed to initialize measurements: %d", err);
        return err;
    }

    size_t channel_count = measurement_get_channel_count();
    if (channel_count == 0) {
        LOG_ERR("No measurement channels defined");
        return -1;
    }

    err = create_ui(measurement_get_channel_count());
    if (err) {
        LOG_ERR("Failed to create UI: %d", err);
        return err;
    }

    err = buttons_init(buttons_activated_callback, NULL);
    if (err) {
        LOG_ERR("Failed to initialize buttons: %d", err);
    }

    lvgl_timer = lv_timer_create(measurement_timer_callback, 100, NULL);
    if (!lvgl_timer) {
        LOG_ERR("Failed to create LVGL timer");
        return -1;
    }

    while (1) {
        lv_timer_handler();
        k_sleep(K_MSEC(10));
    }

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
