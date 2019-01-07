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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>

#define G_LOG_DOMAIN ("UI_CB")

#include <gtk/gtk.h>

#include "logs.h"
#include "rc.h"

#include "socket.h"

#include "ui_notif_dlg.h"
#include "ui_main_screen.h"
#include "ui_menu_bar.h"
#include "ui_callbacks.h"
#include "ui_interface.h"
#include "ui_notifications.h"
#include "ui_tree_view.h"
#include "ui_signal_dissect_view.h"
#include "ui_filters.h"
#include "ui_notebook.h"

#include "types.h"
#include "locate_root.h"
#include "xml_parse.h"

static gboolean refresh_message_list =  TRUE;
static gboolean filters_changed =       FALSE;
static gboolean operation_running =     FALSE;

static const char  *ui_ip;
static uint16_t     ui_port;
static GtkWidget   *dialogbox_connect;
static gboolean     ui_auto_reconnect;

gboolean ui_callback_on_open_messages(GtkWidget *widget, gpointer data)
{
    gboolean refresh = (data != NULL) ? TRUE : FALSE;

    g_message("Open messages event occurred %d", refresh);

    if (operation_running == FALSE)
    {
        operation_running = TRUE;
        if (refresh && (ui_main_data.messages_file_name != NULL))
        {
            ui_messages_read (ui_main_data.messages_file_name);
        }
        else
        {
            ui_messages_open_file_chooser ();
        }
        operation_running = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_on_save_messages(GtkWidget *widget, gpointer data)
{
    gboolean filtered = (data != NULL) ? TRUE : FALSE;

    g_message("Save messages event occurred %d", filtered);

    if (operation_running == FALSE)
    {
        operation_running = TRUE;
        ui_messages_save_file_chooser(filtered);
        operation_running = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_on_filters_enabled(GtkToolButton *button, gpointer data)
{
    gboolean enabled;
    gboolean changed;

    enabled = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON(button));

    g_info("Filters enabled event occurred %d", enabled);

    changed = ui_filters_enable (enabled);

    if (changed)
    {
        /* Set the tool tip text */
        if (enabled)
        {
            gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(button), "Disable messages filtering");
        }
        else
        {
            gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(button), "Enable messages filtering");
        }
        ui_tree_view_refilter ();
    }

    return TRUE;
}

gboolean ui_callback_on_open_filters(GtkWidget *widget, gpointer data)
{
    gboolean refresh = (data != NULL) ? TRUE : FALSE;

    g_message("Open filters event occurred");

    if (operation_running == FALSE)
    {
        operation_running = TRUE;
        if (refresh && (ui_main_data.filters_file_name != NULL))
        {
            ui_filters_read (ui_main_data.filters_file_name);
        }
        else
        {
            ui_filters_open_file_chooser ();
        }
        operation_running = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_on_save_filters(GtkWidget *widget, gpointer data)
{
    if (operation_running == FALSE)
    {
        operation_running = TRUE;
        g_message("Save filters event occurred");
        ui_filters_save_file_chooser();
        operation_running = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_on_enable_filters(GtkWidget *widget, gpointer data)
{
    gboolean enabled;

    enabled = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.filters_enabled));
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.filters_enabled), !enabled);

    return TRUE;
}

gboolean ui_callback_on_about(GtkWidget *widget, gpointer data)
{
#if defined(PACKAGE_STRING)
    ui_notification_dialog (GTK_MESSAGE_INFO, FALSE, "about", "Eurecom %s", PACKAGE_STRING);
#else
    ui_notification_dialog (GTK_MESSAGE_INFO, FALSE, "about", "Eurecom itti_analyzer");
#endif

    return TRUE;
}

gint ui_callback_check_string (const char *string, const gint lenght, const guint message_number)
{
    gint real_length;

    real_length = strnlen (string, lenght);
    if (lenght != real_length)
    {
        if (message_number)
        {
            g_warning("Bad string format for message %u, '\\0' found before end of string (%d/%d)\n", message_number, real_length, lenght);
        }
        else
        {
            g_warning("Bad string format, '\\0' found before end of string (%d/%d)\n", real_length, lenght);
        }
        return (real_length);
    }
    else
    {
        return (lenght);
    }
}

gboolean ui_callback_on_select_signal(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
                                      gboolean path_currently_selected, gpointer user_data)
{
    static gpointer buffer_current;
    ui_text_view_t *text_view;
    GtkTreeIter iter;

    g_debug("Message selected %d %p %p %s", path_currently_selected, buffer_current, path, gtk_tree_path_to_string(path));

    if (!path_currently_selected)
    {
        text_view = (ui_text_view_t *) user_data;

        g_assert(text_view != NULL);

        if (gtk_tree_model_get_iter (model, &iter, path))
        {
            gpointer buffer;

            uint32_t message_number;
            uint32_t message_id;
            uint32_t origin_task_id;
            uint32_t destination_task_id;
            char *instance_name;
            uint32_t instance;
            char label[100];

            gtk_tree_model_get (model, &iter, COL_MSG_NUM, &message_number, COL_MESSAGE_ID, &message_id,
                                COL_FROM_TASK_ID, &origin_task_id, COL_TO_TASK_ID, &destination_task_id, COL_INSTANCE, &instance_name,
                                COL_INSTANCE_ID, &instance, COL_BUFFER, &buffer, -1);

            g_debug("  Get iter %p %p", buffer_current, buffer);

            if (ui_tree_view_last_event)
            {
                g_debug("last_event %p %d %d", ui_tree_view_last_event, ui_tree_view_last_event->type, ui_tree_view_last_event->button);

                if (ui_tree_view_last_event->type == GDK_BUTTON_PRESS)
                {
                    /* Callback is due to a button click */
                    if (ui_tree_view_last_event->button == 1)
                    {
                        /* It was a left mouse click */
                        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.signals_go_to_last_button), FALSE);
                    }

                    if (ui_tree_view_last_event->button == 3)
                    {
                        /* It was a right mouse click */
                        int item;

                        /* Clear event */
                        ui_tree_view_last_event = NULL;

                        /* Create filter menus to refers its items in the pop-up menu */
                        ui_create_filter_menus ();

                        g_info("Message selected right click %d %d %d %d", message_id, origin_task_id, destination_task_id, instance);

                        /* Message Id menu */
                        {
                            /* Invalidate associated menu item to avoid issue with call back when updating the menu item check state */
                            ui_tree_view_menu_enable[MENU_MESSAGE].filter_item = NULL;
                            item = ui_filters_search_id (&ui_filters.messages, message_id);
                            /* Update the menu item check state based on message ID state */
                            gtk_check_menu_item_set_active (
                                    GTK_CHECK_MENU_ITEM(ui_tree_view_menu_enable[MENU_MESSAGE].menu_enable),
                                    ui_filters.messages.items[item].enabled);
                            /* Set menu item label */
                            sprintf (label, "Message:  %s", message_id_to_string (message_id));
                            gtk_menu_item_set_label (GTK_MENU_ITEM(ui_tree_view_menu_enable[MENU_MESSAGE].menu_enable),
                                                     label);
                            /* Save menu item associated to this row */
                            ui_tree_view_menu_enable[MENU_MESSAGE].filter_item = &ui_filters.messages.items[item];
                        }

                        /* Origin task id */
                        {
                            /* Invalidate associated menu item to avoid issue with call back when updating the menu item check state */
                            ui_tree_view_menu_enable[MENU_FROM_TASK].filter_item = NULL;
                            item = ui_filters_search_id (&ui_filters.origin_tasks, origin_task_id);
                            /* Update the menu item check state based on message ID state */
                            gtk_check_menu_item_set_active (
                                    GTK_CHECK_MENU_ITEM(ui_tree_view_menu_enable[MENU_FROM_TASK].menu_enable),
                                    ui_filters.origin_tasks.items[item].enabled);
                            /* Set menu item label */
                            sprintf (label, "From:  %s", task_id_to_string (origin_task_id, origin_task_id_type));
                            gtk_menu_item_set_label (
                                    GTK_MENU_ITEM(ui_tree_view_menu_enable[MENU_FROM_TASK].menu_enable), label);
                            /* Save menu item associated to this row */
                            ui_tree_view_menu_enable[MENU_FROM_TASK].filter_item = &ui_filters.origin_tasks.items[item];
                        }

                        /* Destination task id */
                        {
                            /* Invalidate associated menu item to avoid issue with call back when updating the menu item check state */
                            ui_tree_view_menu_enable[MENU_TO_TASK].filter_item = NULL;
                            item = ui_filters_search_id (&ui_filters.destination_tasks, destination_task_id);
                            /* Update the menu item check state based on message ID state */
                            gtk_check_menu_item_set_active (
                                    GTK_CHECK_MENU_ITEM(ui_tree_view_menu_enable[MENU_TO_TASK].menu_enable),
                                    ui_filters.destination_tasks.items[item].enabled);
                            /* Set menu item label */
                            sprintf (label, "To:  %s",
                                     task_id_to_string (destination_task_id, destination_task_id_type));
                            gtk_menu_item_set_label (GTK_MENU_ITEM(ui_tree_view_menu_enable[MENU_TO_TASK].menu_enable),
                                                     label);
                            /* Save menu item associated to this row */
                            ui_tree_view_menu_enable[MENU_TO_TASK].filter_item =
                                    &ui_filters.destination_tasks.items[item];
                        }

                        /* Instance */
                        {
                            /* Invalidate associated menu item to avoid issue with call back when updating the menu item check state */
                            ui_tree_view_menu_enable[MENU_INSTANCE].filter_item = NULL;
                            item = ui_filters_search_id (&ui_filters.instances, instance);
                            /* Update the menu item check state based on message ID state */
                            gtk_check_menu_item_set_active (
                                    GTK_CHECK_MENU_ITEM(ui_tree_view_menu_enable[MENU_INSTANCE].menu_enable),
                                    ui_filters.instances.items[item].enabled);
                            /* Set menu item label */
                            sprintf (label, "Instance:  %s", instance_name);
                            gtk_menu_item_set_label (GTK_MENU_ITEM(ui_tree_view_menu_enable[MENU_INSTANCE].menu_enable),
                                                     label);
                            /* Save menu item associated to this row */
                            ui_tree_view_menu_enable[MENU_INSTANCE].filter_item = &ui_filters.instances.items[item];
                        }

                        gtk_menu_popup (GTK_MENU (ui_tree_view_menu), NULL, NULL, NULL, NULL, 0,
                                        gtk_get_current_event_time ());
                    }
                }

                /* Clear event */
                ui_tree_view_last_event = NULL;
            }

            if (buffer_current != buffer)
            {
                buffer_current = buffer;

                /* Clear the view */
                CHECK_FCT_DO(ui_signal_dissect_clear_view(text_view), return FALSE);

                if (ui_main_data.display_message_header)
                {
                    CHECK_FCT_DO(dissect_signal_header((buffer_t*)buffer, ui_signal_set_text, text_view), return FALSE);
                }

                if ((strcmp (message_id_to_string (message_id), "ERROR_LOG") == 0)
                        || (strcmp (message_id_to_string (message_id), "WARNING_LOG") == 0)
                        || (strcmp (message_id_to_string (message_id), "NOTICE_LOG") == 0)
                        || (strcmp (message_id_to_string (message_id), "INFO_LOG") == 0)
                        || (strcmp (message_id_to_string (message_id), "DEBUG_LOG") == 0)
                        || (strcmp (message_id_to_string (message_id), "GENERIC_LOG") == 0))
                {
                    gchar *data;
                    gint data_size;
                    uint32_t message_header_type_size;

                    if (ui_main_data.display_message_header)
                    {
                        ui_signal_set_text (text_view, "\n", 1);
                    }

                    message_header_type_size = get_message_header_type_size ();
                    data = (gchar *) buffer_at_offset ((buffer_t*) buffer, message_header_type_size);
                    data_size = get_message_size ((buffer_t*) buffer);
                    data_size = ui_callback_check_string (data, data_size, message_number);

                    g_info("    dump message %d: header type size: %u, data size: %u, buffer %p, follow last %d",
                           message_number, message_header_type_size, data_size, buffer, ui_main_data.follow_last);

                    ui_signal_set_text (text_view, data, data_size);
                }
                else
                {
                    g_info("    dissect message %d: id %d, buffer %p, follow last %d", message_number, message_id, buffer, ui_main_data.follow_last);

                    /* Dissect the signal */
                    CHECK_FCT_DO(dissect_signal((buffer_t*)buffer, ui_signal_set_text, text_view), return FALSE);
                }
            }
        }
    }
    return TRUE;
}

