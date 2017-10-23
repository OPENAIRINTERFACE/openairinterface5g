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

#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#define G_LOG_DOMAIN ("UI")

#include <sys/stat.h>

#include <gtk/gtk.h>

#include "logs.h"
#include "itti_types.h"
#include "rc.h"

#include "ui_interface.h"
#include "ui_main_screen.h"
#include "ui_menu_bar.h"
#include "ui_notifications.h"
#include "ui_notif_dlg.h"
#include "ui_callbacks.h"
#include "ui_filters.h"
#include "ui_tree_view.h"

#include "locate_root.h"
#include "xml_parse.h"
#include "socket.h"

static const itti_message_types_t itti_dump_xml_definition_end =  ITTI_DUMP_XML_DEFINITION_END;
static const itti_message_types_t itti_dump_message_type_end =    ITTI_DUMP_MESSAGE_TYPE_END;

static FILE *messages_file;
static uint32_t message_number;
static gboolean ui_abort;

int ui_disable_connect_button(void)
{
    /* Disable Connect button and enable disconnect button */
    gtk_widget_set_sensitive (GTK_WIDGET (ui_main_data.connect), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (ui_main_data.disconnect), TRUE);
    socket_abort_connection = FALSE;

    return RC_OK;
}

int ui_enable_connect_button(void)
{
    /* Disable Disconnect button and enable connect button */
    gtk_widget_set_sensitive (GTK_WIDGET (ui_main_data.connect), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (ui_main_data.disconnect), FALSE);
    socket_abort_connection = TRUE;
    ui_set_sensitive_save_message_buttons (TRUE);
    ui_set_title ("");

    return RC_OK;
}

static void ui_change_cursor(gboolean busy)
{
    static GdkWindow *window;

    if (busy)
    {
        GdkDisplay *display;
        GdkCursor *cursor;
        // gint x, y;

        cursor = gdk_cursor_new (GDK_WATCH);
        display = gdk_display_get_default ();
        window = gtk_widget_get_window (GTK_WIDGET(ui_main_data.window));
        // window = gdk_display_get_window_at_pointer(display, &x, &y);

        gdk_window_set_cursor (window, cursor);
        gdk_display_sync (display);
        g_object_unref (cursor);
        // gtk_widget_set_sensitive (ui_main_data.window, FALSE);
        ui_gtk_flush_events ();
    }
    else
    {
        gdk_window_set_cursor (window, NULL);
        // gtk_widget_set_sensitive (ui_main_data.window, TRUE);
        ui_gtk_flush_events ();
    }
}

static void gtk_filter_add(GtkWidget *file_chooser, const gchar *title, const gchar *pattern)
{
    GtkFileFilter *file_filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (file_filter, title);
    gtk_file_filter_add_pattern (file_filter, pattern);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_chooser), file_filter);
}

