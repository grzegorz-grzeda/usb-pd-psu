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
#include <lvgl.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    lv_obj_t* sensor_screen;
    lv_obj_t* title_label;
    lv_obj_t* voltage_text_label;
    lv_obj_t* voltage_label;
    lv_obj_t* voltage_unit_label;
    lv_obj_t* current_text_label;
    lv_obj_t* current_label;
    lv_obj_t* current_unit_label;
    lv_obj_t* power_text_label;
    lv_obj_t* power_label;
    lv_obj_t* power_unit_label;
} ui_channel_screen_t;
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
const struct device* const inas[] = {
    DEVICE_DT_GET(DT_NODELABEL(ina0)),
    DEVICE_DT_GET(DT_NODELABEL(ina1)),
    DEVICE_DT_GET(DT_NODELABEL(ina2)),
};
//----------------------------------------------------------------------------------------------------------------------
static uint8_t active_screen_index = 0;
static ui_channel_screen_t screens[sizeof(inas) / sizeof(inas[0])] = {0};
//----------------------------------------------------------------------------------------------------------------------
#define BUTTON0_NODE DT_NODELABEL(button0)
#define BUTTON1_NODE DT_NODELABEL(button1)
#define BUTTON2_NODE DT_NODELABEL(button2)

static const struct gpio_dt_spec button0_spec = GPIO_DT_SPEC_GET_OR(BUTTON0_NODE, gpios, {0});
static const struct gpio_dt_spec button1_spec = GPIO_DT_SPEC_GET_OR(BUTTON1_NODE, gpios, {0});
static const struct gpio_dt_spec button2_spec = GPIO_DT_SPEC_GET_OR(BUTTON2_NODE, gpios, {0});