gboolean ui_callback_on_menu_enable(GtkWidget *widget, gpointer data)
{
    ui_tree_view_menu_enable_t *menu_enable = data;

    if (menu_enable->filter_item != NULL)
    {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_enable->filter_item->menu_item),
                                        gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menu_enable->menu_enable)));
        menu_enable->filter_item = NULL;
    }

    return TRUE;
}

gboolean ui_callback_on_menu_color(GtkWidget *widget, gpointer data)
{
    ui_tree_view_menu_color_t *menu_color = data;

    GdkRGBA color;
    GtkWidget *color_chooser;
    gint response;

    color_chooser = gtk_color_chooser_dialog_new ("Select message background color", GTK_WINDOW(ui_main_data.window));
    gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER(color_chooser), FALSE);
    response = gtk_dialog_run (GTK_DIALOG (color_chooser));

    if (response == GTK_RESPONSE_OK)
    {
        int red, green, blue;
        char *color_string;

        color_string =
                menu_color->foreground ?
                        menu_color->menu_enable->filter_item->foreground :
                        menu_color->menu_enable->filter_item->background;

        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(color_chooser), &color);

        red = (int) (color.red * 255);
        green = (int) (color.green * 255);
        blue = (int) (color.blue * 255);

        snprintf (color_string, COLOR_SIZE, "#%02x%02x%02x", red, green, blue);
        ui_tree_view_refilter ();

        g_info("Selected color for %s %f->%02x %f->%02x %f->%02x %s",
                  menu_color->menu_enable->filter_item->name, color.red, red, color.green, green, color.blue, blue, color_string);
    }
    gtk_widget_destroy (color_chooser);

    return TRUE;
}

