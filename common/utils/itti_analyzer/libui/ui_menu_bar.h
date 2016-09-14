#ifndef UI_MENU_BAR_H_
#define UI_MENU_BAR_H_

void ui_set_sensitive_move_buttons(gboolean enable);

void ui_set_sensitive_save_message_buttons(gboolean enable);

int ui_menu_bar_create(GtkWidget *vbox);

int ui_toolbar_create(GtkWidget *vbox);

#endif /* UI_MENU_BAR_H_ */