static struct gpio_callback button0_cb;
static struct gpio_callback button1_cb;
static struct gpio_callback button2_cb;
//----------------------------------------------------------------------------------------------------------------------
static void timer_callback(lv_timer_t* timer) {
    struct sensor_value voltage, current, power;
    if (sensor_sample_fetch(inas[active_screen_index]) < 0) {
        LOG_ERR("Failed to fetch sample from %s", inas[active_screen_index]->name);
        return;
    }

    if (sensor_channel_get(inas[active_screen_index], SENSOR_CHAN_VOLTAGE, &voltage) < 0 ||
        sensor_channel_get(inas[active_screen_index], SENSOR_CHAN_CURRENT, &current) < 0 ||
        sensor_channel_get(inas[active_screen_index], SENSOR_CHAN_POWER, &power) < 0) {
        LOG_ERR("Failed to get sensor data from %s", inas[active_screen_index]->name);
        return;
    }

    lv_label_set_text_fmt(screens[active_screen_index].voltage_label, "%d.%03d", voltage.val1, voltage.val2 / 1000);
    lv_label_set_text_fmt(screens[active_screen_index].current_label, "%d.%03d", current.val1, current.val2 / 1000);
    lv_label_set_text_fmt(screens[active_screen_index].power_label, "%d.%03d", power.val1, power.val2 / 1000);
}
//----------------------------------------------------------------------------------------------------------------------
static void create_ui(void) {
    static lv_style_t style_dark;
    lv_style_init(&style_dark);

    lv_style_set_bg_color(&style_dark, lv_color_white());
    lv_style_set_bg_opa(&style_dark, LV_OPA_100);

    lv_style_set_text_color(&style_dark, lv_color_black());
    lv_style_set_border_color(&style_dark, lv_color_black());
    lv_style_set_outline_color(&style_dark, lv_color_black());

    for (size_t i = 0; i < ARRAY_SIZE(screens); i++) {
        screens[i].sensor_screen = lv_obj_create(NULL);
        lv_obj_add_style(screens[i].sensor_screen, &style_dark, 0);
        lv_obj_set_size(screens[i].sensor_screen, LV_HOR_RES, LV_VER_RES);

        screens[i].title_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text_fmt(screens[i].title_label, "Channel %zu", i);
        lv_obj_align(screens[i].title_label, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_text_font(screens[i].title_label, &lv_font_montserrat_16, 0);

        screens[i].voltage_text_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].voltage_text_label, "Voltage");
        lv_obj_align(screens[i].voltage_text_label, LV_ALIGN_TOP_LEFT, 10, 15);
        lv_obj_set_style_text_font(screens[i].voltage_text_label, &lv_font_montserrat_12, 0);
        screens[i].voltage_unit_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].voltage_unit_label, "V");
        lv_obj_align(screens[i].voltage_unit_label, LV_ALIGN_TOP_RIGHT, 0, 15);
        screens[i].voltage_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].voltage_label, "?");
        lv_obj_align(screens[i].voltage_label, LV_ALIGN_TOP_RIGHT, -20, 15);

        screens[i].current_text_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].current_text_label, "Current");
        lv_obj_align(screens[i].current_text_label, LV_ALIGN_TOP_LEFT, 10, 30);
        lv_obj_set_style_text_font(screens[i].current_text_label, &lv_font_montserrat_12, 0);
        screens[i].current_unit_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].current_unit_label, "A");
        lv_obj_align(screens[i].current_unit_label, LV_ALIGN_TOP_RIGHT, 0, 30);
        screens[i].current_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].current_label, "?");
        lv_obj_align(screens[i].current_label, LV_ALIGN_TOP_RIGHT, -20, 30);

        screens[i].power_text_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].power_text_label, "Power");
        lv_obj_align(screens[i].power_text_label, LV_ALIGN_TOP_LEFT, 10, 45);
        lv_obj_set_style_text_font(screens[i].power_text_label, &lv_font_montserrat_12, 0);
        screens[i].power_unit_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].power_unit_label, "W");
        lv_obj_align(screens[i].power_unit_label, LV_ALIGN_TOP_RIGHT, 0, 45);
        screens[i].power_label = lv_label_create(screens[i].sensor_screen);
        lv_label_set_text(screens[i].power_label, "?");
        lv_obj_align(screens[i].power_label, LV_ALIGN_TOP_RIGHT, -20, 45);
    }

    lv_screen_load(screens[active_screen_index].sensor_screen);
}
//----------------------------------------------------------------------------------------------------------------------
static void button_handler(const struct device* dev, struct gpio_callback* cb, uint32_t pins) {
    if ((pins & BIT(button0_spec.pin)) && (dev == button0_spec.port) && (active_screen_index != 0)) {
        active_screen_index = 0;
    } else if ((pins & BIT(button1_spec.pin)) && (dev == button1_spec.port) && (active_screen_index != 1)) {
        active_screen_index = 1;
    } else if ((pins & BIT(button2_spec.pin)) && (dev == button2_spec.port) && (active_screen_index != 2)) {
        active_screen_index = 2;
    } else {
        return;
    }
    lv_screen_load(screens[active_screen_index].sensor_screen);
}
//----------------------------------------------------------------------------------------------------------------------
static void configure_buttons(void) {
    if (!gpio_is_ready_dt(&button0_spec) || !gpio_is_ready_dt(&button1_spec) || !gpio_is_ready_dt(&button2_spec)) {
        LOG_ERR("Button GPIOs not ready");
        return;
    }
    gpio_pin_configure_dt(&button0_spec, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure_dt(&button1_spec, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_configure_dt(&button2_spec, GPIO_INPUT | GPIO_PULL_UP);

    gpio_pin_interrupt_configure_dt(&button0_spec, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button1_spec, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button2_spec, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button0_cb, button_handler, BIT(button0_spec.pin));
    gpio_init_callback(&button1_cb, button_handler, BIT(button1_spec.pin));
    gpio_init_callback(&button2_cb, button_handler, BIT(button2_spec.pin));

    if (gpio_add_callback(button0_spec.port, &button0_cb) < 0) {
        LOG_ERR("Failed to add button callback");
        return;
    }
    if (gpio_add_callback(button1_spec.port, &button1_cb) < 0) {
        LOG_ERR("Failed to add button callback");
        return;
    }
    if (gpio_add_callback(button2_spec.port, &button2_cb) < 0) {
        LOG_ERR("Failed to add button callback");
        return;
    }
}
//----------------------------------------------------------------------------------------------------------------------
int main(void) {
    LOG_INF("Starting USB-PD PSU application");
    const struct device* display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready, aborting");
        return 0;
    }
    LOG_INF("Display device ready");

    create_ui();
    configure_buttons();

    lv_timer_create(timer_callback, 200, NULL);

    lv_timer_handler();
    display_blanking_off(display_dev);

    while (1) {
        lv_timer_handler();
        k_sleep(K_MSEC(10));
    }

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