void ui_signal_add_to_list(gpointer data, gpointer user_data)
{
    gboolean goto_last = user_data ? TRUE : FALSE;
    buffer_t *signal_buffer;
    GtkTreePath *path;
    GtkTreeViewColumn *focus_column;
    uint32_t lte_frame;
    uint32_t lte_slot;
    uint32_t origin_task_id;
    uint32_t destination_task_id;
    uint32_t instance;
    char lte_time[15];

    g_assert(data != NULL);
    g_assert(origin_task_id_type != NULL);
    g_assert(destination_task_id_type != NULL);

    gtk_tree_view_get_cursor (GTK_TREE_VIEW(ui_main_data.messages_list), &path, &focus_column);

    signal_buffer = (buffer_t *) data;

    lte_frame = get_lte_frame (signal_buffer);
    lte_slot = get_lte_slot (signal_buffer);
    sprintf (lte_time, "%d.%02d", lte_frame, lte_slot);

    get_message_id (root, signal_buffer, &signal_buffer->message_id);
    origin_task_id = get_task_id (signal_buffer, origin_task_id_type);
    destination_task_id = get_task_id (signal_buffer, destination_task_id_type);
    instance = get_instance (signal_buffer);

    ui_tree_view_new_signal_ind (signal_buffer->message_number, lte_time, signal_buffer->message_id,
                                 message_id_to_string (signal_buffer->message_id), origin_task_id,
                                 task_id_to_string (origin_task_id, origin_task_id_type), destination_task_id,
                                 task_id_to_string (destination_task_id, destination_task_id_type), instance, data);

    if ((strcmp (message_id_to_string (signal_buffer->message_id), "ERROR_LOG") == 0)
            || (strcmp (message_id_to_string (signal_buffer->message_id), "WARNING_LOG") == 0)
            || (strcmp (message_id_to_string (signal_buffer->message_id), "NOTICE_LOG") == 0)
            || (strcmp (message_id_to_string (signal_buffer->message_id), "INFO_LOG") == 0)
            || (strcmp (message_id_to_string (signal_buffer->message_id), "DEBUG_LOG") == 0)
            || (strcmp (message_id_to_string (signal_buffer->message_id), "GENERIC_LOG") == 0))
    {
        gchar *string_terminal;
        gint string_terminal_size;
        uint32_t message_header_type_size;

        message_header_type_size = get_message_header_type_size ();
        string_terminal = (gchar *) buffer_at_offset ((buffer_t*) signal_buffer, message_header_type_size);
        string_terminal_size = get_message_size ((buffer_t*) signal_buffer);
        string_terminal_size = ui_callback_check_string (string_terminal, string_terminal_size, signal_buffer->message_number);
        ui_notebook_terminal_append_data(string_terminal, string_terminal_size);
    }

    /* Increment number of messages */
    ui_main_data.nb_message_received++;

    if ((ui_main_data.follow_last) && (goto_last))
    {
        /* Advance to the new last signal */
        ui_tree_view_select_row (ui_tree_view_get_filtered_number () - 1);
    }
}

