#ifndef _GUI_H_
#define _GUI_H_

/* defines the public API of the GUI */

typedef void gui;
typedef void widget;

#define HORIZONTAL 0
#define VERTICAL   1

gui *gui_init(void);

/* position = -1 to put at the end */
void widget_add_child(gui *gui, widget *parent, widget *child, int position);

widget *new_toplevel_window(gui *gui, int width, int height, char *title);
widget *new_container(gui *gui, int vertical);
widget *new_label(gui *gui, const char *text);
widget *new_xy_plot(gui *gui, int width, int height, char *label,
    int vruler_width);
widget *new_text_list(gui *_gui, int width, int nlines, int background_color);

void xy_plot_set_range(gui *gui, widget *this,
    float xmin, float xmax, float ymin, float ymax);

void text_list_add(gui *gui, widget *this, const char *text, int position);

void gui_loop(gui *gui);

void glock(gui *gui);
void gunlock(gui *gui);

int new_color(gui *gui, char *color);

#endif /* _GUI_H_ */
