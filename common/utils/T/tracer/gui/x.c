#include "x.h"
#include "x_defs.h"
#include "gui_defs.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int x_connection_fd(x_connection *_x)
{
  struct x_connection *x = _x;
  return ConnectionNumber(x->d);
}

static GC create_gc(Display *d, char *color)
{
  GC ret = XCreateGC(d, DefaultRootWindow(d), 0, NULL);
  XGCValues gcv;
  XColor rcol, scol;

  XCopyGC(d, DefaultGC(d, DefaultScreen(d)), -1L, ret);
  if (XAllocNamedColor(d, DefaultColormap(d, DefaultScreen(d)),
                      color, &scol, &rcol)) {
    gcv.foreground = scol.pixel;
    XChangeGC(d, ret, GCForeground, &gcv);
  } else ERR("X: could not allocate color '%s'\n", color);

  return ret;
}

int x_new_color(x_connection *_x, char *color)
{
  struct x_connection *x = _x;
  x->ncolors++;
  x->colors = realloc(x->colors, x->ncolors * sizeof(GC));
  if (x->colors == NULL) OOM;
  x->colors[x->ncolors-1] = create_gc(x->d, color);
  return x->ncolors - 1;
}

x_connection *x_open(void)
{
  struct x_connection *ret;

  ret = calloc(1, sizeof(struct x_connection));
  if (ret == NULL) OOM;

  ret->d = XOpenDisplay(0);
printf("XOpenDisplay display %p return x_connection %p\n", ret->d, ret);
  if (ret->d == NULL) ERR("error calling XOpenDisplay: no X? you root?\n");

  x_new_color(ret, "white");    /* background color */
  x_new_color(ret, "black");    /* foreground color */

  return ret;
}

x_window *x_create_window(x_connection *_x, int width, int height,
    char *title)
{
  struct x_connection *x = _x;
  struct x_window *ret;

  ret = calloc(1, sizeof(struct x_window));
  if (ret == NULL) OOM;

  ret->w = XCreateSimpleWindow(x->d, DefaultRootWindow(x->d), 0, 0,
      width, height, 0, WhitePixel(x->d, DefaultScreen(x->d)),
      WhitePixel(x->d, DefaultScreen(x->d)));
  ret->width = width;
  ret->height = height;

  XStoreName(x->d, ret->w, title);

  ret->p = XCreatePixmap(x->d, ret->w, width, height,
      DefaultDepth(x->d, DefaultScreen(x->d)));
  XFillRectangle(x->d, ret->p, x->colors[BACKGROUND_COLOR],
      0, 0, width, height);

  /* enable backing store */
  {
    XSetWindowAttributes att;
    att.backing_store = Always;
    XChangeWindowAttributes(x->d, ret->w, CWBackingStore, &att);
  }

  XSelectInput(x->d, ret->w,
      KeyPressMask      |
      ButtonPressMask   |
      ButtonReleaseMask |
      PointerMotionMask |
      ExposureMask      |
      StructureNotifyMask);

  XMapWindow(x->d, ret->w);

#if 0
  /* wait for window to be mapped */
printf("wait for map\n");
  while (1) {
    XEvent ev;
    //XWindowEvent(x->d, ret->w, StructureNotifyMask, &ev);
    XWindowEvent(x->d, ret->w, ExposureMask, &ev);
printf("got ev %d\n", ev.type);
    //if (ev.type == MapNotify) break;
    if (ev.type == Expose) break;
  }
printf("XXX create connection %p window %p (win id %d pixmap %d) w h %d %d\n", x, ret, (int)ret->w, (int)ret->p, width, height);
#endif

  return ret;
}

static struct toplevel_window_widget *find_x_window(struct gui *g, Window id)
{
  struct widget_list *cur;
  struct toplevel_window_widget *w;
  struct x_window *xw;
  cur = g->toplevel;
  while (cur) {
    w = (struct toplevel_window_widget *)cur->item;
    xw = w->x;
    if (xw->w == id) return w;
    cur = cur->next;
  }
  return NULL;
}