static gboolean ui_handle_update_signal_list(gint fd, void *data, size_t data_length)
{
    pipe_new_signals_list_message_t *signal_list_message;

    /* Enable buttons to move in the list of signals */
    ui_set_sensitive_move_buttons (TRUE);

    signal_list_message = (pipe_new_signals_list_message_t *) data;

    g_assert(signal_list_message != NULL);
    g_assert(signal_list_message->signal_list != NULL);

    g_list_foreach (signal_list_message->signal_list, ui_signal_add_to_list, (gpointer) TRUE);

    /* Free the list but not user data associated with each element */
    g_list_free (signal_list_message->signal_list);
    /* Free the message */
    free (signal_list_message);

    ui_gtk_flush_events ();

    return TRUE;
}

static gboolean ui_handle_socket_connection_failed(gint fd)
{
    gtk_dialog_response(GTK_DIALOG (dialogbox_connect), GTK_RESPONSE_REJECT);

    return TRUE;
}

static gboolean ui_handle_socket_connection_lost(gint fd)
{
    if (operation_running)
    {
        operation_running = FALSE;
        if (ui_auto_reconnect)
        {
            ui_callback_on_connect (NULL, (gpointer) FALSE);
        }
        else
        {
            ui_enable_connect_button();
            ui_set_sensitive_save_message_buttons (TRUE);
        }
    }

    return TRUE;
}