int ui_messages_read(char *file_name)
{
    int result = RC_OK;
    int source;
    int read_data = 0;
    void *input_data = NULL;
    size_t input_data_length = 0;
    int read_messages = 0;

    ui_change_cursor (TRUE);

    source = open (file_name, O_RDONLY);
    if (source < 0)
    {
        ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "open messages", "Failed to open file \"%s\": %s", file_name,
                                g_strerror (errno));
        result = RC_FAIL;
    }
    else
    {
        itti_socket_header_t message_header;
        struct stat st;
        int size;
        double read_fraction = 0.f;

        if (stat (file_name, &st) < 0)
        {
            ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "get file length",
                                    "Failed to retrieve length for file \"%s\": %s", file_name, g_strerror (errno));
            result = RC_FAIL;
        }
        size = st.st_size;

        ui_callback_signal_clear_list (NULL, NULL);

        /* Initialize the progress bar */
        ui_abort = FALSE;
        ui_progress_bar_set_fraction (0);

        do
        {
            read_data = read (source, &message_header, sizeof(itti_socket_header_t));

            if (read_data == -1)
            {
                ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "open messages", "Failed to read from file \"%s\": %s",
                                        file_name, g_strerror (errno));
                result = RC_FAIL;
                break;
            }

            if (read_data == 0)
            {
                break;
            }

            if (read_data < sizeof(itti_socket_header_t))
            {
                if (ui_notification_dialog (GTK_MESSAGE_WARNING, TRUE, "open messages",
                                            "Failed to read a complete message header from file \"%s\": %s", file_name, g_strerror (errno)) == RC_FAIL)
                {
                    read_data = 0;
                }
            }
            else
            {
                read_fraction += (double) read_data / size;

                input_data_length = message_header.message_size - sizeof(itti_socket_header_t);

                g_debug("%x, %x, %zd", message_header.message_type, message_header.message_size, input_data_length);

                /* Checking for non-header part */
                if (input_data_length > 0)
                {
                    input_data = malloc (input_data_length);

                    read_data = read (source, input_data, input_data_length);
                    if (read_data < input_data_length)
                    {
                        if (ui_notification_dialog (GTK_MESSAGE_WARNING, TRUE, "open messages",
                                                    "Failed to read a complete message from file \"%s\": %s", file_name, g_strerror (errno)) == RC_FAIL)
                        {
                            read_data = 0;
                        }
                        break;
                    }

                    read_fraction += (double) input_data_length / size;
                }

                switch (message_header.message_type)
                {
                    case ITTI_DUMP_XML_DEFINITION:
                        ui_gtk_flush_events ();
                        if (memcmp (&(((char *) input_data)[input_data_length - sizeof (itti_message_types_t)]),
                            &itti_dump_xml_definition_end, sizeof (itti_message_types_t)) == 0)
                        {
                            result = xml_parse_buffer (input_data, input_data_length - sizeof (itti_message_types_t));
                            if (result != RC_OK)
                            {
                                ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "open messages",
                                                        "Error in parsing XML definitions in file \"%s\": %s", file_name,
                                                        rc_strings[-result]);
                                read_data = 0;
                            }
                            ui_gtk_flush_events ();
                            g_message("Parsed XML definition from file \"%s\"", file_name);
                        }
                        else
                        {
                            ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "open messages",
                                                    "Error in parsing XML definitions in file \"%s\", end mark is missing", file_name);
                        }
                        /* Data input buffer is kept in case user when to save the log file later */
                        break;

                    case ITTI_DUMP_MESSAGE_TYPE:
                    {
                        itti_signal_header_t *itti_signal_header = input_data;
                        buffer_t *buffer;

                        if (memcmp (&(((char *) input_data)[input_data_length - sizeof (itti_message_types_t)]),
                            &itti_dump_message_type_end, sizeof (itti_message_types_t)) == 0)
                        {
                            /* Create the new buffer */
                            if (buffer_new_from_data (&buffer, input_data + sizeof(itti_signal_header_t),
                                                      input_data_length - sizeof(itti_signal_header_t) - sizeof(itti_message_types_t), 0) != RC_OK)
                            {
                                g_error("Failed to create new buffer");
                                g_assert_not_reached ();
                            }

                            sscanf (itti_signal_header->message_number_char, MESSAGE_NUMBER_CHAR_FORMAT, &buffer->message_number);

                            ui_signal_add_to_list (buffer, ((read_messages % 1000) == 0) ? (gpointer) 1 : NULL);

                            if ((read_messages % 100) == 0)
                            {
                                ui_progress_bar_set_fraction (read_fraction);
                                ui_gtk_flush_events ();
                            }

                            read_messages++;
                        }
                        else
                        {
                            if (ui_notification_dialog (GTK_MESSAGE_WARNING, TRUE, "open messages",
                                                        "Failed to read a message from file \"%s\", end mark is missing", file_name) == RC_FAIL)
                            {
                                read_data = 0;
                            }
                            break;
                        }

                        free (input_data);
                        break;
                    }

                    case ITTI_STATISTIC_MESSAGE_TYPE:
                    default:
                        if (ui_notification_dialog (GTK_MESSAGE_WARNING, TRUE, "open messages",
                                                    "Unknown (or not implemented) record type: %d in file \"%s\"",
                                                    message_header.message_type, file_name) == RC_FAIL)
                        {
                            read_data = 0;
                        }

                        free (input_data);
                        break;
                }
            }
        } while ((ui_abort == FALSE) && (read_data > 0));

        if (read_messages > 0)
        {
            char *basename;

            /* Enable buttons to move in the list of signals */
            ui_set_sensitive_move_buttons (TRUE);

            if (ui_main_data.follow_last)
            {
                /* Advance to the last signal */
                ui_tree_view_select_row (ui_tree_view_get_filtered_number () - 1);
            }

            basename = g_path_get_basename (file_name);
            ui_set_title ("\"%s\"", basename);
        }
        else
        {
            result = RC_FAIL;
        }

        ui_progress_bar_terminate ();

        g_message("Read %d messages (%d to display) from file \"%s\"\n", read_messages, ui_tree_view_get_filtered_number(), file_name);

        close (source);
    }

    ui_change_cursor (FALSE);

    return result;
}

