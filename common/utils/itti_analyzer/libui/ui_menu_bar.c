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

#define G_LOG_DOMAIN ("UI")

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "rc.h"
#include "ui_main_screen.h"
#include "ui_menu_bar.h"
#include "ui_callbacks.h"
#include "ui_notifications.h"

static const guint BUTTON_SPACE = 0;
static const guint LABEL_SPACE = 5;
static const guint SEPARATOR_SPACE = 5;

void ui_set_sensitive_move_buttons(gboolean enable)
{
    gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.signals_go_to_last_button), enable);
    gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.signals_go_to_first_button), enable);
}

void ui_set_sensitive_save_message_buttons(gboolean enable)
{
    if (ui_main_data.nb_message_received == 0)
    {
        enable = FALSE;
    }
    gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.save_replay_file), enable);
    gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.save_replay_file_filtered), enable);
}

int ui_menu_bar_create(GtkWidget *vbox)
{
    GtkAccelGroup *accel_group;
    GtkWidget *menu_bar;

    GtkWidget *system_menu;
    GtkWidget *system;
    GtkWidget *quit;

    GtkWidget *filters_menu;
    GtkWidget *filters;
    GtkWidget *open_filters;
    GtkWidget *reload_filters;
    GtkWidget *save_filters;
    GtkWidget *enable_filters;

    GtkWidget *messages_menu;
    GtkWidget *messages;
    GtkWidget *open_messages;
    GtkWidget *reload_messages;
    GtkWidget *stop_loading;
    GtkWidget *save_messages;
    GtkWidget *save_messages_filtered;
    GtkWidget *goto_first_messages;
    GtkWidget *goto_messages;
    GtkWidget *goto_last_messages;
    GtkWidget *display_message_header;
    GtkWidget *display_brace;

    GtkWidget *connect_menu;
    GtkWidget *connect;
    GtkWidget *auto_reconnect;
    GtkWidget *connect_to;
    GtkWidget *disconnect;

    GtkWidget *help_menu;
    GtkWidget *help;
    GtkWidget *about;

    if (!vbox)
        return RC_BAD_PARAM;

    accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(ui_main_data.window), accel_group);

    menu_bar = gtk_menu_bar_new();

    /* Create the System sub-menu */
    {
        system = gtk_menu_item_new_with_mnemonic ("_System");
        gtk_menu_shell_append (GTK_MENU_SHELL(menu_bar), system);

        system_menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(system), system_menu);

        /* Create the Filters menu items */
        {
            quit = gtk_menu_item_new_with_mnemonic ("_Quit");
            gtk_widget_add_accelerator (quit, "activate", accel_group, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(system_menu), quit);
            g_signal_connect(G_OBJECT(quit), "activate", ui_main_window_destroy, NULL);
        }
    }

    /* Create the Filters sub-menu */
    {
        filters = gtk_menu_item_new_with_mnemonic ("_Filters");
        gtk_menu_shell_append (GTK_MENU_SHELL(menu_bar), filters);

        filters_menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(filters), filters_menu);

        /* Create the Filters menu items */
        {
            open_filters = gtk_menu_item_new_with_mnemonic ("_Open filters file");
            gtk_widget_add_accelerator (open_filters, "activate", accel_group, GDK_KEY_p, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(filters_menu), open_filters);
            g_signal_connect(G_OBJECT(open_filters), "activate", G_CALLBACK(ui_callback_on_open_filters), (gpointer) FALSE);

            reload_filters = gtk_menu_item_new_with_mnemonic ("_Reload filters file");
            gtk_widget_add_accelerator (reload_filters, "activate", accel_group, GDK_KEY_d, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_widget_add_accelerator (reload_filters, "activate", accel_group, GDK_KEY_F4, 0, GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(filters_menu), reload_filters);
            g_signal_connect(G_OBJECT(reload_filters), "activate", G_CALLBACK(ui_callback_on_open_filters),
                             (gpointer) TRUE);

            save_filters = gtk_menu_item_new_with_mnemonic ("_Save filters file");
            gtk_widget_add_accelerator (save_filters, "activate", accel_group, GDK_KEY_v, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(filters_menu), save_filters);
            g_signal_connect(G_OBJECT(save_filters), "activate", G_CALLBACK(ui_callback_on_save_filters), NULL);

            gtk_menu_shell_append (GTK_MENU_SHELL(filters_menu), gtk_menu_item_new ()); // Separator

            enable_filters = gtk_menu_item_new_with_mnemonic ("_Enable filtering");
            gtk_widget_add_accelerator (enable_filters, "activate", accel_group, GDK_KEY_e, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(filters_menu), enable_filters);
            g_signal_connect(G_OBJECT(enable_filters), "activate", G_CALLBACK(ui_callback_on_enable_filters), NULL);
        }
    }

    /* Create the Messages sub-menu */
    {
        messages = gtk_menu_item_new_with_mnemonic ("_Messages");
        gtk_menu_shell_append (GTK_MENU_SHELL(menu_bar), messages);

        messages_menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(messages), messages_menu);

        /* Crate the Messages menu items */
        {
            open_messages = gtk_menu_item_new_with_mnemonic ("_Open messages file");
            gtk_widget_add_accelerator (open_messages, "activate", accel_group, GDK_KEY_o, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), open_messages);
            g_signal_connect(G_OBJECT(open_messages), "activate", G_CALLBACK(ui_callback_on_open_messages),
                             (gpointer) FALSE);

            reload_messages = gtk_menu_item_new_with_mnemonic ("_Reload messages file");
            gtk_widget_add_accelerator (reload_messages, "activate", accel_group, GDK_KEY_r, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_widget_add_accelerator (reload_messages, "activate", accel_group, GDK_KEY_F5, 0, GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), reload_messages);
            g_signal_connect(G_OBJECT(reload_messages), "activate", G_CALLBACK(ui_callback_on_open_messages),
                             (gpointer) TRUE);

            stop_loading = gtk_menu_item_new_with_mnemonic ("S_top loading messages file");
            gtk_widget_add_accelerator (stop_loading, "activate", accel_group, GDK_KEY_x, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), stop_loading);
            g_signal_connect(G_OBJECT(stop_loading), "activate", G_CALLBACK(ui_progressbar_window_destroy),
                             NULL);

            save_messages = gtk_menu_item_new_with_mnemonic ("S_ave messages file (all)");
            gtk_widget_add_accelerator (save_messages, "activate", accel_group, GDK_KEY_a, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), save_messages);
            g_signal_connect(G_OBJECT(save_messages), "activate", G_CALLBACK(ui_callback_on_save_messages),
                             (gpointer) FALSE);

            save_messages_filtered = gtk_menu_item_new_with_mnemonic ("_Save messages file (filtered)");
            gtk_widget_add_accelerator (save_messages_filtered, "activate", accel_group, GDK_KEY_s, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), save_messages_filtered);
            g_signal_connect(G_OBJECT(save_messages_filtered), "activate", G_CALLBACK(ui_callback_on_save_messages),
                             (gpointer) TRUE);

            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), gtk_menu_item_new ()); // Separator

            goto_first_messages = gtk_menu_item_new_with_mnemonic ("Go to _first message");
            gtk_widget_add_accelerator (goto_first_messages, "activate", accel_group, GDK_KEY_f, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), goto_first_messages);
            g_signal_connect(G_OBJECT(goto_first_messages), "activate", G_CALLBACK(ui_callback_signal_go_to_first),
                             NULL);

            goto_messages = gtk_menu_item_new_with_mnemonic ("_Go to message ...");
            gtk_widget_add_accelerator (goto_messages, "activate", accel_group, GDK_KEY_g, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), goto_messages);
            g_signal_connect(G_OBJECT(goto_messages), "activate", G_CALLBACK(ui_callback_signal_go_to), NULL);

            goto_last_messages = gtk_menu_item_new_with_mnemonic ("Go to _last message");
            gtk_widget_add_accelerator (goto_last_messages, "activate", accel_group, GDK_KEY_l, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), goto_last_messages);
            g_signal_connect(G_OBJECT(goto_last_messages), "activate", G_CALLBACK(ui_callback_signal_go_to_last), (gpointer) TRUE);

            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), gtk_menu_item_new ()); // Separator

            display_message_header = gtk_menu_item_new_with_mnemonic ("Display message _header");
            gtk_widget_add_accelerator (display_message_header, "activate", accel_group, GDK_KEY_h, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), display_message_header);
            g_signal_connect(G_OBJECT(display_message_header), "activate",
                             G_CALLBACK(ui_callback_display_message_header), NULL);

            display_brace = gtk_menu_item_new_with_mnemonic ("Display _brace");
            gtk_widget_add_accelerator (display_brace, "activate", accel_group, GDK_KEY_b, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(messages_menu), display_brace);
            g_signal_connect(G_OBJECT(display_brace), "activate", G_CALLBACK(ui_callback_display_brace), NULL);
        }
    }

    /* Create the Connect sub-menu */
    {
        connect = gtk_menu_item_new_with_mnemonic ("_Connect");
        gtk_menu_shell_append (GTK_MENU_SHELL(menu_bar), connect);

        connect_menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(connect), connect_menu);

        /* Create the Help menu item */
        {
            auto_reconnect = gtk_menu_item_new_with_mnemonic ("_Automatic reconnection");
            gtk_widget_add_accelerator (auto_reconnect, "activate", accel_group, GDK_KEY_t, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(connect_menu), auto_reconnect);
            g_signal_connect(G_OBJECT(auto_reconnect), "activate", G_CALLBACK(ui_callback_on_auto_reconnect), (gpointer) TRUE);

            connect_to = gtk_menu_item_new_with_mnemonic ("_Connect");
            gtk_widget_add_accelerator (connect_to, "activate", accel_group, GDK_KEY_c, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(connect_menu), connect_to);
            g_signal_connect(G_OBJECT(connect_to), "activate", G_CALLBACK(ui_callback_on_connect), (gpointer) TRUE);

            disconnect = gtk_menu_item_new_with_mnemonic ("_Disconnect");
            gtk_widget_add_accelerator (disconnect, "activate", accel_group, GDK_KEY_u, GDK_CONTROL_MASK,
                                        GTK_ACCEL_VISIBLE);
            gtk_menu_shell_append (GTK_MENU_SHELL(connect_menu), disconnect);
            g_signal_connect(G_OBJECT(disconnect), "activate", G_CALLBACK(ui_callback_on_disconnect), NULL);
        }
    }

    /* Create the Help sub-menu */
    {
        help = gtk_menu_item_new_with_mnemonic ("_Help");
        gtk_menu_shell_append (GTK_MENU_SHELL(menu_bar), help);

        help_menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(help), help_menu);

        /* Create the Help menu item */
        {
            about = gtk_menu_item_new_with_mnemonic ("_About");
            gtk_menu_shell_append (GTK_MENU_SHELL(help_menu), about);
            g_signal_connect(G_OBJECT(about), "activate", G_CALLBACK(ui_callback_on_about), NULL);
        }
    }

    /* Add the menubar to the vbox */
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 3);

    return RC_OK;
}