static gboolean ui_handle_socket_xml_definition(gint fd, void *data, size_t data_length)
{
    pipe_xml_definition_message_t *xml_definition_message;

    xml_definition_message = (pipe_xml_definition_message_t *) data;
    g_assert(xml_definition_message != NULL);
    g_assert(data_length == sizeof(pipe_xml_definition_message_t));

    xml_parse_buffer (xml_definition_message->xml_definition, xml_definition_message->xml_definition_length);

    return TRUE;
}

gboolean ui_pipe_callback(gint source, gpointer user_data)
{
    void *input_data = NULL;
    size_t input_data_length = 0;
    pipe_input_header_t input_header;

    /* Read the header */
    if (read (source, &input_header, sizeof(input_header)) < 0)
    {
        g_warning("Failed to read from pipe %d: %s", source, g_strerror(errno));
        return FALSE;
    }

    input_data_length = input_header.message_size - sizeof(input_header);

    /* Checking for non-header part */
    if (input_data_length > 0)
    {
        input_data = malloc (input_data_length);

        if (read (source, input_data, input_data_length) < 0)
        {
            g_warning("Failed to read from pipe %d: %s", source, g_strerror(errno));
            return FALSE;
        }
    }

    switch (input_header.message_type)
    {
        case UI_PIPE_CONNECTION_FAILED:
            return ui_handle_socket_connection_failed (source);

        case UI_PIPE_CONNECTION_LOST:
            return ui_handle_socket_connection_lost (source);

        case UI_PIPE_XML_DEFINITION:
            gtk_dialog_response(GTK_DIALOG (dialogbox_connect), GTK_RESPONSE_OK);
            return ui_handle_socket_xml_definition (source, input_data, input_data_length);

        case UI_PIPE_UPDATE_SIGNAL_LIST:
            return ui_handle_update_signal_list (source, input_data, input_data_length);

        default:
            g_warning("[gui] Unhandled message type %u", input_header.message_type);
            g_assert_not_reached();
    }
    return FALSE;
}