static void ui_message_write_callback(const gpointer buffer, const gchar *signal_name)
{
    buffer_t *signal_buffer = (buffer_t *) buffer;
    itti_socket_header_t message_header;
    itti_signal_header_t itti_signal_header;
    uint32_t message_size;

    message_size = signal_buffer->size_bytes;

    message_header.message_size = sizeof(itti_socket_header_t) + sizeof(itti_signal_header) + message_size + sizeof(itti_message_types_t);
    message_header.message_type = ITTI_DUMP_MESSAGE_TYPE;

    sprintf(itti_signal_header.message_number_char, MESSAGE_NUMBER_CHAR_FORMAT, message_number);
    itti_signal_header.message_number_char[sizeof(itti_signal_header.message_number_char) - 1] = '\n';
    message_number++;

    fwrite (&message_header, sizeof(message_header), 1, messages_file);
    fwrite (&itti_signal_header, sizeof(itti_signal_header), 1, messages_file);
    fwrite (signal_buffer->data, message_size, 1, messages_file);
    fwrite (&itti_dump_message_type_end, sizeof(itti_message_types_t), 1, messages_file);
}

static int ui_messages_file_write(char *file_name, gboolean filtered)
{
    if (file_name == NULL)
    {
        g_warning("No name for log file");
        return RC_FAIL;
    }

    messages_file = fopen (file_name, "w");
    if (messages_file == NULL)
    {
        ui_notification_dialog (GTK_MESSAGE_ERROR, FALSE, "Failed to open file \"%s\": %s", file_name, g_strerror (errno));
        return RC_FAIL;
    }

    /* Write XML definitions */
    {
        itti_socket_header_t message_header;

        message_header.message_size = sizeof(itti_socket_header_t) + xml_raw_data_size + sizeof(itti_message_types_t);
        message_header.message_type = ITTI_DUMP_XML_DEFINITION;

        fwrite (&message_header, sizeof(message_header), 1, messages_file);
        fwrite (xml_raw_data, xml_raw_data_size, 1, messages_file);
        fwrite (&itti_dump_xml_definition_end, sizeof(itti_message_types_t), 1, messages_file);
    }

    /* Write messages */
    {
        message_number = 1;
        ui_tree_view_foreach_message (ui_message_write_callback, filtered);
    }

    fclose (messages_file);

    return RC_OK;
}

int ui_messages_open_file_chooser(void)
{
    int result = RC_OK;

    GtkWidget *filechooser;
    gboolean response_accept;
    char *filename;

    filechooser = gtk_file_chooser_dialog_new ("Select file", GTK_WINDOW (ui_main_data.window),
                                               GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                               GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_filter_add (filechooser, "Log files", "*.log");
    gtk_filter_add (filechooser, "All files", "*");

    /* Process the response */
    response_accept = gtk_dialog_run (GTK_DIALOG (filechooser)) == GTK_RESPONSE_ACCEPT;

    if (response_accept)
    {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
    }
    gtk_widget_destroy (filechooser);
    if (response_accept)
    {
        ui_set_sensitive_save_message_buttons (FALSE);

        result = ui_messages_read (filename);
        if (result == RC_OK)
        {
            /* Update messages file name for future use */
            if (ui_main_data.messages_file_name != NULL)
            {
                g_free (ui_main_data.messages_file_name);
            }
            ui_main_data.messages_file_name = filename;

            ui_set_sensitive_save_message_buttons (TRUE);
        }
        else
        {
            g_free (filename);
        }
    }
    return result;
}

int ui_messages_save_file_chooser(gboolean filtered)
{
    int result = RC_OK;
    GtkWidget *filechooser;

    /* Check if there is something to save */
    if (xml_raw_data_size > 0)
    {
        static const char *title[] =
        {
             "Save file (all messages)",
             "Save file (filtered messages)",
        };

        filechooser = gtk_file_chooser_dialog_new (title[filtered], GTK_WINDOW (ui_main_data.window),
                                                   GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
                                                   GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
        gtk_filter_add (filechooser, "Log files", "*.log");
        gtk_filter_add (filechooser, "All files", "*");

        gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (filechooser), TRUE);

        if (ui_main_data.messages_file_name != NULL)
        {
            gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser), ui_main_data.messages_file_name);
        }
        else
        {
            gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (filechooser), "messages.log");
        }

        /* Process the response */
        if (gtk_dialog_run (GTK_DIALOG (filechooser)) == GTK_RESPONSE_ACCEPT)
        {
            char *filename;

            filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
            result = ui_messages_file_write (filename, filtered);
            if (result == RC_OK)
            {
                /* Update filters file name for future use */
                if (ui_main_data.messages_file_name != NULL)
                {
                    g_free (ui_main_data.messages_file_name);
                }
                ui_main_data.messages_file_name = filename;
            }
            else
            {
                g_free (filename);
            }
        }
        gtk_widget_destroy (filechooser);
    }

    return result;
}

