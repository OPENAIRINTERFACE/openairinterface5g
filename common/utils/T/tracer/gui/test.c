#include "gui.h"

int main(void)
{
  gui *g;
  widget *w, *c1, *c2, *l, *plot, *tl;
  int tlcol;

  g = gui_init();

  c1 = new_container(g, VERTICAL);
  c2 = new_container(g, HORIZONTAL);

  l = new_label(g, "this is a good label");
  widget_add_child(g, c2, l, 0);
  l = new_label(g, "this is another good label");
  widget_add_child(g, c2, l, -1);

  l = new_label(g, "OH! WHAT A LABEL!");
  widget_add_child(g, c1, l, -1);

  widget_add_child(g, c1, c2, 0);

  plot = new_xy_plot(g, 100, 100, "xy plot test", 30);
#if 0
  c2 = new_container(g, HORIZONTAL);
  widget_add_child(g, c2, plot, -1);
  widget_add_child(g, c1, c2, -1);
#else
  widget_add_child(g, c1, plot, -1);
#endif

  tlcol = new_color(g, "#ddf");
  tl = new_textlist(g, 300, 10, tlcol);
  widget_add_child(g, c1, tl, -1);

  textlist_add(g, tl, "hello", -1, FOREGROUND_COLOR);
  textlist_add(g, tl, "world", -1, FOREGROUND_COLOR);

  w = new_toplevel_window(g, 500, 400, "test window");
  widget_add_child(g, w, c1, 0);

  gui_loop(g);

  return 0;
}