gboolean ui_callback_on_auto_reconnect(GtkWidget *widget, gpointer data)
{
    gboolean enabled;
    gboolean changed = TRUE;

    gboolean toggle = (data != NULL) ? TRUE : FALSE;

    enabled = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.auto_reconnect));

    if (toggle)
    {
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.auto_reconnect), !enabled);
        return TRUE;
    }

    g_info("Auto reconnect event occurred %d %d ", toggle, enabled);

    ui_auto_reconnect = enabled;

    if (changed)
    {
        /* Set the tool tip text */
        if (enabled)
        {
            gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(ui_main_data.auto_reconnect), "Disable automatic reconnection");
        }
        else
        {
            gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(ui_main_data.auto_reconnect), "Enable automatic reconnection");
        }
    }

    return TRUE;
}

void ui_callback_dialogbox_connect_destroy(void)
{
    if (dialogbox_connect != NULL)
    {
        gtk_widget_destroy (dialogbox_connect);
        dialogbox_connect = NULL;
        g_message("Connect dialogbox destroyed");
    }
}

gboolean ui_callback_on_connect(GtkWidget *widget, gpointer data)
{
    /* We have to retrieve the ip address and ui_port of remote host */
    gboolean start = (data != NULL) ? TRUE : FALSE;
    int pipe_fd[2];

    if (start)
    {
        ui_port = atoi (gtk_entry_get_text (GTK_ENTRY(ui_main_data.port_entry)));
        ui_ip = gtk_entry_get_text (GTK_ENTRY(ui_main_data.ip_entry));
    }

    g_message("Connect event occurred to %s:%d %s", ui_ip, ui_port, start ? "START" : "RETRY");

    if (start)
    {
        if (strlen (ui_ip) == 0)
        {
            ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "Connect", "Empty host ip address");
            return FALSE;
        }

        if (ui_port == 0)
        {
            ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "Connect", "Invalid host ui_port value");
            return FALSE;
        }
    }

    if (operation_running == FALSE)
    {
        operation_running = TRUE;

        ui_pipe_new (pipe_fd, ui_pipe_callback, NULL);

        memcpy (ui_main_data.pipe_fd, pipe_fd, sizeof(int) * 2);

        /* Disable the connect button */
        ui_disable_connect_button ();

        ui_set_title ("connecting to %s:%d ...", ui_ip, ui_port);
        {
            const static char *message_formats[] =
            {
                "Connecting to %s:%d ...",
                "Connection lost!\n\n" "Trying to reconnect to %s:%d ..."
            };
            gint response;
            gint x, y;
            gint w, h;

            if (socket_connect_to_remote_host (ui_ip, ui_port, pipe_fd[1]) != 0)
            {
                ui_enable_connect_button ();
                operation_running = FALSE;
                return FALSE;
            }

            /* Create dialog box for connect message:
             * - non modal mode does not seems to work, don't set the parent window to allow some interactions with it!
             */
            dialogbox_connect = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER,
                                                GTK_BUTTONS_CANCEL, message_formats[start ? 0 : 1], ui_ip, ui_port);
            gtk_window_set_title (GTK_WINDOW(dialogbox_connect), "Connect");

            /* Set the window at center of main window manually as there is no parent defined */
            gtk_window_get_position (GTK_WINDOW(ui_main_data.window), &x, &y);
            gtk_window_get_size (GTK_WINDOW(ui_main_data.window), &w, &h);
            g_message("Main window position: %d,%d dimension: %d,%d", x, y, w, h);
            x += w / 2;
            y += h / 2;
            g_message("Connect window position %d,%d", x, y);
            gtk_window_set_gravity (GTK_WINDOW(dialogbox_connect), GDK_GRAVITY_CENTER);
            gtk_window_move (GTK_WINDOW(dialogbox_connect), x, y);

            response = gtk_dialog_run (GTK_DIALOG (dialogbox_connect));
            g_message("Connect dialog response %s (%d)", gtk_get_respose_string(response), response);

            if (response == GTK_RESPONSE_NONE)
            {
                /* Dialogbox has been destroyed when program is exited, do nothing */
                return (TRUE);
            }
            if (response == GTK_RESPONSE_OK)
            {
                /* Connection is established */
                ui_set_sensitive_save_message_buttons (FALSE);
                ui_callback_signal_clear_list (NULL, NULL);
                ui_set_title ("%s:%d", ui_ip, ui_port);
            }
            else
            {
                /* Connection can not be established */
                if (response == GTK_RESPONSE_REJECT)
                {
                    /* Connection retry time-out */
                    ui_notification_dialog (GTK_MESSAGE_WARNING, FALSE, "Connect", "Failed to connect to provided host/ip address");
                }

                /* Re-enable connect button */
                ui_enable_connect_button();
                operation_running = FALSE;
            }
            ui_callback_dialogbox_connect_destroy ();
        }
    }

    return TRUE;
}

