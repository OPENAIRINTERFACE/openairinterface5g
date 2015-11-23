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


#include <string.h>

#define G_LOG_DOMAIN ("UI")

#include <gtk/gtk.h>

#include "rc.h"

#include "ui_main_screen.h"
#include "ui_menu_bar.h"
#include "ui_signal_dissect_view.h"

ui_text_view_t *ui_signal_dissect_new(GtkWidget *hbox)
{
    GtkWidget *scrolled_window;
    ui_text_view_t *new_text_view;

    new_text_view = malloc(sizeof(ui_text_view_t));

    new_text_view->text_view = gtk_text_view_new();
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    /* Change the font of the widget */
    new_text_view->text_view_font = pango_font_description_new();

    g_debug("Default family is: %s\n", pango_font_description_get_family(new_text_view->text_view_font));

    pango_font_description_set_family(new_text_view->text_view_font, "Monospace");
    pango_font_description_set_absolute_size(new_text_view->text_view_font, 10 * PANGO_SCALE);

    g_debug("Font family has been set to: %s\n", pango_font_description_get_family(new_text_view->text_view_font));

    gtk_widget_override_font(new_text_view->text_view, new_text_view->text_view_font);

    /* Disable editable attribute */
    gtk_text_view_set_editable(GTK_TEXT_VIEW(new_text_view->text_view), FALSE);

    gtk_container_add(GTK_CONTAINER(scrolled_window), new_text_view->text_view);

    gtk_paned_pack2 (GTK_PANED (hbox), scrolled_window, TRUE, FALSE);

    return new_text_view;
}

int ui_signal_dissect_clear_view(ui_text_view_t *text_view)
{
    GtkTextBuffer *text_buffer;

    g_assert(text_view != NULL);

    if (text_view->text_view != NULL)
    {
        /* Create an empty text buffer */
        text_buffer = gtk_text_buffer_new(NULL);
        gtk_text_buffer_set_text(text_buffer, "", 0);

        gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view->text_view), text_buffer);
    }

    return RC_OK;
}

gboolean ui_signal_set_text(gpointer user_data, gchar *text, gint length)
{
    GtkTextBuffer  *text_buffer;
    ui_text_view_t *text_view;

    if (length < 0)
        return FALSE;

    text_view = (ui_text_view_t *)user_data;

    g_assert(text != NULL);
    g_assert(text_view != NULL);
    g_assert(text_view->text_view != NULL);

    g_assert(GTK_IS_TEXT_VIEW(GTK_TEXT_VIEW(text_view->text_view)));

    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view->text_view));

    if (text_buffer) {
        /* We already have a text buffer, use it */
        gtk_text_buffer_insert_at_cursor(text_buffer, text, length);
    } else {
        /* No buffer currently in use, create a new one */
        text_buffer = gtk_text_buffer_new(NULL);
        gtk_text_buffer_set_text(text_buffer, text, length);
        gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view->text_view),
                                 text_buffer);
    }

    return TRUE;
}
