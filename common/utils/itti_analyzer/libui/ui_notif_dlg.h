#include <gtk/gtk.h>

#ifndef UI_NOTIF_DLG_H_
#define UI_NOTIF_DLG_H_

const char *gtk_get_respose_string (gint response);

extern int ui_notification_dialog(GtkMessageType type, gboolean cancel, const char *title, const char *fmt, ...);

#endif /* UI_NOTIF_DLG_H_ */
