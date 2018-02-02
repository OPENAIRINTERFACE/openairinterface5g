/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdlib.h>
#include <stdint.h>

#define G_LOG_DOMAIN ("UI_TREE")

#include <gtk/gtk.h>

#include "logs.h"
#include "rc.h"

#include "buffers.h"

#include "ui_main_screen.h"
#include "ui_tree_view.h"
#include "ui_callbacks.h"
#include "ui_filters.h"

#include "ui_signal_dissect_view.h"

typedef struct
{
    GtkListStore *store;
    GtkTreeModelFilter *filtered;
    uint32_t filtered_last_msg;
    uint32_t filtered_msg_number;
    uint32_t instance_number;
} ui_store_t;

static ui_store_t ui_store;

GtkWidget *ui_tree_view_menu;
ui_tree_view_menu_enable_t ui_tree_view_menu_enable[NUM_MENU_TYPE];

GdkEventButton *ui_tree_view_last_event;

static gboolean ui_tree_filter_messages(GtkTreeModel *model, GtkTreeIter *iter, ui_store_t *store)
{
    uint32_t msg_number;
    uint32_t message_id;
    uint32_t origin_task_id;
    uint32_t destination_task_id;
    uint32_t instance;

    gboolean enabled = TRUE;

    if (ui_filters.filters_enabled)
    {
        gtk_tree_model_get (model, iter, COL_MSG_NUM, &msg_number, COL_MESSAGE_ID, &message_id, COL_FROM_TASK_ID,
                            &origin_task_id, COL_TO_TASK_ID, &destination_task_id, COL_INSTANCE_ID, &instance, -1);
        if (msg_number != 0)
        {
            enabled = ui_filters_message_enabled (message_id, origin_task_id, destination_task_id, instance);

            if ((enabled) && (store->filtered_last_msg < msg_number))
            {
                store->filtered_last_msg = msg_number;
                store->filtered_msg_number++;
            }
            g_debug("ui_tree_filter_messages: %p %p %d m:%d o:%d d:%d i:%d %d %d", model, iter, msg_number, message_id, origin_task_id, destination_task_id, instance, enabled, store->filtered_msg_number);
        }
    }

    return enabled;
}

static gboolean onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
    g_info("onButtonPressed %p %p %p %d %d", treeview, event, userdata, event->type, event->button);
    ui_tree_view_last_event = event;

    return FALSE;
}

/*
 static gboolean onButtonRelease(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
 {
 g_info("onButtonRelease %p %p %p %d %d", treeview, event, userdata, event->type, event->button);
 // last_event = event;

 return FALSE;
 }
 */

