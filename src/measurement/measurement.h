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
#ifndef MEASUREMENT_H
#define MEASUREMENT_H
//----------------------------------------------------------------------------------------------------------------------
#include <stdbool.h>
#include <stddef.h>
//----------------------------------------------------------------------------------------------------------------------
typedef struct measurement_channel {
    double voltage;
    double current;
    double power;
    bool ok;
} measurement_channel_t;
//----------------------------------------------------------------------------------------------------------------------
typedef void (*measurement_callback_t)(const measurement_channel_t* const channels,
                                       size_t channel_count,
                                       void* userdata);
//----------------------------------------------------------------------------------------------------------------------
int measurement_init(measurement_callback_t callback, void* userdata);
//----------------------------------------------------------------------------------------------------------------------
void measurement_perform(void);
//----------------------------------------------------------------------------------------------------------------------
size_t measurement_get_channel_count(void);
//----------------------------------------------------------------------------------------------------------------------
#endif  // MEASUREMENT_H