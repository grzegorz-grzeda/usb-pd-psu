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
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
static ui_measurement_t ui_get_measurement_callback(size_t channel_index, void* userdata);
static void measurement_timer_callback(struct k_timer* timer_id);
static void measurement_work_handler(struct k_work* work);
static measurement_channel_t* sensor_channels = NULL;
static size_t sensor_channel_count = 0;
K_MUTEX_DEFINE(ui_mutex);
K_TIMER_DEFINE(measurement_timer, measurement_timer_callback, NULL);
K_WORK_DEFINE(measurement_work, measurement_work_handler);
//----------------------------------------------------------------------------------------------------------------------
static void buttons_activated_callback(int button_index, void* userdata) {}
//----------------------------------------------------------------------------------------------------------------------
static ui_measurement_t ui_get_measurement_callback(size_t channel_index, void* userdata) {
    if (channel_index >= sensor_channel_count) {
        LOG_ERR("Channel index out of bounds: %zu", channel_index);
        return (ui_measurement_t){0};
    }
    k_mutex_lock(&ui_mutex, K_FOREVER);
    ui_measurement_t measurement = {
        .voltage = sensor_channels[channel_index].voltage,
        .current = sensor_channels[channel_index].current,
        .ok = sensor_channels[channel_index].ok,
    };
    k_mutex_unlock(&ui_mutex);
    return measurement;
}
//----------------------------------------------------------------------------------------------------------------------
static void measurement_callback(const measurement_channel_t* const channels, size_t channel_count, void* userdata) {
    if (channel_count != sensor_channel_count) {
        LOG_ERR("Channel count mismatch: expected %zu, got %zu", sensor_channel_count, channel_count);
        return;
    }
    k_mutex_lock(&ui_mutex, K_FOREVER);
    for (size_t i = 0; i < channel_count; i++) {
        sensor_channels[i] = channels[i];
    }
    k_mutex_unlock(&ui_mutex);
}
//----------------------------------------------------------------------------------------------------------------------
static void measurement_work_handler(struct k_work* work) {}
//----------------------------------------------------------------------------------------------------------------------
static void measurement_timer_callback(struct k_timer* timer_id) {
    k_work_submit(&measurement_work);
}
//----------------------------------------------------------------------------------------------------------------------
int main(void) {
    LOG_INF("Starting USB-PD PSU application");

    int err = measurement_init(measurement_callback, NULL);
    if (err) {
        LOG_ERR("Failed to initialize measurements: %d", err);
        return err;
    }

    sensor_channel_count = measurement_get_channel_count();
    if (sensor_channel_count == 0) {
        LOG_ERR("No measurement channels defined");
        return -1;
    }
    sensor_channels = calloc(sensor_channel_count, sizeof(measurement_channel_t));
    if (!sensor_channels) {
        LOG_ERR("Failed to allocate memory for sensor channels");
        return -ENOMEM;
    }

    ui_config_t ui_config = {
        .measurement_channel_count = sensor_channel_count,
        .get_measurement_callback = ui_get_measurement_callback,
        .userdata = NULL,
    };

    err = ui_init(ui_config);
    if (err) {
        LOG_ERR("Failed to initialize UI: %d", err);
        return err;
    }

    err = buttons_init(buttons_activated_callback, NULL);
    if (err) {
        LOG_ERR("Failed to initialize buttons: %d", err);
    }
    k_timer_start(&measurement_timer, K_MSEC(200), K_MSEC(200));

    while (1) {
        measurement_perform();

        ui_loop();
        k_sleep(K_MSEC(10));
    }

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
