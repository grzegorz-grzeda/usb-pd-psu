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
#include <lvgl.h>
#include <stdlib.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
//----------------------------------------------------------------------------------------------------------------------
extern const lv_img_dsc_t logo2_1b;
//----------------------------------------------------------------------------------------------------------------------
static lv_obj_t* measurement_screen = NULL;
static lv_obj_t** channel_labels = NULL;
static size_t channel_count = 0;
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(ui, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
static void splash_screen_timeout_cb(lv_timer_t* timer) {
    lv_obj_t* main_screen = (lv_obj_t*)lv_timer_get_user_data(timer);
    lv_screen_load(main_screen);
}
//----------------------------------------------------------------------------------------------------------------------
int ui_init(ui_config_t config) {
    if (config.channel_count == 0) {
        LOG_ERR("Channel count must be greater than 0");
        return -1;
    }

    const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready, aborting");
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
    lv_obj_t* logo = lv_image_create(splash_screen);
    lv_image_set_src(logo, &logo2_1b);
    lv_obj_center(logo);

    measurement_screen = lv_obj_create(NULL);
    lv_obj_add_style(measurement_screen, &style_dark, 0);
    static lv_obj_t* title_label;
    title_label = lv_label_create(measurement_screen);
    lv_label_set_text(title_label, "USB-PD PSU");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, 0);

    channel_count = config.channel_count;
    channel_labels = calloc(channel_count, sizeof(lv_obj_t*));
    if (!channel_labels) {
        LOG_ERR("Failed to allocate memory for channel labels");
        return -1;
    }

    for (size_t i = 0; i < channel_count; i++) {
        lv_obj_t* channel_label = lv_label_create(measurement_screen);
        lv_label_set_text_fmt(channel_label, "%zu: ? V @ ? A", i + 1);
        lv_obj_align(channel_label, LV_ALIGN_TOP_LEFT, 10, 30 + (i * 20));
        lv_obj_set_style_text_font(channel_label, &lv_font_montserrat_12, 0);
        channel_labels[i] = channel_label;
    }

    lv_screen_load(splash_screen);
    lv_timer_create(splash_screen_timeout_cb, 3000, measurement_screen);
    display_blanking_off(display_dev);

    lv_timer_handler();

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int ui_update_measurements(ui_measurements_t measurements) {
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