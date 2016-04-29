#ifndef _GUI_H_
#define _GUI_H_

/* defines the public API of the GUI */

typedef void gui;
typedef void widget;

#define HORIZONTAL 0
#define VERTICAL   1

#define BACKGROUND_COLOR 0
#define FOREGROUND_COLOR 1

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
void xy_plot_set_points(gui *gui, widget *this,
    int npoints, float *x, float *y);

void text_list_add(gui *gui, widget *this, const char *text, int position,
    int color);
void text_list_del(gui *gui, widget *this, int position);
void text_list_state(gui *_gui, widget *_this,
    int *visible_lines, int *start_line, int *number_of_lines);
void text_list_set_start_line(gui *gui, widget *this, int line);
void text_list_get_line(gui *gui, widget *this, int line,
    char **text, int *color);
void text_list_set_color(gui *gui, widget *this, int line, int color);

void gui_loop(gui *gui);

void glock(gui *gui);
void gunlock(gui *gui);

int new_color(gui *gui, char *color);

/* notifications */
/* known notifications:
 * - text_list:
 *      - scrollup   { void *: NULL }
 *      - scrolldown { void *: NULL }
 *      - click      { int [2]: line, button }
 */

/* same type as in gui_defs.h */
typedef void (*notifier)(void *private, gui *g,
    char *notification, widget *w, void *notification_data);
unsigned long register_notifier(gui *g, char *notification, widget *w,
    notifier handler, void *private);
void unregister_notifier(gui *g, unsigned long notifier_id);

#endif /* _GUI_H_ */
