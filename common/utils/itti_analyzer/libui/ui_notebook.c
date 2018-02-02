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
#define G_LOG_DOMAIN ("UI")

#include <gtk/gtk.h>

#include "rc.h"

#include "ui_notebook.h"
#include "ui_tree_view.h"
#include "ui_signal_dissect_view.h"

static ui_text_view_t *terminal_view;

void ui_notebook_terminal_clear(void)
{
     ui_signal_dissect_clear_view(terminal_view);
}

void ui_notebook_terminal_append_data(gchar *text, gint length)
{
    ui_signal_set_text(terminal_view, text, length);
}

int ui_notebook_create(GtkWidget *vbox)
{
    GtkWidget *notebook;
    GtkWidget *vbox_notebook, *vbox_terminal;

    if (!vbox)
        return RC_BAD_PARAM;

    notebook = gtk_notebook_new();

    vbox_notebook = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    ui_tree_view_create(NULL, vbox_notebook);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox_notebook, NULL);
    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(notebook), vbox_notebook, "Messages list");

#if defined (FILTERS_TAB)
    vbox_notebook = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox_notebook, NULL);
    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(notebook), vbox_notebook, "Filters");
#endif

    vbox_notebook = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox_notebook, NULL);
    gtk_notebook_set_tab_label_text (GTK_NOTEBOOK(notebook), vbox_notebook, "Terminal");

    vbox_terminal = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

    terminal_view = ui_signal_dissect_new(vbox_terminal);

    gtk_box_pack_start (GTK_BOX(vbox_notebook), vbox_terminal, TRUE, TRUE, 5);

    /* Add the notebook to the vbox of the main window */
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    return RC_OK;
}
