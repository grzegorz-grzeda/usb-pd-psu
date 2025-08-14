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
#include "measurement.h"
#include <stddef.h>
#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
//----------------------------------------------------------------------------------------------------------------------
LOG_MODULE_REGISTER(measurement, LOG_LEVEL_INF);
//----------------------------------------------------------------------------------------------------------------------
static const struct device* const sensors[] = {
    DEVICE_DT_GET(DT_NODELABEL(sensor0)),
    DEVICE_DT_GET(DT_NODELABEL(sensor1)),
    DEVICE_DT_GET(DT_NODELABEL(sensor2)),
};
//----------------------------------------------------------------------------------------------------------------------
#define MEASUREMENT_CHANNEL_COUNT (ARRAY_SIZE(sensors))
//----------------------------------------------------------------------------------------------------------------------
static measurement_channel_t* channels = NULL;
static measurement_callback_t measurement_callback = NULL;
static void* measurement_userdata = NULL;
//----------------------------------------------------------------------------------------------------------------------
void measurement_perform(void) {
    for (size_t i = 0; i < MEASUREMENT_CHANNEL_COUNT; i++) {
        if (!device_is_ready(sensors[i])) {
            LOG_ERR("Sensor %s is not ready", sensors[i]->name);
            channels[i].ok = false;
            continue;
        }
        if (sensor_sample_fetch(sensors[i]) < 0) {
            LOG_ERR("Failed to fetch sample from %s", sensors[i]->name);
            channels[i].ok = false;
            continue;
        }

        struct sensor_value voltage, current, power;
        if (sensor_channel_get(sensors[i], SENSOR_CHAN_VOLTAGE, &voltage) < 0 ||
            sensor_channel_get(sensors[i], SENSOR_CHAN_CURRENT, &current) < 0 ||
            sensor_channel_get(sensors[i], SENSOR_CHAN_POWER, &power) < 0) {
            LOG_ERR("Failed to get channel data from %s", sensors[i]->name);
            channels[i].ok = false;
            continue;
        }
        channels[i].voltage = sensor_value_to_double(&voltage);
        channels[i].current = sensor_value_to_double(&current);
        channels[i].power = sensor_value_to_double(&power);
        channels[i].ok = true;
    }
    if (!measurement_callback) {
        return;
    }
    measurement_callback((const measurement_channel_t* const)channels, MEASUREMENT_CHANNEL_COUNT, measurement_userdata);
}
//----------------------------------------------------------------------------------------------------------------------
int measurement_init(measurement_callback_t callback, void* userdata) {
    if (MEASUREMENT_CHANNEL_COUNT == 0) {
        LOG_ERR("No measurement channels defined");
        return -1;
    }

    if (!callback) {
        LOG_ERR("Measurement callback cannot be NULL");
        return -1;
    }

    bool all_ready = true;
    for (size_t i = 0; i < MEASUREMENT_CHANNEL_COUNT; i++) {
        if (!device_is_ready(sensors[i])) {
            LOG_ERR("Sensor %s is not ready", sensors[i]->name);
            all_ready = false;
        }
    }
    if (!all_ready) {
        LOG_ERR("Not all sensors are ready, cannot initialize measurements");
        return -1;
    }

    channels = calloc(MEASUREMENT_CHANNEL_COUNT, sizeof(measurement_channel_t));
    if (!channels) {
        LOG_ERR("Failed to allocate memory for measurement channels");
        return -1;
    }

    measurement_callback = callback;
    measurement_userdata = userdata;

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
size_t measurement_get_channel_count(void) {
    return MEASUREMENT_CHANNEL_COUNT;
}
//----------------------------------------------------------------------------------------------------------------------
