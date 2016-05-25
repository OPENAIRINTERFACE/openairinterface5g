#ifndef _X_H_
#define _X_H_

/* public X interface */

typedef void x_connection;
typedef void x_window;
typedef void x_image;

x_connection *x_open(void);

x_window *x_create_window(x_connection *x, int width, int height,
    char *title);

x_image *x_create_image(x_connection *x, unsigned char *data,
    int width, int height);

int x_connection_fd(x_connection *x);

void x_flush(x_connection *x);

int x_new_color(x_connection *x, char *color);
int x_new_font(x_connection *x, char *font);

/* for x_events, we pass the gui */
#include "gui.h"
void x_events(gui *gui);

void x_text_get_dimensions(x_connection *, int font, const char *t,
                           int *width, int *height, int *baseline);

/* drawing functions */

void x_draw_line(x_connection *c, x_window *w, int color,
    int x1, int y1, int x2, int y2);

void x_draw_rectangle(x_connection *c, x_window *w, int color,
    int x, int y, int width, int height);

void x_fill_rectangle(x_connection *c, x_window *w, int color,
    int x, int y, int width, int height);

void x_draw_string(x_connection *_c, x_window *_w, int font, int color,
    int x, int y, const char *t);

void x_draw_clipped_string(x_connection *_c, x_window *_w, int font,
    int color, int x, int y, const char *t,
    int clipx, int clipy, int clipwidth, int clipheight);

void x_draw_image(x_connection *c, x_window *w, x_image *img, int x, int y);

/* specials functions to plot many points
 * you call several times x_add_point() then x_plot_points()
 */
void x_add_point(x_connection *c, int x, int y);
void x_plot_points(x_connection *c, x_window *w, int color);

/* this function copies the pixmap to the window */
void x_draw(x_connection *c, x_window *w);

#endif /* _X_H_ */
