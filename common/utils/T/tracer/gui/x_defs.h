#ifndef _X_DEFS_H_
#define _X_DEFS_H_

#include <X11/Xlib.h>

struct x_connection {
  Display *d;
  GC *colors;
  int ncolors;
};

struct x_window {
  Window w;
  Pixmap p;
  int width;
  int height;
  /* below: internal data used for X events handling */
  int redraw;
  int repaint;
  int resize, new_width, new_height;
};

#endif /* _X_DEFS_H_ */
