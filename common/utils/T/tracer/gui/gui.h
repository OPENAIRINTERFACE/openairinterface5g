#ifndef _GUI_H_
#define _GUI_H_

/* defines the public API of the GUI */

typedef void gui;
typedef void widget;

#define HORIZONTAL 0
#define VERTICAL   1

#define BACKGROUND_COLOR 0
#define FOREGROUND_COLOR 1

#define DEFAULT_FONT 0

/* tic type for XY plot */
#define XY_PLOT_DEFAULT_TICK 0
#define XY_PLOT_SCROLL_TICK  1

/* key modifiers */
#define KEY_SHIFT   (1<<0)
#define KEY_CONTROL (1<<1)
#define KEY_ALT     (1<<2)

gui *gui_init(void);

/* position = -1 to put at the end */
void widget_add_child(gui *gui, widget *parent, widget *child, int position);
void widget_del_child(gui *gui, widget *parent, widget *child);
void widget_dirty(gui *gui, widget *this);

widget *new_toplevel_window(gui *gui, int width, int height, char *title);
widget *new_container(gui *gui, int vertical);
widget *new_positioner(gui *gui);
widget *new_label(gui *gui, const char *text);
widget *new_textarea(gui *gui, int width, int height, int maxsize);
widget *new_xy_plot(gui *gui, int width, int height, char *label,
    int vruler_width);
widget *new_textlist(gui *gui, int width, int nlines, int background_color);
widget *new_timeline(gui *gui, int width, int number_of_sublines,
    int subline_height);
widget *new_space(gui *gui, int width, int height);
widget *new_image(gui *gui, unsigned char *data, int length);

void label_set_clickable(gui *gui, widget *label, int clickable);
void label_set_text(gui *gui, widget *label, char *text);

void textarea_set_text(gui *gui, widget *textarea, char *text);

void container_set_child_growable(gui *_gui, widget *_this,
    widget *child, int growable);

int xy_plot_new_plot(gui *gui, widget *this, int color);
void xy_plot_set_range(gui *gui, widget *this,
    float xmin, float xmax, float ymin, float ymax);
void xy_plot_set_points(gui *gui, widget *this,
    int plot, int npoints, float *x, float *y);
void xy_plot_get_dimensions(gui *gui, widget *this, int *width, int *height);
void xy_plot_set_title(gui *gui, widget *this, char *label);
void xy_plot_set_tick_type(gui *gui, widget *this, int type);

void textlist_add(gui *gui, widget *this, const char *text, int position,
    int color);
void textlist_del(gui *gui, widget *this, int position);
void textlist_add_silent(gui *gui, widget *this, const char *text,
    int position, int color);
void textlist_del_silent(gui *gui, widget *this, int position);
void textlist_state(gui *_gui, widget *_this,
    int *visible_lines, int *start_line, int *number_of_lines);
void textlist_set_start_line(gui *gui, widget *this, int line);
void textlist_get_line(gui *gui, widget *this, int line,
    char **text, int *color);
void textlist_set_color(gui *gui, widget *this, int line, int color);

void timeline_clear(gui *gui, widget *this);
void timeline_clear_silent(gui *gui, widget *this);
void timeline_add_points(gui *gui, widget *this, int subline, int color,
    int *x, int len);
void timeline_add_points_silent(gui *gui, widget *this, int subline,
    int color, int *x, int len);
void timeline_set_subline_background_color(gui *gui, widget *this,
    int subline, int color);
void timeline_get_width(gui *gui, widget *this, int *width);

void gui_loop(gui *gui);

void glock(gui *gui);
void gunlock(gui *gui);

int new_color(gui *gui, char *color);

/* notifications */
/* known notifications:
 * - textlist:
 *      - scrollup   { int: key_modifiers }
 *      - scrolldown { int: key_modifiers }
 *      - click      { int [2]: line, button }
 * - label:
 *      - click      { int: button } (if enabled)
 * - timeline
 *      - resize     { int: width }
 *      - scrollup   { int [3]: x, y, key_modifiers }
 *      - scrolldown { int [3]: x, y, key_modifiers }
 *      - click      { int: button }
 */

/* same type as in gui_defs.h */
typedef void (*notifier)(void *private, gui *g,
    char *notification, widget *w, void *notification_data);
unsigned long register_notifier(gui *g, char *notification, widget *w,
    notifier handler, void *private);
void unregister_notifier(gui *g, unsigned long notifier_id);

#endif /* _GUI_H_ */
