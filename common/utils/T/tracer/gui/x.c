#include "x.h"
#include "x_defs.h"
#include "gui_defs.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int x_connection_fd(x_connection *_x) {
  struct x_connection *x = _x;
  return ConnectionNumber(x->d);
}

static GC create_gc(Display *d, char *color) {
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

int x_new_color(x_connection *_x, char *color) {
  struct x_connection *x = _x;
  x->ncolors++;
  x->colors = realloc(x->colors, x->ncolors * sizeof(GC));

  if (x->colors == NULL) OOM;

  x->colors[x->ncolors-1] = create_gc(x->d, color);
  x->xft_colors = realloc(x->xft_colors, x->ncolors * sizeof(XftColor));

  if (x->xft_colors == NULL) OOM;

  if (XftColorAllocName(x->d, DefaultVisual(x->d, DefaultScreen(x->d)),
                        DefaultColormap(x->d, DefaultScreen(x->d)),
                        color, &x->xft_colors[x->ncolors-1]) == False)
    ERR("could not allocate color '%s'\n", color);

  return x->ncolors - 1;
}

int x_new_font(x_connection *_x, char *font) {
  struct x_connection *x = _x;
  /* TODO: allocate fonts only once */
  x->nfonts++;
  x->fonts = realloc(x->fonts, x->nfonts * sizeof(XftFont *));

  if (x->fonts == NULL) OOM;

  x->fonts[x->nfonts-1] = XftFontOpenName(x->d, DefaultScreen(x->d), font);

  if (x->fonts[x->nfonts-1] == NULL)
    ERR("failed allocating font '%s'\n", font);

  return x->nfonts - 1;
}

x_connection *x_open(void) {
  struct x_connection *ret;
  ret = calloc(1, sizeof(struct x_connection));

  if (ret == NULL) OOM;

  ret->d = XOpenDisplay(0);
  LOGD("XOpenDisplay display %p return x_connection %p\n", ret->d, ret);

  if (ret->d == NULL) ERR("error calling XOpenDisplay: no X? you root?\n");

  x_new_color(ret, "white");    /* background color */
  x_new_color(ret, "black");    /* foreground color */
  x_new_font(ret, "sans-8");
  return ret;
}

x_window *x_create_window(x_connection *_x, int width, int height,
                          char *title) {
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
  ret->xft = XftDrawCreate(x->d, ret->p,
                           DefaultVisual(x->d, DefaultScreen(x->d)),
                           DefaultColormap(x->d, DefaultScreen(x->d)));

  if (ret->xft == NULL) ERR("XftDrawCreate failed\n");

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
  return ret;
}

x_image *x_create_image(x_connection *_x, unsigned char *data,
                        int width, int height) {
  struct x_connection *x = _x;
  struct x_image *ret;
  XImage *ximage;
  XVisualInfo *vs;
  XVisualInfo template;
  int nvs;
  Visual *v;
  ret = calloc(1, sizeof(struct x_image));

  if (ret == NULL) OOM;

  template.class = TrueColor;
  template.depth = 24;
  template.red_mask = 0xff0000;
  template.green_mask = 0x00ff00;
  template.blue_mask = 0x0000ff;
  template.bits_per_rgb = 8;
  vs = XGetVisualInfo(x->d, VisualDepthMask | VisualClassMask |
                      VisualRedMaskMask | VisualGreenMaskMask | VisualBlueMaskMask |
                      VisualBitsPerRGBMask, &template, &nvs);

  if (vs == NULL) {
    /* try again with 32 bpp */
    template.depth = 32;
    vs = XGetVisualInfo(x->d, VisualDepthMask | VisualClassMask |
                        VisualRedMaskMask | VisualGreenMaskMask | VisualBlueMaskMask |
                        VisualBitsPerRGBMask, &template, &nvs);
  }

  if (vs == NULL) ERR("no good visual found\n");

  v = vs[0].visual;
  XFree(vs);
  ximage = XCreateImage(x->d, v, 24, ZPixmap, 0,
                        (char *)data, width, height, 32, 0);

  if (ximage == NULL) ERR("image creation failed\n");

  ret->p = XCreatePixmap(x->d, DefaultRootWindow(x->d), width, height, 24);
  XPutImage(x->d, ret->p, DefaultGC(x->d, DefaultScreen(x->d)),
            ximage, 0, 0, 0, 0, width, height);
  /* TODO: be sure it's fine to set data to NULL */
  ximage->data = NULL;
  XDestroyImage(ximage);
  ret->width = width;
  ret->height = height;
  return ret;
}

static struct toplevel_window_widget *find_x_window(struct gui *g, Window id) {
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

void x_events(gui *_gui) {
  struct gui *g = _gui;
  struct widget_list *cur;
  struct x_connection *x = g->x;
  struct toplevel_window_widget *w;
  LOGD("x_events START\n");
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
    LOGD("XEV %d\n", ev.type);

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

          LOGD("ConfigureNotify %d %d\n", ev.xconfigure.width, ev.xconfigure.height);
        }

        break;

      case ButtonPress:
        if ((w = find_x_window(g, ev.xbutton.window)) != NULL) {
          int key_modifiers = 0;

          if (ev.xbutton.state & ShiftMask)   key_modifiers |= KEY_SHIFT;

          if (ev.xbutton.state & Mod1Mask)    key_modifiers |= KEY_ALT;

          if (ev.xbutton.state & ControlMask) key_modifiers |= KEY_CONTROL;

          w->common.button(g, w, ev.xbutton.x, ev.xbutton.y, key_modifiers,
                           ev.xbutton.button, 0);
        }

        break;

      case ButtonRelease:
        if ((w = find_x_window(g, ev.xbutton.window)) != NULL) {
          int key_modifiers = 0;

          if (ev.xbutton.state & ShiftMask)   key_modifiers |= KEY_SHIFT;

          if (ev.xbutton.state & Mod1Mask)    key_modifiers |= KEY_ALT;

          if (ev.xbutton.state & ControlMask) key_modifiers |= KEY_CONTROL;

          w->common.button(g, w, ev.xbutton.x, ev.xbutton.y, key_modifiers,
                           ev.xbutton.button, 1);
        }

        break;

      default:
        if (gui_logd) WARN("TODO: X event type %d\n", ev.type);

        break;
    }
  }

  /* postprocessing */
  LOGD("post processing\n");
  cur = g->toplevel;

  while (cur) {
    struct toplevel_window_widget *w =
      (struct toplevel_window_widget *)cur->item;
    struct x_window *xw = w->x;

    if (xw->resize) {
      LOGD("resize old %d %d new %d %d\n", xw->width, xw->height, xw->new_width, xw->new_height);

      if (xw->width != xw->new_width || xw->height != xw->new_height) {
        w->common.allocate(g, w, 0, 0, xw->new_width, xw->new_height);
        xw->width = xw->new_width;
        xw->height = xw->new_height;
        XftDrawDestroy(xw->xft);
        XFreePixmap(x->d, xw->p);
        xw->p = XCreatePixmap(x->d, xw->w, xw->width, xw->height,
                              DefaultDepth(x->d, DefaultScreen(x->d)));
        XFillRectangle(x->d, xw->p, x->colors[BACKGROUND_COLOR],
                       0, 0, xw->width, xw->height);
        xw->xft = XftDrawCreate(x->d, xw->p,
                                DefaultVisual(x->d, DefaultScreen(x->d)),
                                DefaultColormap(x->d, DefaultScreen(x->d)));

        if (xw->xft == NULL) ERR("XftDrawCreate failed\n");

        //xw->repaint = 1;
      }
    }

    if (xw->repaint) {
      w->common.paint(g, w);
      xw->redraw = 1;
    }

    if (xw->redraw) {
      struct x_connection *x = g->x;
      LOGD("XCopyArea w h %d %d\n", xw->width, xw->height);
      XCopyArea(x->d, xw->p, xw->w, x->colors[1],
                0, 0, xw->width, xw->height, 0, 0);
    }

    cur = cur->next;
  }

  LOGD("x_events DONE\n");
}