void x_events(gui *_gui)
{
  struct gui *g = _gui;
  struct widget_list *cur;
  struct x_connection *x = g->x;
  struct toplevel_window_widget *w;

printf("x_events START\n");
  /* preprocessing (to "compress" events) */
  cur = g->toplevel;
  while (cur) {
    struct x_window *xw;
    w = (struct toplevel_window_widget *)cur->item;
    xw = w->x;
    xw->redraw = 0;
    xw->repaint = 0;
    xw->resize = 0;
    cur = cur->next;
  }

  while (XPending(x->d)) {
    XEvent ev;
    XNextEvent(x->d, &ev);
printf("XEV %d\n", ev.type);
    switch (ev.type) {
    case MapNotify:
    case Expose:
      if ((w = find_x_window(g, ev.xexpose.window)) != NULL) {
        struct x_window *xw = w->x;
        xw->redraw = 1;
      }
      break;
    case ConfigureNotify:
      if ((w = find_x_window(g, ev.xconfigure.window)) != NULL) {
        struct x_window *xw = w->x;
        xw->resize = 1;
        xw->new_width = ev.xconfigure.width;
        xw->new_height = ev.xconfigure.height;
        if (xw->new_width < 10) xw->new_width = 10;
        if (xw->new_height < 10) xw->new_height = 10;
printf("ConfigureNotify %d %d\n", ev.xconfigure.width, ev.xconfigure.height);
      }
      break;
    case ButtonPress:
      if ((w = find_x_window(g, ev.xbutton.window)) != NULL) {
        w->common.button(g, w, ev.xbutton.x, ev.xbutton.y,
            ev.xbutton.button, 0);
      }
      break;
    case ButtonRelease:
      if ((w = find_x_window(g, ev.xbutton.window)) != NULL) {
        w->common.button(g, w, ev.xbutton.x, ev.xbutton.y,
            ev.xbutton.button, 1);
      }
      break;
#if 0
    case MapNotify:
      if ((w = find_x_window(g, ev.xmap.window)) != NULL) {
        struct x_window *xw = w->x;
        xw->repaint = 1;
      }
      break;
#endif
    default: WARN("TODO: X event type %d\n", ev.type); break;
    }
  }

  /* postprocessing */
printf("post processing\n");
  cur = g->toplevel;
  while (cur) {
    struct toplevel_window_widget *w =
        (struct toplevel_window_widget *)cur->item;
    struct x_window *xw = w->x;
    if (xw->resize) {
printf("resize old %d %d new %d %d\n", xw->width, xw->height, xw->new_width, xw->new_height);
      if (xw->width != xw->new_width || xw->height != xw->new_height) {
        w->common.allocate(g, w, 0, 0, xw->new_width, xw->new_height);
        xw->width = xw->new_width;
        xw->height = xw->new_height;
        XFreePixmap(x->d, xw->p);
        xw->p = XCreatePixmap(x->d, xw->w, xw->width, xw->height,
            DefaultDepth(x->d, DefaultScreen(x->d)));
        XFillRectangle(x->d, xw->p, x->colors[BACKGROUND_COLOR],
            0, 0, xw->width, xw->height);
        //xw->repaint = 1;
      }
    }
    if (xw->repaint) {
      w->common.paint(g, w);
      xw->redraw = 1;
    }
    if (xw->redraw) {
      struct x_connection *x = g->x;
printf("XCopyArea w h %d %d\n", xw->width, xw->height);
      XCopyArea(x->d, xw->p, xw->w, x->colors[1],
          0, 0, xw->width, xw->height, 0, 0);
    }
    cur = cur->next;
  }
printf("x_events DONE\n");
}

void x_flush(x_connection *_x)
{
  struct x_connection *x = _x;
  XFlush(x->d);
}

void x_text_get_dimensions(x_connection *_c, const char *t,
    int *width, int *height, int *baseline)
{
  struct x_connection *c = _c;
  int dir;
  int ascent;
  int descent;
  XCharStruct overall;

  /* TODO: don't use XQueryTextExtents (X roundtrip) */
  XQueryTextExtents(c->d, XGContextFromGC(c->colors[1]), t, strlen(t),
      &dir, &ascent, &descent, &overall);

//printf("dir %d ascent %d descent %d lbearing %d rbearing %d width %d ascent %d descent %d\n", dir, ascent, descent, overall.lbearing, overall.rbearing, overall.width, overall.ascent, overall.descent);

  *width = overall.width;
  *height = ascent + descent;
  *baseline = ascent;
}

/***********************************************************************/
/*                    public drawing functions                         */
/***********************************************************************/

void x_draw_line(x_connection *_c, x_window *_w, int color,
    int x1, int y1, int x2, int y2)
{
  struct x_connection *c = _c;
  struct x_window *w = _w;
  XDrawLine(c->d, w->p, c->colors[color], x1, y1, x2, y2);
}

void x_draw_rectangle(x_connection *_c, x_window *_w, int color,
    int x, int y, int width, int height)
{
  struct x_connection *c = _c;
  struct x_window *w = _w;
  XDrawRectangle(c->d, w->p, c->colors[color], x, y, width, height);
}

void x_fill_rectangle(x_connection *_c, x_window *_w, int color,
    int x, int y, int width, int height)
{
  struct x_connection *c = _c;
  struct x_window *w = _w;
  XFillRectangle(c->d, w->p, c->colors[color], x, y, width, height);
}

void x_draw_string(x_connection *_c, x_window *_w, int color,
    int x, int y, const char *t)
{
  struct x_connection *c = _c;
  struct x_window *w = _w;
  int tlen = strlen(t);
  XDrawString(c->d, w->p, c->colors[color], x, y, t, tlen);
}

void x_draw_clipped_string(x_connection *_c, x_window *_w, int color,
    int x, int y, const char *t,
    int clipx, int clipy, int clipwidth, int clipheight)
{
  struct x_connection *c = _c;

  XRectangle clip = { clipx, clipy, clipwidth, clipheight };
  XSetClipRectangles(c->d, c->colors[color], 0, 0, &clip, 1, Unsorted);
  x_draw_string(_c, _w, color, x, y, t);
  XSetClipMask(c->d, c->colors[color], None);
}

void x_draw(x_connection *_c, x_window *_w)
{
  struct x_connection *c = _c;
  struct x_window *w = _w;
printf("x_draw XCopyArea w h %d %d display %p window %d pixmap %d\n", w->width, w->height, c->d, (int)w->w, (int)w->p);
  XCopyArea(c->d, w->p, w->w, c->colors[1], 0, 0, w->width, w->height, 0, 0);
}

/* those two special functions are to plot many points
 * first call x_add_point many times then x_plot_points once
 */
void x_add_point(x_connection *_c, int x, int y)
{
  struct x_connection *c = _c;

  if (c->pts_size == c->pts_maxsize) {
    c->pts_maxsize += 65536;
    c->pts = realloc(c->pts, c->pts_maxsize * sizeof(XPoint));
    if (c->pts == NULL) OOM;
  }

  c->pts[c->pts_size].x = x;
  c->pts[c->pts_size].y = y;
  c->pts_size++;
}

void x_plot_points(x_connection *_c, x_window *_w, int color)
{
  struct x_connection *c = _c;
fprintf(stderr, "x_plot_points %d points\n", c->pts_size);
  struct x_window *w = _w;
  XDrawPoints(c->d, w->p, c->colors[color], c->pts, c->pts_size,
      CoordModeOrigin);
  c->pts_size = 0;
}