static void ui_tree_view_init_list(GtkWidget *list)
{
    GtkCellRenderer *renderer_left;
    GtkCellRenderer *renderer_right;
    GtkTreeViewColumn *column;

    renderer_left = gtk_cell_renderer_text_new ();
    gtk_cell_renderer_set_padding (renderer_left, 5, 0);

    renderer_right = gtk_cell_renderer_text_new ();
    gtk_cell_renderer_set_alignment (renderer_right, 1, 0.5);
    gtk_cell_renderer_set_padding (renderer_right, 5, 0);

    column = gtk_tree_view_column_new_with_attributes ("MN", renderer_right, "text", COL_MSG_NUM, "foreground",
                                                       COL_FOREGROUND, "background", COL_BACKGROUND, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_append_column (GTK_TREE_VIEW(list), column);

    column = gtk_tree_view_column_new_with_attributes ("LTE Time", renderer_right, "text", COL_LTE_TIME, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_append_column (GTK_TREE_VIEW(list), column);

    column = gtk_tree_view_column_new_with_attributes ("Message", renderer_left, "text", COL_MESSAGE, "foreground",
                                                       COL_FOREGROUND, "background", COL_BACKGROUND, "strikethrough",
                                                       COL_STRIKETHROUGH, "style", COL_STYLE, "weight", COL_WEIGHT, "underline", COL_UNDERLINE, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_append_column (GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(ui_callback_on_tree_column_header_click),
                     (gpointer) COL_MESSAGE);

    column = gtk_tree_view_column_new_with_attributes ("From", renderer_left, "text", COL_FROM_TASK, NULL);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(ui_callback_on_tree_column_header_click),
                     (gpointer) COL_FROM_TASK);

    column = gtk_tree_view_column_new_with_attributes ("To", renderer_left, "text", COL_TO_TASK, NULL);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(ui_callback_on_tree_column_header_click),
                     (gpointer) COL_TO_TASK);

    column = gtk_tree_view_column_new_with_attributes ("Ins", renderer_right, "text", COL_INSTANCE, NULL);
    gtk_tree_view_column_set_alignment (column, 0.5);
    gtk_tree_view_append_column (GTK_TREE_VIEW(list), column);
    g_signal_connect(G_OBJECT(column), "clicked", G_CALLBACK(ui_callback_on_tree_column_header_click),
                     (gpointer) COL_INSTANCE);

    ui_store.store = gtk_list_store_new (NUM_COLS, // Number of columns
            // Displayed columns
            G_TYPE_UINT, // COL_MSG_NUM
            G_TYPE_STRING, // COL_LTE_TIME
            G_TYPE_STRING, // COL_MESSAGE
            G_TYPE_STRING, // COL_FROM_TASK
            G_TYPE_STRING, // COL_TO_TASK
            G_TYPE_STRING, // COL_INSTANCE
            // Not displayed columns
            // Id of some message fields to speed-up filtering
            G_TYPE_UINT, // COL_MESSAGE_ID
            G_TYPE_UINT, // COL_FROM_TASK_ID
            G_TYPE_UINT, // COL_TO_TASK_ID
            G_TYPE_UINT, // COL_INSTANCE_ID
            G_TYPE_STRING, // COL_FOREGROUND
            G_TYPE_STRING, // COL_BACKGROUND
            G_TYPE_BOOLEAN, // COL_STRIKETHROUGH
            G_TYPE_UINT,
            G_TYPE_UINT,
            G_TYPE_UINT,
            // Reference to the buffer here to avoid maintaining multiple lists.
            G_TYPE_POINTER);

    if (ui_store.store == NULL)
    {
        g_error("gtk_list_store_new failed");
    }

    ui_store.filtered = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (ui_store.store), NULL));
    gtk_tree_model_filter_set_visible_func (ui_store.filtered, (GtkTreeModelFilterVisibleFunc) ui_tree_filter_messages,
                                            &ui_store, NULL);

    gtk_tree_view_set_model (GTK_TREE_VIEW(list), GTK_TREE_MODEL(ui_store.filtered));

    gtk_tree_view_columns_autosize (GTK_TREE_VIEW(list));
}

static void ui_tree_view_add_to_list(GtkWidget *list, const gchar *lte_time, const uint32_t message_number,
                                     const uint32_t message_id, const gchar *signal_name, const uint32_t origin_task_id,
                                     const char *origin_task_name, const uint32_t destination_task_id,
                                     const char *destination_task_name, uint32_t instance_id, const char *instance_name, gpointer buffer)
{
    static int counter = 0;

    GtkTreeIter iter;
    gboolean enabled;
    int message_index;

    g_info("ui_tree_view_add_to_list: %d %d %d %d %d", message_number, message_id, origin_task_id, destination_task_id, instance_id);

    enabled = ui_filters_message_enabled (message_id, origin_task_id, destination_task_id, instance_id);
    message_index = ui_filters_search_id (&ui_filters.messages, message_id);

    g_info("ui_tree_view_add_to_list: %d %d", enabled, message_index);

    gtk_list_store_append (ui_store.store, &iter);
    gtk_list_store_set (ui_store.store, &iter,
    /* Columns */
    COL_MSG_NUM,
                        message_number, COL_LTE_TIME, lte_time, COL_MESSAGE, signal_name, COL_FROM_TASK, origin_task_name,
                        COL_TO_TASK, destination_task_name, COL_INSTANCE, instance_name, COL_MESSAGE_ID, message_id,
                        COL_FROM_TASK_ID, origin_task_id, COL_TO_TASK_ID, destination_task_id, COL_INSTANCE_ID, instance_id,
                        COL_BUFFER, buffer, COL_FOREGROUND, ui_filters.messages.items[message_index].foreground, COL_BACKGROUND,
                        ui_filters.messages.items[message_index].background, COL_STRIKETHROUGH, !enabled,
//                        COL_STYLE, (counter % 2) == 0 ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL,
//                        COL_WEIGHT, ((counter + 2) % 4) < 2 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
//                        COL_UNDERLINE, ((counter + 4) % 8) < 4 ?PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE,
                        COL_STYLE, PANGO_STYLE_NORMAL,
                        COL_WEIGHT, PANGO_WEIGHT_NORMAL,
                        COL_UNDERLINE, PANGO_UNDERLINE_NONE,
                        /* End of columns */
                        -1);
    counter++;
}

