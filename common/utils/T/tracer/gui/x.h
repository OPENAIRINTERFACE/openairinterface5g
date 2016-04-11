#ifndef _X_H_
#define _X_H_

/* public X interface */

#define BACKGROUND_COLOR 0
#define FOREGROUND_COLOR 1

typedef void x_connection;
typedef void x_window;

x_connection *x_open(void);

x_window *x_create_window(x_connection *x, int width, int height,
    char *title);

int x_connection_fd(x_connection *x);

void x_flush(x_connection *x);

int x_new_color(x_connection *x, char *color);

/* for x_events, we pass the gui */
#include "gui.h"
void x_events(gui *gui);

void x_text_get_dimensions(x_connection *, const char *t,
                           int *width, int *height, int *baseline);

/* drawing functions */

void x_draw_line(x_connection *c, x_window *w, int color,
    int x1, int y1, int x2, int y2);

void x_draw_rectangle(x_connection *c, x_window *w, int color,
    int x, int y, int width, int height);

void x_fill_rectangle(x_connection *c, x_window *w, int color,
    int x, int y, int width, int height);

void x_draw_string(x_connection *_c, x_window *_w, int color,
    int x, int y, const char *t);

/* this function copies the pixmap to the window */
void x_draw(x_connection *c, x_window *w);

#endif /* _X_H_ */
