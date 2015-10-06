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
