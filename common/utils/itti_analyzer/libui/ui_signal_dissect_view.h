#ifndef UI_SIGNAL_DISSECT_VIEW_H_
#define UI_SIGNAL_DISSECT_VIEW_H_

typedef struct {
    GtkWidget            *text_view;
    PangoFontDescription *text_view_font;
} ui_text_view_t;

ui_text_view_t *ui_signal_dissect_new(GtkWidget *hbox);

int ui_signal_dissect_clear_view(ui_text_view_t *text_view);

gboolean ui_signal_set_text(gpointer user_data, gchar *text, gint length);

#endif /*UI_SIGNAL_DISSECT_VIEW_H_ */