void x_flush(x_connection *_x) {
  struct x_connection *x = _x;
  XFlush(x->d);
}

void x_text_get_dimensions(x_connection *_c, int font, const char *t,
                           int *width, int *height, int *baseline) {
  struct x_connection *c = _c;
  XGlyphInfo ext;
  XftTextExtentsUtf8(c->d, c->fonts[font], (FcChar8 *)t, strlen(t), &ext);
  *width = ext.width;
  *height = c->fonts[font]->height;
  *baseline = c->fonts[font]->ascent;
}

/***********************************************************************/
/*                    public drawing functions                         */
/***********************************************************************/

void x_draw_line(x_connection *_c, x_window *_w, int color,
                 int x1, int y1, int x2, int y2) {
  struct x_connection *c = _c;
  struct x_window *w = _w;
  XDrawLine(c->d, w->p, c->colors[color], x1, y1, x2, y2);
}

void x_draw_rectangle(x_connection *_c, x_window *_w, int color,
                      int x, int y, int width, int height) {
  struct x_connection *c = _c;
  struct x_window *w = _w;
  XDrawRectangle(c->d, w->p, c->colors[color], x, y, width, height);
}

void x_fill_rectangle(x_connection *_c, x_window *_w, int color,
                      int x, int y, int width, int height) {
  struct x_connection *c = _c;
  struct x_window *w = _w;
  XFillRectangle(c->d, w->p, c->colors[color], x, y, width, height);
}

