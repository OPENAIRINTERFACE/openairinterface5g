#ifndef _VIEW_H_
#define _VIEW_H_

#include "gui/gui.h"

/* defines the public API of views */

typedef struct view {
  void (*clear)(struct view *this);
  void (*append)(struct view *this, ...);
} view;

view *new_stdout(void);
view *new_textlist(int maxsize, float refresh_rate, gui *g, widget *w);

#endif /* _VIEW_H_ */
