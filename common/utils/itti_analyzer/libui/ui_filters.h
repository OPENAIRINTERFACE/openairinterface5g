/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */


#ifndef UI_FILTERS_H_
#define UI_FILTERS_H_

#include <stdint.h>

#include "itti_types.h"

#define SIGNAL_NAME_LENGTH  100
#define COLOR_SIZE          10

typedef enum
{
    FILTER_UNKNOWN, FILTER_MESSAGES, FILTER_ORIGIN_TASKS, FILTER_DESTINATION_TASKS, FILTER_INSTANCES,
} ui_filter_e;

typedef enum
{
    ENTRY_ENABLED_FALSE, ENTRY_ENABLED_TRUE, ENTRY_ENABLED_UNDEFINED,
} ui_entry_enabled_e;

typedef struct
{
    uint32_t id;
    char name[SIGNAL_NAME_LENGTH];
    uint8_t enabled;
    char foreground[COLOR_SIZE];
    char background[COLOR_SIZE];
    GtkWidget *menu_item;
} ui_filter_item_t;

typedef struct
{
    char *name;
    uint32_t allocated;
    uint32_t used;
    ui_filter_item_t *items;
} ui_filter_t;

typedef struct
{
    gboolean filters_enabled;
    ui_filter_t messages;
    ui_filter_t origin_tasks;
    ui_filter_t destination_tasks;
    ui_filter_t instances;
} ui_filters_t;

extern ui_filters_t ui_filters;

int ui_init_filters(int reset, int clear_ids);

gboolean ui_filters_enable(gboolean enabled);

int ui_filters_search_id(ui_filter_t *filter, uint32_t value);

void ui_filters_add(ui_filter_e filter, uint32_t value, const char *name, ui_entry_enabled_e entry_enabled,
                    const char *foreground, const char *background);

gboolean ui_filters_message_enabled(const uint32_t message, const uint32_t origin_task, const uint32_t destination_task,
                                    const uint32_t instance);

int ui_filters_read(const char *file_name);

int ui_filters_file_write(const char *file_name);

void ui_create_filter_menus(void);

void ui_destroy_filter_menus(void);

void ui_destroy_filter_menu(ui_filter_e filter);

void ui_show_filter_menu(GtkWidget **menu, ui_filter_t *filter);

#endif /* UI_FILTERS_H_ */