void x_draw_string(x_connection *_c, x_window *_w, int font, int color,
                   int x, int y, const char *t) {
  struct x_connection *c = _c;
  struct x_window *w = _w;
  int tlen = strlen(t);
  XftDrawStringUtf8(w->xft, &c->xft_colors[color], c->fonts[font],
                    x, y, (const unsigned char *)t, tlen);
}

void x_draw_clipped_string(x_connection *_c, x_window *_w, int font,
                           int color, int x, int y, const char *t,
                           int clipx, int clipy, int clipwidth, int clipheight) {
  struct x_window *w = _w;
  XRectangle clip = { clipx, clipy, clipwidth, clipheight };

  if (XftDrawSetClipRectangles(w->xft, 0, 0, &clip, 1) == False) abort();

  x_draw_string(_c, _w, font, color, x, y, t);

  if (XftDrawSetClip(w->xft, NULL) == False) abort();
}

void x_draw_image(x_connection *_c, x_window *_w, x_image *_img, int x, int y) {
  struct x_connection *c = _c;
  struct x_window *w = _w;
  struct x_image *img = _img;
  XCopyArea(c->d, img->p, w->p, DefaultGC(c->d, DefaultScreen(c->d)),
            0, 0, img->width, img->height, x, y);
}

void x_draw(x_connection *_c, x_window *_w) {
  struct x_connection *c = _c;
  struct x_window *w = _w;
  LOGD("x_draw XCopyArea w h %d %d display %p window %d pixmap %d\n", w->width, w->height, c->d, (int)w->w, (int)w->p);
  XCopyArea(c->d, w->p, w->w, c->colors[1], 0, 0, w->width, w->height, 0, 0);
}

/* those two special functions are to plot many points
 * first call x_add_point many times then x_plot_points once
 */
void x_add_point(x_connection *_c, int x, int y) {
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

void x_plot_points(x_connection *_c, x_window *_w, int color) {
  struct x_connection *c = _c;
  LOGD("x_plot_points %d points\n", c->pts_size);
  struct x_window *w = _w;
  XDrawPoints(c->d, w->p, c->colors[color], c->pts, c->pts_size,
              CoordModeOrigin);
  c->pts_size = 0;
}
