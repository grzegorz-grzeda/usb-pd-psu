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
#include "buttons.h"
#include <stddef.h>
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    const struct gpio_dt_spec spec;
    struct gpio_callback cb;
} button_t;
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(buttons, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
#define BUTTON0_NODE DT_NODELABEL(button0)
#define BUTTON1_NODE DT_NODELABEL(button1)
#define BUTTON2_NODE DT_NODELABEL(button2)
//----------------------------------------------------------------------------------------------------------------------
static button_t buttons[] = {
    {GPIO_DT_SPEC_GET_OR(BUTTON0_NODE, gpios, {0}), {}},
    {GPIO_DT_SPEC_GET_OR(BUTTON1_NODE, gpios, {0}), {}},
    {GPIO_DT_SPEC_GET_OR(BUTTON2_NODE, gpios, {0}), {}},
};
static const size_t buttons_count = ARRAY_SIZE(buttons);
//----------------------------------------------------------------------------------------------------------------------
static buttons_activate_callback_t buttons_activate_callback = NULL;
static void* buttons_userdata = NULL;
//----------------------------------------------------------------------------------------------------------------------
static void button_handler(const struct device* dev, struct gpio_callback* cb, uint32_t pins) {
    int button_index = -1;
    for (size_t i = 0; i < buttons_count; i++) {
        if (dev == buttons[i].spec.port && (pins & BIT(buttons[i].spec.pin))) {
            button_index = i;
            break;
        }
    }

    if (button_index < 0 || button_index >= buttons_count) {
        return;
    }

    if (!buttons_activate_callback) {
        return;
    }
    buttons_activate_callback(button_index, buttons_userdata);
}
//----------------------------------------------------------------------------------------------------------------------
int buttons_init(buttons_activate_callback_t callback, void* userdata) {
    if (!callback) {
        LOG_ERR("Callback cannot be NULL");
        return -EINVAL;
    }
    buttons_activate_callback = callback;
    buttons_userdata = userdata;

    for (size_t i = 0; i < buttons_count; i++) {
        if (!gpio_is_ready_dt(&buttons[i].spec)) {
            LOG_ERR("Button GPIO %zu not ready", i);
            return -ENODEV;
        }
        if (gpio_pin_configure_dt(&buttons[i].spec, GPIO_INPUT | GPIO_PULL_UP) < 0) {
            LOG_ERR("Failed to configure button GPIO %zu", i);
            return -EIO;
        }
        if (gpio_pin_interrupt_configure_dt(&buttons[i].spec, GPIO_INT_EDGE_TO_ACTIVE) < 0) {
            LOG_ERR("Failed to configure button interrupt %zu", i);
            return -EIO;
        }
        gpio_init_callback(&buttons[i].cb, button_handler, BIT(buttons[i].spec.pin));
        if (gpio_add_callback(buttons[i].spec.port, &buttons[i].cb) < 0) {
            LOG_ERR("Failed to add button callback %zu", i);
            return -EIO;
        }
    }
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
int buttons_get_count(void) {
    return buttons_count;
}
//----------------------------------------------------------------------------------------------------------------------
