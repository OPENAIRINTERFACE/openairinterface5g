#ifndef _X_DEFS_H_
#define _X_DEFS_H_

#include <X11/Xlib.h>
#include <Xft.h>

struct x_connection {
  Display *d;
  GC *colors;
  XftColor *xft_colors;
  int ncolors;
  XPoint *pts;
  int pts_size;
  int pts_maxsize;
  XftFont **fonts;
  int nfonts;
};

struct x_window {
  Window w;
  Pixmap p;
  int width;
  int height;
  XftDraw *xft;
  /* below: internal data used for X events handling */
  int redraw;
  int repaint;
  int resize, new_width, new_height;
};

struct x_image {
  Pixmap p;
  int width;
  int height;
};

#endif /* _X_DEFS_H_ */