int ui_filters_open_file_chooser(void)
{
    int result = RC_OK;
    GtkWidget *filechooser;
    gboolean response_accept;
    char *filename;

    filechooser = gtk_file_chooser_dialog_new ("Select file", GTK_WINDOW (ui_main_data.window),
                                               GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                               GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_filter_add (filechooser, "Filters files", "*.xml");
    gtk_filter_add (filechooser, "All files", "*");

    /* Process the response */
    response_accept = gtk_dialog_run (GTK_DIALOG (filechooser)) == GTK_RESPONSE_ACCEPT;

    if (response_accept)
    {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
    }
    gtk_widget_destroy (filechooser);
    if (response_accept)
    {
        result = ui_filters_read (filename);
        if (result == RC_OK)
        {
            /* Update filters file name for future use */
            if (ui_main_data.filters_file_name != NULL)
            {
                g_free (ui_main_data.filters_file_name);
            }
            ui_main_data.filters_file_name = filename;
        }
        else
        {
            g_free (filename);
        }
    }

    return result;
}

int ui_filters_save_file_chooser(void)
{
    int result = RC_OK;
    GtkWidget *filechooser;

    filechooser = gtk_file_chooser_dialog_new ("Save file", GTK_WINDOW (ui_main_data.window),
                                               GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                               GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    gtk_filter_add (filechooser, "Filters files", "*.xml");
    gtk_filter_add (filechooser, "All files", "*");

    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (filechooser), TRUE);

    if (ui_main_data.filters_file_name != NULL)
    {
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser), ui_main_data.filters_file_name);
    }
    else
    {
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (filechooser), "filters.xml");
    }

    /* Process the response */
    if (gtk_dialog_run (GTK_DIALOG (filechooser)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename;

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));
        result = ui_filters_file_write (filename);
        if (result == RC_OK)
        {
            /* Update filters file name for future use */
            if (ui_main_data.filters_file_name != NULL)
            {
                g_free (ui_main_data.filters_file_name);
            }
            ui_main_data.filters_file_name = filename;
        }
        else
        {
            g_free (filename);
        }
    }
    gtk_widget_destroy (filechooser);

    return result;
}

void ui_progressbar_window_destroy (void)
{
    ui_abort = TRUE;
    ui_progress_bar_terminate();
}

int ui_progress_bar_set_fraction(double fraction)
{
    if (ui_abort == FALSE)
    {
        /* If not exist instantiate */
        if (!ui_main_data.progressbar && !ui_main_data.progressbar_window)
        {
            ui_main_data.progressbar_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
            /* Keep progress bar window on top of main window */
            gtk_window_set_transient_for (GTK_WINDOW(ui_main_data.progressbar_window), GTK_WINDOW(ui_main_data.window));

            /* Set the window at center of main window */
            gtk_window_set_position (GTK_WINDOW(ui_main_data.progressbar_window), GTK_WIN_POS_CENTER_ON_PARENT);
            gtk_window_set_title (GTK_WINDOW(ui_main_data.progressbar_window), "Processing");

            gtk_container_set_border_width (GTK_CONTAINER (ui_main_data.progressbar_window), 10);

            ui_main_data.progressbar = gtk_progress_bar_new ();

            gtk_container_add (GTK_CONTAINER (ui_main_data.progressbar_window), ui_main_data.progressbar);

            /* Assign the destroy event */
            g_signal_connect(ui_main_data.progressbar_window, "destroy", ui_progressbar_window_destroy, NULL);

            gtk_widget_show_all (ui_main_data.progressbar_window);

            gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.stop_loading), TRUE);
        }

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(ui_main_data.progressbar), fraction);

//       ui_gtk_flush_events();
    }

    return RC_OK;
}

int ui_progress_bar_terminate(void)
{
    if (ui_main_data.progressbar)
    {
        gtk_widget_destroy (ui_main_data.progressbar);
        ui_main_data.progressbar = NULL;
    }
    if (ui_main_data.progressbar_window)
    {
        gtk_widget_destroy (ui_main_data.progressbar_window);
        ui_main_data.progressbar_window = NULL;
    }
    gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.stop_loading), FALSE);

    return RC_OK;
}
