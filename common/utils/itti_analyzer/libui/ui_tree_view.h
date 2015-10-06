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


#ifndef UI_TREE_VIEW_H_
#define UI_TREE_VIEW_H_

#include "ui_filters.h"
#include "ui_notifications.h"

typedef enum col_type_e
{
    COL_MSG_NUM = 0,
    COL_LTE_TIME,
    COL_MESSAGE,
    COL_FROM_TASK,
    COL_TO_TASK,
    COL_INSTANCE,

    COL_MESSAGE_ID,
    COL_FROM_TASK_ID,
    COL_TO_TASK_ID,
    COL_INSTANCE_ID,
    COL_FOREGROUND,
    COL_BACKGROUND,
    COL_STRIKETHROUGH,
    COL_STYLE,
    COL_WEIGHT,
    COL_UNDERLINE,

    COL_BUFFER,
    NUM_COLS
} col_type_t;

typedef enum ui_tree_view_menu_type_e
{
    MENU_MESSAGE = 0,
    MENU_FROM_TASK,
    MENU_TO_TASK,
    MENU_INSTANCE,
    NUM_MENU_TYPE,
} ui_tree_view_menu_type_t;

typedef struct ui_tree_view_menu_enable_s
{
    GtkWidget *menu_enable;
    ui_filter_item_t *filter_item;
} ui_tree_view_menu_enable_t;

typedef struct ui_tree_view_menu_color_s
{
    gboolean foreground;
    ui_tree_view_menu_enable_t *menu_enable;
} ui_tree_view_menu_color_t;

extern GtkWidget *ui_tree_view_menu;
extern ui_tree_view_menu_enable_t ui_tree_view_menu_enable[NUM_MENU_TYPE];

extern GdkEventButton *ui_tree_view_last_event;

int ui_tree_view_create(GtkWidget *window, GtkWidget *vbox);

int ui_tree_view_new_signal_ind(const uint32_t message_number, const gchar *lte_time,
                                const uint32_t message_id, const char *message_name,
                                const uint32_t origin_task_id, const char *origin_task,
                                const uint32_t destination_task_id, const char *to_task,
                                uint32_t instance, gpointer buffer);

void ui_tree_view_destroy_list(GtkWidget *list);

void ui_tree_view_select_row(gint row);

void ui_tree_view_refilter(void);

void ui_tree_view_foreach_message(message_write_callback_t callback, gboolean filter);

guint ui_tree_view_get_filtered_number(void);

#endif /* UI_TREE_VIEW_H_ */
