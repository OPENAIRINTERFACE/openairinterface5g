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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define G_LOG_DOMAIN ("UI")

#include <gtk/gtk.h>

#include "logs.h"
#include "rc.h"

#include "ui_callbacks.h"
#include "ui_interface.h"
#include "ui_main_screen.h"
#include "ui_menu_bar.h"
#include "ui_tree_view.h"
#include "ui_notebook.h"
#include "ui_notifications.h"
#include "ui_filters.h"

ui_main_data_t ui_main_data;

static void ui_help(void)
{
    printf ("Usage: itti_analyser [options]\n\n"
            "Options:\n"
            "  -d DISSECT   write DISSECT file with message types parse details\n"
            "  -f FILTERS   read filters from FILTERS file\n"
            "  -h           display this help and exit\n"
            "  -i IP        set ip address to IP\n"
            "  -l LEVEL     set log level to LEVEL in the range of 2 to 7\n"
            "  -m MESSAGES  read messages from MESSAGES file\n"
            "  -p PORT      set port to PORT\n");
}

void ui_gtk_parse_arg(int argc, char *argv[])
{
    char c;

    /* Clear of ui_main_data not needed */
    // memset (&ui_main_data, 0, sizeof(ui_main_data_t));

    /* Set some default initialization value for the IP address */
    ui_main_data.ip_entry_init = "127.0.0.1";
    ui_main_data.port_entry_init = "10006";

    /* Set default log level to at least warning level messages */
    ui_main_data.log_flags = (G_LOG_LEVEL_MASK & (~(G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)));

    while ((c = getopt (argc, argv, "d:f:hi:l:m:p:")) != -1)
    {
        switch (c)
        {
            case 'd':
                ui_main_data.dissect_file_name = malloc (strlen (optarg) + 1);
                if (ui_main_data.dissect_file_name != NULL)
                {
                    strcpy (ui_main_data.dissect_file_name, optarg);
                }
                break;

            case 'f':
                ui_main_data.filters_file_name = malloc (strlen (optarg) + 1);
                if (ui_main_data.filters_file_name != NULL)
                {
                    strcpy (ui_main_data.filters_file_name, optarg);
                }
                break;

            case 'h':
                ui_help ();
                exit (0);
                break;

            case 'i':
                ui_main_data.ip_entry_init = optarg;
                break;

            case 'l':
            {
                GLogLevelFlags log_flag;

                log_flag = 1 << atoi(optarg);
                if (log_flag < G_LOG_LEVEL_ERROR)
                {
                    log_flag = G_LOG_LEVEL_ERROR;
                }
                else
                {
                    if (log_flag > G_LOG_LEVEL_DEBUG)
                    {
                        log_flag = G_LOG_LEVEL_DEBUG;
                    }
                }
                ui_main_data.log_flags = ((log_flag << 1) - 1) & G_LOG_LEVEL_MASK;
                break;
            }

            case 'm':
                ui_main_data.messages_file_name = malloc (strlen (optarg) + 1);
                if (ui_main_data.messages_file_name != NULL)
                {
                    strcpy (ui_main_data.messages_file_name, optarg);
                }
                break;

            case 'p':
                ui_main_data.port_entry_init = optarg;
                break;

            default:
                ui_help ();
                exit (-1);
                break;
        }
    }
}

static int ui_idle_callback(gpointer data)
{
    g_info("Entering idle state");

    gtk_window_set_focus (GTK_WINDOW(ui_main_data.window), ui_main_data.messages_list);

    /* Read filters file */
    if (ui_main_data.filters_file_name != NULL)
    {
        ui_filters_read(ui_main_data.filters_file_name);
    }

    /* Read messages file */
    if (ui_main_data.messages_file_name != NULL)
    {
        ui_messages_read (ui_main_data.messages_file_name);
    }

    /* One shot execution */
    return FALSE;
}

void ui_set_title(const char *fmt, ...)
{
    va_list args;
    char *name;
    char buffer[200];
    char title[220];

#if defined(PACKAGE_STRING)
    name = PACKAGE_NAME;
#else
    name = "itti_analyzer";
#endif

    va_start(args, fmt);

    vsnprintf (buffer, sizeof(buffer), fmt, args);
    snprintf (title, sizeof(title), "%s  %s", name, buffer);
    va_end (args);

    gtk_window_set_title (GTK_WINDOW(ui_main_data.window), title);
}

void ui_main_window_destroy (void)
{
    ui_callback_dialogbox_connect_destroy();
    ui_progressbar_window_destroy();
    gtk_main_quit();
}

int ui_gtk_initialize(int argc, char *argv[])
{
    GtkWidget *vbox;

    /* Create the main window */
    ui_main_data.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    ui_init_filters (TRUE, FALSE);

    // gtk_window_set_default_icon_from_file ("../analyzer.png", NULL);
    gtk_window_set_default_icon_name (GTK_STOCK_FIND);

    gtk_window_set_position (GTK_WINDOW(ui_main_data.window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW(ui_main_data.window), 1024, 800);
    ui_set_title("");
    gtk_window_set_resizable (GTK_WINDOW(ui_main_data.window), TRUE);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    CHECK_FCT(ui_menu_bar_create(vbox));
    CHECK_FCT(ui_toolbar_create(vbox));
    CHECK_FCT(ui_notebook_create(vbox));

    gtk_container_add (GTK_CONTAINER(ui_main_data.window), vbox);

    /* Assign the destroy event */
    g_signal_connect(ui_main_data.window, "destroy", ui_main_window_destroy, NULL);

    /* Show the application window */
    gtk_widget_show_all (ui_main_data.window);

    g_idle_add (ui_idle_callback, NULL);

    return RC_OK;
}

void ui_gtk_flush_events(void)
{
    while (gtk_events_pending ())
    {
        gtk_main_iteration();
    }
}