void ui_tree_view_destroy_list(GtkWidget *list)
{
    g_assert(list != NULL);

    gtk_list_store_clear (ui_store.store);
    ui_store.filtered_last_msg = 0;
    ui_store.filtered_msg_number = 0;
    ui_store.instance_number = 0;

    /* Reset number of messages */
    ui_main_data.nb_message_received = 0;
}

/* Search for the message with its message number equal to the given value or the previous one */
static gboolean ui_tree_view_search(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter,
                                    gpointer search_data)
{
    uint32_t msg_number;
    uint32_t key_value = atoi (key);

    gtk_tree_model_get (model, iter, column, &msg_number, -1);

    g_debug("ui_tree_view_search %d %d", key_value, msg_number);

    if (key_value == msg_number)
    {
        /* Value found, use this message */
        return 0;
    }

    {
        GtkTreeIter iter_next = *iter;

        if (gtk_tree_model_iter_next (model, &iter_next))
        {
            gtk_tree_model_get (model, &iter_next, column, &msg_number, -1);

            if (key_value < msg_number)
            {
                /* Next value will be greater, use this message */
                return 0;
            }
        }
        else
        {
            /* Last value, use this message */
            return 0;
        }
    }
    return 1;
}

static void ui_tree_view_create_menu(GtkWidget **menu)
{
    GtkWidget *menu_items;

    *menu = gtk_menu_new ();

    /* Create the "Message enable" menu-item */
    {
        /* Create a new menu-item */
        menu_items = gtk_check_menu_item_new ();
        ui_tree_view_menu_enable[MENU_MESSAGE].menu_enable = menu_items;

        /* Add it to the menu. */
        gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);
        g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_enable),
                         &ui_tree_view_menu_enable[MENU_MESSAGE]);

        /* Show the widget */
        gtk_widget_show (menu_items);
    }

    /* Create the "Destination task enable" menu-item */
    {
        /* Create a new menu-item */
        menu_items = gtk_check_menu_item_new ();
        ui_tree_view_menu_enable[MENU_FROM_TASK].menu_enable = menu_items;

        /* Add it to the menu. */
        gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);
        g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_enable),
                         &ui_tree_view_menu_enable[MENU_FROM_TASK]);

        /* Show the widget */
        gtk_widget_show (menu_items);
    }

    /* Create the "Origin task enable" menu-item */
    {
        /* Create a new menu-item */
        menu_items = gtk_check_menu_item_new ();
        ui_tree_view_menu_enable[MENU_TO_TASK].menu_enable = menu_items;

        /* Add it to the menu. */
        gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);
        g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_enable),
                         &ui_tree_view_menu_enable[MENU_TO_TASK]);

        /* Show the widget */
        gtk_widget_show (menu_items);
    }

    /* Create the "Instance enable" menu-item */
    {
        /* Create a new menu-item */
        menu_items = gtk_check_menu_item_new ();
        ui_tree_view_menu_enable[MENU_INSTANCE].menu_enable = menu_items;

        /* Add it to the menu. */
        gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);
        g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_enable),
                         &ui_tree_view_menu_enable[MENU_INSTANCE]);

        /* Show the widget */
        gtk_widget_show (menu_items);
    }

    /* Create the "Foreground color" menu-item */
    {
        static ui_tree_view_menu_color_t menu_color_foreground =
            {TRUE, &ui_tree_view_menu_enable[MENU_MESSAGE]};

        /* Create a new menu-item with a name */
        menu_items = gtk_menu_item_new_with_label ("Select message foreground color");

        /* Add it to the menu. */
        gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);
        g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_color),
                         &menu_color_foreground);

        /* Show the widget */
        gtk_widget_show (menu_items);
    }

    /* Create the "Background color" menu-item */
    {
        static ui_tree_view_menu_color_t menu_color_background =
            {FALSE, &ui_tree_view_menu_enable[MENU_MESSAGE]};

        /* Create a new menu-item with a name */
        menu_items = gtk_menu_item_new_with_label ("Select message background color");

        /* Add it to the menu. */
        gtk_menu_shell_append (GTK_MENU_SHELL(*menu), menu_items);
        g_signal_connect(G_OBJECT(menu_items), "activate", G_CALLBACK(ui_callback_on_menu_color),
                         &menu_color_background);

        /* Show the widget */
        gtk_widget_show (menu_items);
    }
}