gboolean ui_callback_on_disconnect(GtkWidget *widget, gpointer data)
{
    g_message("Disconnect event occurred");

    ui_pipe_write_message (ui_main_data.pipe_fd[0], UI_PIPE_DISCONNECT_EVT, NULL, 0);

    if (dialogbox_connect != NULL)
    {
        gtk_dialog_response(GTK_DIALOG (dialogbox_connect), GTK_RESPONSE_CLOSE);
    }
    else
    {
        ui_enable_connect_button ();
        operation_running = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_signal_go_to_first(GtkWidget *widget, gpointer data)
{
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.signals_go_to_last_button), FALSE);
    ui_tree_view_select_row (0);

    return TRUE;
}

gboolean ui_callback_signal_go_to(GtkWidget *widget, gpointer data)
{
    gtk_window_set_focus (GTK_WINDOW(ui_main_data.window), ui_main_data.signals_go_to_entry);
    return TRUE;
}

gboolean ui_callback_signal_go_to_entry(GtkWidget *widget, gpointer data)
{
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.signals_go_to_last_button), FALSE);
    // gtk_entry_buffer_set_text(GTK_ENTRY(ui_main_data.signals_go_to_entry), "");
    gtk_window_set_focus (GTK_WINDOW(ui_main_data.window), ui_main_data.messages_list);
    return TRUE;
}

