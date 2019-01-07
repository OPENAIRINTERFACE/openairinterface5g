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
