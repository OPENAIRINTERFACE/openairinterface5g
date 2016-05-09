#ifndef _VIEW_H_
#define _VIEW_H_

#include "gui/gui.h"

/* defines the public API of views */

typedef struct view {
  void (*clear)(struct view *this);
  void (*append)(struct view *this, ...);
  void (*set)(struct view *this, char *name, ...);
} view;

view *new_view_stdout(void);
view *new_view_textlist(int maxsize, float refresh_rate, gui *g, widget *w);
view *new_view_xy(int length, float refresh_rate, gui *g, widget *w,
    int color);

#endif /* _VIEW_H_ */