gboolean ui_callback_signal_go_to_last(GtkWidget *widget, gpointer data)
{
    gboolean enable = (data != NULL) ? TRUE : FALSE;
    gboolean enabled;

    if (enable)
    {
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.signals_go_to_last_button), TRUE);
        return TRUE;
    }

    enabled = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.signals_go_to_last_button));

    g_info("Button signal go to last event occurred %d %d", enable, enabled);

    if (enabled)
    {
        ui_main_data.follow_last = TRUE;
        ui_tree_view_select_row (ui_tree_view_get_filtered_number () - 1);
    }
    else
    {
        ui_main_data.follow_last = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_display_message_header(GtkWidget *widget, gpointer data)
{
    ui_main_data.display_message_header = !ui_main_data.display_message_header;
    // TODO refresh textview.
    return TRUE;
}

gboolean ui_callback_display_brace(GtkWidget *widget, gpointer data)
{
    ui_main_data.display_brace = !ui_main_data.display_brace;
    // TODO refresh textview.
    return TRUE;
}

gboolean ui_callback_signal_clear_list(GtkWidget *widget, gpointer data)
{
    /* Disable buttons to move in the list of signals */
    ui_set_sensitive_move_buttons (FALSE);
    ui_set_title ("");

    /* Clear list of signals */
    ui_tree_view_destroy_list (ui_main_data.messages_list);

    if (ui_main_data.text_view != NULL)
    {
        ui_signal_dissect_clear_view (ui_main_data.text_view);
    }

    ui_notebook_terminal_clear();

    return TRUE;
}

static void ui_callback_on_menu_items_selected(GtkWidget *widget, gpointer data)
{
    gboolean active = data != NULL;

    if (GTK_IS_CHECK_MENU_ITEM(widget))
    {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(widget), active);
    }
}

gboolean ui_callback_on_menu_none(GtkWidget *widget, gpointer data)
{
    GtkWidget *menu = (GtkWidget *) data;

    g_info("ui_callback_on_menu_none occurred %lx %lx)", (long) widget, (long) data);

    refresh_message_list = FALSE;
    gtk_container_foreach (GTK_CONTAINER(menu), ui_callback_on_menu_items_selected, (gpointer) FALSE);
    refresh_message_list = TRUE;

    if (filters_changed)
    {
        ui_tree_view_refilter ();
        filters_changed = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_on_menu_all(GtkWidget *widget, gpointer data)
{
    GtkWidget *menu = (GtkWidget *) data;

    g_info("ui_callback_on_menu_all occurred %lx %lx)", (long) widget, (long) data);

    refresh_message_list = FALSE;
    gtk_container_foreach (GTK_CONTAINER(menu), ui_callback_on_menu_items_selected, (gpointer) TRUE);
    refresh_message_list = TRUE;

    if (filters_changed)
    {
        ui_tree_view_refilter ();
        filters_changed = FALSE;
    }

    return TRUE;
}

gboolean ui_callback_on_menu_item_selected(GtkWidget *widget, gpointer data)
{
    ui_filter_item_t *filter_entry = data;
    gboolean enabled;

    enabled = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(widget));
    if (filter_entry->enabled != enabled)
    {
        filter_entry->enabled = enabled;
        if (refresh_message_list)
        {
            ui_tree_view_refilter ();
        }
        else
        {
            filters_changed = TRUE;
        }
    }
    g_info("ui_callback_on_menu_item_selected occurred %p %p %s %d (%d messages to display)", widget, data, filter_entry->name, enabled, ui_tree_view_get_filtered_number());

    return TRUE;
}

gboolean ui_callback_on_tree_column_header_click(GtkWidget *widget, gpointer data)
{
    col_type_t col = (col_type_t) data;

    g_info("ui_callback_on_tree_column_header_click %d", col);
    switch (col)
    {
        case COL_MESSAGE:
            ui_show_filter_menu (&ui_main_data.menu_filter_messages, &ui_filters.messages);
            break;

        case COL_FROM_TASK:
            ui_show_filter_menu (&ui_main_data.menu_filter_origin_tasks, &ui_filters.origin_tasks);
            break;

        case COL_TO_TASK:
            ui_show_filter_menu (&ui_main_data.menu_filter_destination_tasks, &ui_filters.destination_tasks);
            break;

        case COL_INSTANCE:
            ui_show_filter_menu (&ui_main_data.menu_filter_instances, &ui_filters.instances);
            break;

        default:
            g_warning("Unknown column filter %d in call to ui_callback_on_tree_column_header_click", col);
            return FALSE;
    }

    return TRUE;
}