int ui_tree_view_create(GtkWidget *window, GtkWidget *vbox)
{
    GtkWidget *hbox;
    GtkTreeSelection *selection;
    GtkWidget *scrolled_window;

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    ui_main_data.messages_list = gtk_tree_view_new ();
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(ui_main_data.messages_list), TRUE);
    gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(ui_main_data.messages_list), ui_tree_view_search, NULL, NULL);
    gtk_tree_view_set_search_entry (GTK_TREE_VIEW(ui_main_data.messages_list),
                                    GTK_ENTRY(ui_main_data.signals_go_to_entry));

    /* Disable multiple selection */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(ui_main_data.messages_list));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

    hbox = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

    gtk_container_add (GTK_CONTAINER(scrolled_window), ui_main_data.messages_list);

    ui_tree_view_init_list (ui_main_data.messages_list);
    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(ui_main_data.messages_list), TRUE);

    gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW(scrolled_window), 680);
    gtk_paned_pack1 (GTK_PANED (hbox), scrolled_window, FALSE, TRUE);
    ui_main_data.text_view = ui_signal_dissect_new (hbox);

    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 5);

    g_signal_connect(G_OBJECT(ui_main_data.messages_list), "button-press-event", G_CALLBACK (onButtonPressed), NULL);
    // g_signal_connect(G_OBJECT(ui_main_data.signalslist), "button-release-event", G_CALLBACK (onButtonRelease), NULL);

    /* Connect callback on row selection */
    gtk_tree_selection_set_select_function (selection, ui_callback_on_select_signal, ui_main_data.text_view, NULL);

    ui_store.filtered_last_msg = 0;
    ui_store.filtered_msg_number = 0;
    ui_store.instance_number = 0;

    ui_main_data.selection = selection;

    ui_tree_view_create_menu (&ui_tree_view_menu);

    return 0;
}

int ui_tree_view_new_signal_ind(const uint32_t message_number, const gchar *lte_time, const uint32_t message_id,
                                const char *message_name, const uint32_t origin_task_id, const char *origin_task_name,
                                const uint32_t destination_task_id, const char *destination_task_name, uint32_t instance_id,
                                gpointer buffer)
{
    char *instance_name = NULL;
    char instance_name_buffer[10];

    if (((instance_t) instance_id) < INSTANCE_DEFAULT)
    {
        if (ui_store.instance_number < (instance_id + 1))
        {
            int i;

            for (i = ui_store.instance_number; i <= instance_id; i++)
            {
                sprintf (instance_name_buffer, "%d", i);
                ui_filters_add (FILTER_INSTANCES, i, instance_name_buffer, ENTRY_ENABLED_TRUE, NULL, NULL);
            }
            ui_store.instance_number = (instance_id + 1);
            ui_destroy_filter_menu (FILTER_INSTANCES);
        }

        sprintf (instance_name_buffer, "%d", instance_id);
        instance_name = instance_name_buffer;
    }
    else
    {
        switch ((instance_t) instance_id)
        {
            case INSTANCE_DEFAULT:
                instance_name = "DEF";
                break;

            case INSTANCE_ALL:
                instance_name = "ALL";
                break;

            default:
                break;
        }

        if ((instance_name != NULL) && (ui_filters_search_id (&ui_filters.instances, instance_id) < 0))
        {
            ui_filters_add (FILTER_INSTANCES, instance_id, instance_name, ENTRY_ENABLED_TRUE, NULL, NULL);
        }
    }

    ui_tree_view_add_to_list (ui_main_data.messages_list, lte_time, message_number, message_id, message_name,
                              origin_task_id, origin_task_name, destination_task_id, destination_task_name, instance_id, instance_name,
                              buffer);

    return RC_OK;
}

