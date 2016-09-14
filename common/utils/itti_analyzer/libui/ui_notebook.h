#ifndef UI_NOTEBOOK_H_
#define UI_NOTEBOOK_H_

void ui_notebook_terminal_clear(void);

int ui_notebook_create(GtkWidget *vbox);

void ui_notebook_terminal_append_data(gchar *text, gint length);

#endif /* UI_NOTEBOOK_H_ */
