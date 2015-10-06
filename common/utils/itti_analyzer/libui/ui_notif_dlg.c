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


#define G_LOG_DOMAIN ("UI")

#include "rc.h"

#include "ui_notif_dlg.h"
#include "ui_main_screen.h"

static const char * const gtk_response_strings[] =
    {"GTK_RESPONSE_NONE", "GTK_RESPONSE_REJECT", "GTK_RESPONSE_ACCEPT", "GTK_RESPONSE_DELETE_EVENT", "GTK_RESPONSE_OK",
     "GTK_RESPONSE_CANCEL", "GTK_RESPONSE_CLOSE", "GTK_RESPONSE_YES", "GTK_RESPONSE_NO", "GTK_RESPONSE_APPLY", "GTK_RESPONSE_HELP"};

static const char * const title_type_strings[] =
    {"Info", "Warning", "Question", "Error", "Other"};

const char * gtk_get_respose_string (gint response)
{
    gint response_index = - response - 1;

    if ((0 <= response_index) && (response_index < (sizeof (gtk_response_strings) / sizeof (gtk_response_strings[0]))))
    {
        return (gtk_response_strings[response_index]);
    }
    else
    {
        return ("Invalid response value!");
    }
}
int ui_notification_dialog(GtkMessageType type, gboolean cancel, const char *title, const char *fmt, ...)
{
    va_list     args;
    GtkWidget  *dialogbox;
    char        buffer[200];
    int         result =  RC_OK;

    va_start(args, fmt);

    vsnprintf (buffer, sizeof(buffer), fmt, args);

    g_warning("%s", buffer);

    dialogbox = gtk_message_dialog_new (GTK_WINDOW(ui_main_data.window), GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL, type,
                                        cancel ? GTK_BUTTONS_OK_CANCEL : GTK_BUTTONS_OK, "%s",
                                        buffer);
    /* Set the window at center of main window */
    gtk_window_set_position (GTK_WINDOW(dialogbox), GTK_WIN_POS_CENTER_ON_PARENT);

    gtk_dialog_set_default_response (GTK_DIALOG(dialogbox), GTK_RESPONSE_OK);

    snprintf (buffer, sizeof(buffer), "%s: %s", title_type_strings[type], title);
    gtk_window_set_title (GTK_WINDOW(dialogbox), buffer);

    if (gtk_dialog_run (GTK_DIALOG (dialogbox)) == GTK_RESPONSE_CANCEL)
    {
        result = RC_FAIL;
    }

    gtk_widget_destroy (dialogbox);

    va_end(args);

    return result;
}
