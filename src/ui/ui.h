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
#ifndef UI_H
#define UI_H
//----------------------------------------------------------------------------------------------------------------------
#include <stddef.h>
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    double voltage;
    double current;
} ui_measurement_entry_t;
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    ui_measurement_entry_t* entries;
    size_t count;
} ui_measurements_t;
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    size_t channel_count;
} ui_config_t;
//----------------------------------------------------------------------------------------------------------------------
int ui_init(ui_config_t config);
//----------------------------------------------------------------------------------------------------------------------
int ui_update_measurements(ui_measurements_t measurements);
//----------------------------------------------------------------------------------------------------------------------
int ui_update_button_pressed(int button_index);
//----------------------------------------------------------------------------------------------------------------------
int ui_loop(void);
//----------------------------------------------------------------------------------------------------------------------
#endif  // UI_H