void ui_tree_view_select_row(gint row)
{
    GtkTreePath *path_row;

    if (row >= 0)
    {
        path_row = gtk_tree_path_new_from_indices (row, -1);

        if ((ui_main_data.messages_list != NULL) && (path_row != NULL))
        {
            g_info("Select row %d", row);

            /* Select the message in requested row */
            gtk_tree_view_set_cursor (GTK_TREE_VIEW(ui_main_data.messages_list), path_row, NULL, FALSE);
            /* Center the message in the middle of the list if possible */
            gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(ui_main_data.messages_list), path_row, NULL, TRUE, 0.5, 0.0);
        }
    }
}

static gboolean updateColors(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    uint32_t message_id;
    uint32_t origin_task_id;
    uint32_t destination_task_id;
    uint32_t instance;

    gboolean enabled = FALSE;
    int message_index;

    gtk_tree_model_get (model, iter, COL_MESSAGE_ID, &message_id, COL_FROM_TASK_ID, &origin_task_id, COL_TO_TASK_ID,
                        &destination_task_id, COL_INSTANCE_ID, &instance, -1);
    enabled = ui_filters_message_enabled (message_id, origin_task_id, destination_task_id, instance);
    message_index = ui_filters_search_id (&ui_filters.messages, message_id);

    gtk_list_store_set (GTK_LIST_STORE(model), iter, COL_FOREGROUND,
                        ui_filters.messages.items[message_index].foreground, COL_BACKGROUND,
                        ui_filters.messages.items[message_index].background, COL_STRIKETHROUGH, !enabled, -1);

    return FALSE;
}

void ui_tree_view_refilter()
{
    ui_store.filtered_last_msg = 0;
    ui_store.filtered_msg_number = 0;

    /* Update foreground color of messages, this will also update filtered model */
    if (ui_store.store != NULL)
    {
        gtk_tree_model_foreach (GTK_TREE_MODEL(ui_store.store), updateColors, NULL);

        if (ui_main_data.messages_list != NULL)
        {
            GtkTreePath *path_row;

            /* Get the currently selected message */
            gtk_tree_view_get_cursor (GTK_TREE_VIEW(ui_main_data.messages_list), &path_row, NULL);
            if (path_row != NULL)
            {
                /* Center the message in the middle of the list if possible */
                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(ui_main_data.messages_list), path_row, NULL, TRUE, 0.5,
                                              0.0);
                g_info("ui_tree_view_refilter: center on message");
            }
        }
    }

    g_info("ui_tree_view_refilter: last message %d, %d messages displayed", ui_store.filtered_last_msg, ui_store.filtered_msg_number);
}

typedef struct foreach_message_params_s
{
    message_write_callback_t   callback;
    gboolean                    filter;
} foreach_message_params_t;

static gboolean foreach_message(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    foreach_message_params_t *params = (foreach_message_params_t *) data;
    const gchar *signal_name;
    uint32_t message_id;
    uint32_t origin_task_id;
    uint32_t destination_task_id;
    uint32_t instance;
    gpointer buffer = NULL;

    gtk_tree_model_get (model, iter, COL_MESSAGE, &signal_name, COL_MESSAGE_ID, &message_id, COL_FROM_TASK_ID, &origin_task_id, COL_TO_TASK_ID,
                        &destination_task_id, COL_INSTANCE_ID, &instance, COL_BUFFER, &buffer, -1);

    if (params->filter == TRUE)
    {
        gboolean enabled = FALSE;

        enabled = ui_filters_message_enabled (message_id, origin_task_id, destination_task_id, instance);
        if (enabled == FALSE)
        {
            buffer = NULL;
        }
    }

    if (buffer != NULL)
    {
        params->callback(buffer, signal_name);
    }

    return FALSE;
}

void ui_tree_view_foreach_message(message_write_callback_t callback, gboolean filter)
{
    foreach_message_params_t params = {callback, filter};

    gtk_tree_model_foreach (GTK_TREE_MODEL(ui_store.store), foreach_message, (void *) &params);
}

guint ui_tree_view_get_filtered_number(void)
{
    return ui_store.filtered_msg_number;
}
