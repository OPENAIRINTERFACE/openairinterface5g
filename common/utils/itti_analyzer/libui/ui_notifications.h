#ifndef UI_NOTIFICATIONS_H_
#define UI_NOTIFICATIONS_H_

typedef void (*message_write_callback_t)  (const gpointer buffer, const gchar *signal_name);

int ui_disable_connect_button(void);

int ui_enable_connect_button(void);

int ui_messages_read(char *filename);

int ui_messages_open_file_chooser(void);

int ui_messages_save_file_chooser(gboolean filtered);

int ui_filters_open_file_chooser(void);

int ui_filters_save_file_chooser(void);

void ui_progressbar_window_destroy(void);

int ui_progress_bar_set_fraction(double fraction);

int ui_progress_bar_terminate(void);

#endif /* UI_NOTIFICATIONS_H_ */