int ui_toolbar_create(GtkWidget *vbox)
{
    GtkWidget *hbox;
    GtkWidget *messages_label;
    GtkWidget *ip_label;
    GtkWidget *port_label;

    if (!vbox)
        return RC_BAD_PARAM;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    messages_label = gtk_label_new("Messages");

    /* Button to Enable filtering */
    {
        ui_main_data.filters_enabled = gtk_toggle_tool_button_new();
        gtk_tool_button_set_label (GTK_TOOL_BUTTON(ui_main_data.filters_enabled), "Filters");
        g_signal_connect(G_OBJECT(ui_main_data.filters_enabled), "clicked",
                         G_CALLBACK(ui_callback_on_filters_enabled), NULL);
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.filters_enabled), TRUE);
    }

    /* Button to open filters file */
    {
        ui_main_data.open_filters_file = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.open_filters_file), "Open filters file");

        g_signal_connect(G_OBJECT(ui_main_data.open_filters_file), "clicked",
                        G_CALLBACK(ui_callback_on_open_filters), (gpointer) FALSE);
    }

    /* Button to refresh filters file */
    {
        ui_main_data.refresh_filters_file = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.refresh_filters_file), "Reload filters file");

        g_signal_connect(G_OBJECT(ui_main_data.refresh_filters_file), "clicked",
                        G_CALLBACK(ui_callback_on_open_filters), (gpointer) TRUE);
    }

    /* Button to save filters file */
    {
        ui_main_data.save_filters_file = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.save_filters_file), "Save filters file");

        g_signal_connect(G_OBJECT(ui_main_data.save_filters_file), "clicked",
                        G_CALLBACK(ui_callback_on_save_filters), NULL);
    }

    /* Button to open messages file */
    {
        ui_main_data.open_replay_file = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.open_replay_file), "Open messages file");

        g_signal_connect(G_OBJECT(ui_main_data.open_replay_file), "clicked",
                        G_CALLBACK(ui_callback_on_open_messages), (gpointer) FALSE);
    }

    /* Button to refresh messages file */
    {
        ui_main_data.refresh_replay_file = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.refresh_replay_file), "Reload messages file");

        g_signal_connect(G_OBJECT(ui_main_data.refresh_replay_file), "clicked",
                        G_CALLBACK(ui_callback_on_open_messages), (gpointer) TRUE);
    }

    /* Stop reading messages file */
    {
        ui_main_data.stop_loading = gtk_tool_button_new_from_stock(GTK_STOCK_STOP);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.stop_loading), "Stop loading messages file");

        g_signal_connect(G_OBJECT(ui_main_data.stop_loading), "clicked",
                        ui_progressbar_window_destroy, NULL);
    }

    /* Button to save messages file */
    {
        ui_main_data.save_replay_file = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.save_replay_file), "Save messages file (all)");

        g_signal_connect(G_OBJECT(ui_main_data.save_replay_file), "clicked",
                        G_CALLBACK(ui_callback_on_save_messages), (gpointer) FALSE);
    }

    /* Button to save messages file */
    {
        ui_main_data.save_replay_file_filtered = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE_AS);
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.save_replay_file_filtered), "Save messages file (filtered)");

        g_signal_connect(G_OBJECT(ui_main_data.save_replay_file_filtered), "clicked",
                        G_CALLBACK(ui_callback_on_save_messages), (gpointer) TRUE);
    }

    /* Button to go to first signal in list */
    {
        ui_main_data.signals_go_to_first_button = gtk_tool_button_new_from_stock(GTK_STOCK_GOTO_FIRST);
        /* Set the tooltip text */
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.signals_go_to_first_button), "Goto first signal");

        gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.signals_go_to_first_button), FALSE);

        g_signal_connect(G_OBJECT(ui_main_data.signals_go_to_first_button), "clicked",
                        G_CALLBACK(ui_callback_signal_go_to_first), NULL);
    }

    /* Entry to go to given signal number */
    {
        ui_main_data.signals_go_to_entry = gtk_entry_new ();
        gtk_entry_set_width_chars (GTK_ENTRY(ui_main_data.signals_go_to_entry), 10);
        // gtk_entry_set_input_purpose (GTK_ENTRY(ui_main_data.signals_go_to_entry), GTK_INPUT_PURPOSE_DIGITS);

        g_signal_connect(G_OBJECT(ui_main_data.signals_go_to_entry), "activate",
                        G_CALLBACK(ui_callback_signal_go_to_entry), NULL);
    }

    /* Button to go to last signal in list */
    {
        ui_main_data.signals_go_to_last_button = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_GOTO_LAST);
        /* Set the tooltip text */
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.signals_go_to_last_button), "Goto last signal");

        gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.signals_go_to_last_button), FALSE);

        g_signal_connect(G_OBJECT(ui_main_data.signals_go_to_last_button), "clicked",
                        G_CALLBACK(ui_callback_signal_go_to_last), (gpointer) FALSE);
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.signals_go_to_last_button), TRUE);
    }

    /* Button to automatically try to reconnect */
    {
        ui_main_data.auto_reconnect = gtk_toggle_tool_button_new();
        gtk_tool_button_set_label (GTK_TOOL_BUTTON(ui_main_data.auto_reconnect), "Auto");
        g_signal_connect(G_OBJECT(ui_main_data.auto_reconnect), "clicked",
                         G_CALLBACK(ui_callback_on_auto_reconnect), (gpointer) FALSE);
        gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON(ui_main_data.auto_reconnect), FALSE);
    }

    /* Button to connect to remote */
    {
        ui_main_data.connect = gtk_tool_button_new_from_stock(GTK_STOCK_CONNECT);
        /* Set the tooltip text */
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.connect), "Connect to remote host");

        g_signal_connect(G_OBJECT(ui_main_data.connect), "clicked",
                        G_CALLBACK(ui_callback_on_connect), (gpointer) TRUE);
    }

    /* Button to disconnect from remote */
    {
        ui_main_data.disconnect = gtk_tool_button_new_from_stock(GTK_STOCK_DISCONNECT);
        /* Set the tooltip text */
        gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(ui_main_data.disconnect), "Disconnect from remote host");

        /* Disabled at startup. Will be activated when a connection is established */
        gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.disconnect), FALSE);

        g_signal_connect(G_OBJECT(ui_main_data.disconnect), "clicked",
                        G_CALLBACK(ui_callback_on_disconnect), NULL);
    }

    ip_label = gtk_label_new("ip:");
    port_label = gtk_label_new("port:");

    ui_main_data.ip_entry = gtk_entry_new();
    /* Width of 15 characters for port number (ipv4 address) */
    gtk_entry_set_width_chars(GTK_ENTRY(ui_main_data.ip_entry), 15);
    gtk_entry_set_text(GTK_ENTRY(ui_main_data.ip_entry), ui_main_data.ip_entry_init);

    ui_main_data.port_entry = gtk_entry_new();
    /* Width of 5 characters for port number (uint16_t) */
    gtk_entry_set_width_chars(GTK_ENTRY(ui_main_data.port_entry), 5);
    gtk_entry_set_text(GTK_ENTRY(ui_main_data.port_entry), ui_main_data.port_entry_init);

    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.filters_enabled), FALSE, FALSE, LABEL_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.open_filters_file), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.refresh_filters_file), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.save_filters_file), FALSE, FALSE, BUTTON_SPACE);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, SEPARATOR_SPACE);

    gtk_box_pack_start(GTK_BOX(hbox), messages_label, FALSE, FALSE, LABEL_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.open_replay_file), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.refresh_replay_file), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.stop_loading), FALSE, FALSE, BUTTON_SPACE);
    gtk_widget_set_sensitive(GTK_WIDGET(ui_main_data.stop_loading), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.save_replay_file), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.save_replay_file_filtered), FALSE, FALSE, BUTTON_SPACE);
    ui_set_sensitive_save_message_buttons(FALSE);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, SEPARATOR_SPACE);

    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.signals_go_to_first_button), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.signals_go_to_entry), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.signals_go_to_last_button), FALSE, FALSE, BUTTON_SPACE);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, SEPARATOR_SPACE);

    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.auto_reconnect), FALSE, FALSE, LABEL_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.connect), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ui_main_data.disconnect), FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), ip_label, FALSE, FALSE, LABEL_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), ui_main_data.ip_entry, FALSE, FALSE, BUTTON_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), port_label, FALSE, FALSE, LABEL_SPACE);
    gtk_box_pack_start(GTK_BOX(hbox), ui_main_data.port_entry, FALSE, FALSE, BUTTON_SPACE);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    return RC_OK;
}
