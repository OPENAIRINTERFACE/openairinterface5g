#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include "../utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if 0
/* this version behaves differently when you resize the XY plot. Say
 * you increase the size. The old (smaller) view is put at the center
 * of the new view. Everything inside keeps the same size/aspect ratio.
 * The other version below resizes the old view so that it fully occupies
 * the new view. It may introduce aspect ratio changes, but usage seems
 * to suggest it's a better behaviour.
 */
static void paint(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;
  int wanted_plot_width, allocated_plot_width;
  int wanted_plot_height, allocated_plot_height;
  float pxsize;
  float ticdist;
  float tic;
  float ticstep;
  int k, kmin, kmax;
  float allocated_xmin, allocated_xmax;
  float allocated_ymin, allocated_ymax;
  float center;
  int i;
  int n;

# define FLIP(v) (-(v) + allocated_plot_height-1)

  LOGD("PAINT xy plot xywh %d %d %d %d\n", this->common.x, this->common.y, this->common.width, this->common.height);

//x_draw_rectangle(g->x, g->xwin, 1, this->common.x, this->common.y, this->common.width, this->common.height);

  wanted_plot_width = this->wanted_width;
  allocated_plot_width = this->common.width - this->vrule_width;
  wanted_plot_height = this->wanted_height;
  allocated_plot_height = this->common.height - this->label_height * 2;

  /* plot zone */
  /* TODO: refine height - height of hrule text may be != from label */
  x_draw_rectangle(g->x, g->xwin, 1,
      this->common.x + this->vrule_width,
      this->common.y,
      this->common.width - this->vrule_width -1, /* -1 to see right border */
      this->common.height - this->label_height * 2);

  /* horizontal tics */
  pxsize = (this->xmax - this->xmin) / wanted_plot_width;
  ticdist = 100;
  tic = floor(log10(ticdist * pxsize));
  ticstep = powf(10, tic);
  center = (this->xmax + this->xmin) / 2;
  allocated_xmin = center - ((this->xmax - this->xmin) *
                             allocated_plot_width / wanted_plot_width) / 2;
  allocated_xmax = center + ((this->xmax - this->xmin) *
                             allocated_plot_width / wanted_plot_width) / 2;
  /* adjust tic if too tight */
  LOGD("pre x ticstep %g\n", ticstep);
  while (1) {
    if (ticstep / (allocated_xmax - allocated_xmin)
                * (allocated_plot_width - 1) > 40) break;
    ticstep *= 2;
  }
  LOGD("post x ticstep %g\n", ticstep);
  LOGD("xmin/max %g %g width wanted allocated %d %d alloc xmin/max %g %g ticstep %g\n", this->xmin, this->xmax, wanted_plot_width, allocated_plot_width, allocated_xmin, allocated_xmax, ticstep);
  kmin = ceil(allocated_xmin / ticstep);
  kmax = floor(allocated_xmax / ticstep);
  for (k = kmin; k <= kmax; k++) {
/*
    (k * ticstep - allocated_xmin) / (allocated_max - allocated_xmin) =
    (x - 0) / (allocated_plot_width-1 - 0)
 */
    char v[64];
    int vwidth, dummy;
    int x = (k * ticstep - allocated_xmin) /
            (allocated_xmax - allocated_xmin) *
            (allocated_plot_width - 1);
    x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width + x,
        this->common.y + this->common.height - this->label_height * 2,
        this->common.x + this->vrule_width + x,
        this->common.y + this->common.height - this->label_height * 2 - 5);
    sprintf(v, "%g", k * ticstep);
    x_text_get_dimensions(g->x, DEFAULT_FONT, v, &vwidth, &dummy, &dummy);
    x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
        this->common.x + this->vrule_width + x - vwidth/2,
        this->common.y + this->common.height - this->label_height * 2 +
            this->label_baseline,
        v);
    LOGD("tic k %d val %g x %d\n", k, k * ticstep, x);
  }

  /* vertical tics */
  pxsize = (this->ymax - this->ymin) / wanted_plot_height;
  ticdist = 30;
  tic = floor(log10(ticdist * pxsize));
  ticstep = powf(10, tic);
  center = (this->ymax + this->ymin) / 2;
  allocated_ymin = center - ((this->ymax - this->ymin) *
                             allocated_plot_height / wanted_plot_height) / 2;
  allocated_ymax = center + ((this->ymax - this->ymin) *
                             allocated_plot_height / wanted_plot_height) / 2;
  /* adjust tic if too tight */
  LOGD("pre y ticstep %g\n", ticstep);
  while (1) {
    if (ticstep / (allocated_ymax - allocated_ymin)
                * (allocated_plot_height - 1) > 20) break;
    ticstep *= 2;
  }
  LOGD("post y ticstep %g\n", ticstep);
  LOGD("ymin/max %g %g height wanted allocated %d %d alloc ymin/max %g %g ticstep %g\n", this->ymin, this->ymax, wanted_plot_height, allocated_plot_height, allocated_ymin, allocated_ymax, ticstep);
  kmin = ceil(allocated_ymin / ticstep);
  kmax = floor(allocated_ymax / ticstep);
  for (k = kmin; k <= kmax; k++) {
    char v[64];
    int vwidth, dummy;
    int y = (k * ticstep - allocated_ymin) /
            (allocated_ymax - allocated_ymin) *
            (allocated_plot_height - 1);
    sprintf(v, "%g", k * ticstep);
    x_text_get_dimensions(g->x, DEFAULT_FONT, v, &vwidth, &dummy, &dummy);
    x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width,
        this->common.y + FLIP(y),
        this->common.x + this->vrule_width + 5,
        this->common.y + FLIP(y));
    x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
        this->common.x + this->vrule_width - vwidth - 2,
        this->common.y + FLIP(y) - this->label_height/2+this->label_baseline,
        v);
  }

  /* label at bottom, in the middle */
  x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
      this->common.x + (this->common.width - this->label_width) / 2,
      this->common.y + this->common.height - this->label_height
          + this->label_baseline,
      this->label);

  for (n = 0; n < this->nplots; n++) {
    /* points */
    float ax, bx, ay, by;
    ax = (allocated_plot_width-1) / (allocated_xmax - allocated_xmin);
    bx = -ax * allocated_xmin;
    ay = (allocated_plot_height-1) / (allocated_ymax - allocated_ymin);
    by = -ay * allocated_ymin;
    for (i = 0; i < this->plots[n].npoints; i++) {
      int x, y;
      x = ax * this->plots[n].x[i] + bx;
      y = ay * this->plots[n].y[i] + by;
      if (x >= 0 && x < allocated_plot_width &&
          y >= 0 && y < allocated_plot_height)
        x_add_point(g->x,
            this->common.x + this->vrule_width + x,
            this->common.y + FLIP(y));
    }
    x_plot_points(g->x, g->xwin, this->plots[n].color);
  }
}
#endif

static void paint(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;
  int allocated_plot_width;
  int allocated_plot_height;
  float pxsize;
  float ticdist;
  float tic;
  float ticstep;
  int k, kmin, kmax;
  float allocated_xmin, allocated_xmax;
  float allocated_ymin, allocated_ymax;
  int i;
  int n;
  char v[64];
  int vwidth, dummy;

# define FLIP(v) (-(v) + allocated_plot_height-1)

  LOGD("PAINT xy plot xywh %d %d %d %d\n", this->common.x, this->common.y, this->common.width, this->common.height);

//x_draw_rectangle(g->x, g->xwin, 1, this->common.x, this->common.y, this->common.width, this->common.height);

  allocated_plot_width = this->common.width - this->vrule_width;
  allocated_plot_height = this->common.height - this->label_height * 2;

  if (allocated_plot_width <= 1 || allocated_plot_height <= 1) {
    WARN("PAINT xy plot: width (%d) or height (%d) is wrong, not painting\n",
         this->common.width, this->common.height);
    return;
  }

  /* plot zone */
  /* TODO: refine height - height of hrule text may be != from label */
  x_draw_rectangle(g->x, g->xwin, 1,
      this->common.x + this->vrule_width,
      this->common.y,
      this->common.width - this->vrule_width -1, /* -1 to see right border */
      this->common.height - this->label_height * 2);

  /* horizontal tics */
  pxsize = (this->xmax - this->xmin) / allocated_plot_width;
  ticdist = 100;
  tic = floor(log10(ticdist * pxsize));
  ticstep = powf(10, tic);
  allocated_xmin = this->xmin;
  allocated_xmax = this->xmax;
  /* adjust tic if too tight */
  LOGD("pre x ticstep %g\n", ticstep);
  while (1) {
    if (ticstep / (allocated_xmax - allocated_xmin)
                * (allocated_plot_width - 1) > 40) break;
    ticstep *= 2;
  }
  LOGD("post x ticstep %g\n", ticstep);
  LOGD("xmin/max %g %g width allocated %d alloc xmin/max %g %g ticstep %g\n", this->xmin, this->xmax, allocated_plot_width, allocated_xmin, allocated_xmax, ticstep);
  kmin = ceil(allocated_xmin / ticstep);
  kmax = floor(allocated_xmax / ticstep);
  for (k = kmin; k <= kmax; k++) {
/*
    (k * ticstep - allocated_xmin) / (allocated_max - allocated_xmin) =
    (x - 0) / (allocated_plot_width-1 - 0)
 */
    int x = (k * ticstep - allocated_xmin) /
            (allocated_xmax - allocated_xmin) *
            (allocated_plot_width - 1);
    int px;
    x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width + x,
        this->common.y + this->common.height - this->label_height * 2,
        this->common.x + this->vrule_width + x,
        this->common.y + this->common.height - this->label_height * 2 - 5);
    sprintf(v, "%g", k * ticstep);
    x_text_get_dimensions(g->x, DEFAULT_FONT, v, &vwidth, &dummy, &dummy);

    /* do not draw after the widget ('width-1' for if we use 'width'
     * it is still off by 1 pixel for whatever reason) */
    px = this->vrule_width + x - vwidth/2;
    if (px + vwidth > this->common.width-1)
      px = this->common.width-1 - vwidth;

    x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
        this->common.x + px,
        this->common.y + this->common.height - this->label_height * 2 +
            this->label_baseline,
        v);
    LOGD("tic k %d val %g x %d\n", k, k * ticstep, x);
  }

  /* vertical tics */
  allocated_ymin = this->ymin;
  allocated_ymax = this->ymax;
  switch (this->tick_type) {
  case XY_PLOT_DEFAULT_TICK:
    pxsize = (this->ymax - this->ymin) / allocated_plot_height;
    ticdist = 30;
    tic = floor(log10(ticdist * pxsize));
    ticstep = powf(10, tic);
    /* adjust tic if too tight */
    LOGD("pre y ticstep %g\n", ticstep);
    while (1) {
      if (ticstep / (allocated_ymax - allocated_ymin)
                  * (allocated_plot_height - 1) > 20) break;
      ticstep *= 2;
    }
    LOGD("post y ticstep %g\n", ticstep);
    LOGD("ymin/max %g %g height allocated %d alloc "
         "ymin/max %g %g ticstep %g\n", this->ymin, this->ymax,
         allocated_plot_height, allocated_ymin, allocated_ymax, ticstep);
    kmin = ceil(allocated_ymin / ticstep);
    kmax = floor(allocated_ymax / ticstep);
    for (k = kmin; k <= kmax; k++) {
      int y = (k * ticstep - allocated_ymin) /
              (allocated_ymax - allocated_ymin) *
              (allocated_plot_height - 1);
      sprintf(v, "%g", k * ticstep);
      x_text_get_dimensions(g->x, DEFAULT_FONT, v, &vwidth, &dummy, &dummy);
      x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
          this->common.x + this->vrule_width,
          this->common.y + FLIP(y),
          this->common.x + this->vrule_width + 5,
          this->common.y + FLIP(y));
      x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
          this->common.x + this->vrule_width - vwidth - 2,
          this->common.y + FLIP(y)-this->label_height/2+this->label_baseline,
          v);
    }
    break;
  case XY_PLOT_SCROLL_TICK:
    /* print only max value, formatted using 'bps' for readability */
    bps(v, this->ymax, "");
    x_text_get_dimensions(g->x, DEFAULT_FONT, v, &vwidth, &dummy, &dummy);
    x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
        this->common.x + this->vrule_width - vwidth - 2,
        this->common.y + +this->label_baseline,
        v);
    /* vertically divide into 5 */
    for (k = 1; k < 5; k++) {
      int y = round((k * (allocated_plot_height - 1)) / 5.);
      x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
          this->common.x + this->vrule_width,
          this->common.y + y,
          this->common.x + this->vrule_width + 5,
          this->common.y + y);
    }
    break;
  } /* swich (this->tick_type) */

  /* label at bottom, in the middle */
  x_draw_string(g->x, g->xwin, DEFAULT_FONT, FOREGROUND_COLOR,
      this->common.x + (this->common.width - this->label_width) / 2,
      this->common.y + this->common.height - this->label_height
          + this->label_baseline,
      this->label);

  for (n = 0; n < this->nplots; n++) {
    /* points */
    float ax, bx, ay, by;
    ax = (allocated_plot_width-1) / (allocated_xmax - allocated_xmin);
    bx = -ax * allocated_xmin;
    ay = (allocated_plot_height-1) / (allocated_ymax - allocated_ymin);
    by = -ay * allocated_ymin;
    for (i = 0; i < this->plots[n].npoints; i++) {
      int x, y;
      x = ax * this->plots[n].x[i] + bx;
      y = ay * this->plots[n].y[i] + by;
      if (x >= 0 && x < allocated_plot_width &&
          y >= 0 && y < allocated_plot_height)
        x_add_point(g->x,
            this->common.x + this->vrule_width + x,
            this->common.y + FLIP(y));
    }
    x_plot_points(g->x, g->xwin, this->plots[n].color);
  }
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct xy_plot_widget *w = _w;
  *width = w->wanted_width + w->vrule_width;
  *height = w->wanted_height + w->label_height * 2; /* TODO: refine */
  LOGD("HINTS xy plot wh %d %d (vrule_width %d) (wanted wh %d %d)\n", *width, *height, w->vrule_width, w->wanted_width, w->wanted_height);
}

widget *new_xy_plot(gui *_gui, int width, int height, char *label,
    int vruler_width)
{
  struct gui *g = _gui;
  struct xy_plot_widget *w;

  glock(g);

  w = new_widget(g, XY_PLOT, sizeof(struct xy_plot_widget));

  w->label = strdup(label); if (w->label == NULL) OOM;
  /* TODO: be sure calling X there is valid wrt "global model" (we are
   * not in the "gui thread") */
  x_text_get_dimensions(g->x, DEFAULT_FONT, label,
      &w->label_width, &w->label_height, &w->label_baseline);
  LOGD("XY PLOT label wh %d %d\n", w->label_width, w->label_height);

  w->wanted_width = width;
  w->wanted_height = height;
  w->vrule_width = vruler_width;

  w->xmin = -1;
  w->xmax = 1;
  w->ymin = -1;
  w->ymax = 1;
  w->plots = NULL;
  w->nplots = 0;
  w->tick_type = XY_PLOT_DEFAULT_TICK;

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}

/*************************************************************************/
/*                           public functions                            */
/*************************************************************************/

int xy_plot_new_plot(gui *_gui, widget *_this, int color)
{
  int ret;
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;

  glock(g);

  ret = this->nplots;

  this->nplots++;
  this->plots = realloc(this->plots,
      this->nplots * sizeof(struct xy_plot_plot));
  if (this->plots == NULL) abort();

  this->plots[ret].x = NULL;
  this->plots[ret].y = NULL;
  this->plots[ret].npoints = 0;
  this->plots[ret].color = color;

  gunlock(g);

  return ret;
}

void xy_plot_set_range(gui *_gui, widget *_this,
    float xmin, float xmax, float ymin, float ymax)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;

  glock(g);

  this->xmin = xmin;
  this->xmax = xmax;
  this->ymin = ymin;
  this->ymax = ymax;

  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void xy_plot_set_points(gui *_gui, widget *_this, int plot,
    int npoints, float *x, float *y)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;

  glock(g);

  if (npoints != this->plots[plot].npoints) {
    free(this->plots[plot].x);
    free(this->plots[plot].y);
    this->plots[plot].x = calloc(npoints, sizeof(float));
    if (this->plots[plot].x == NULL) abort();
    this->plots[plot].y = calloc(npoints, sizeof(float));
    if (this->plots[plot].y == NULL) abort();
    this->plots[plot].npoints = npoints;
  }

  memcpy(this->plots[plot].x, x, npoints * sizeof(float));
  memcpy(this->plots[plot].y, y, npoints * sizeof(float));

  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void xy_plot_get_dimensions(gui *_gui, widget *_this, int *width, int *height)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;

  glock(g);

  if (this->common.width == 0 || this->common.height == 0) {
    *width = this->wanted_width;
    *height = this->wanted_height;
  } else {
    *width = this->common.width - this->vrule_width;
    *height = this->common.height - this->label_height * 2;
  }

  gunlock(g);
}

void xy_plot_set_title(gui *_gui, widget *_this, char *label)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;

  glock(g);

  free(this->label);
  this->label = strdup(label); if (this->label == NULL) OOM;
  /* TODO: be sure calling X there is valid wrt "global model" (we are
   * not in the "gui thread") */
  x_text_get_dimensions(g->x, DEFAULT_FONT, label,
      &this->label_width, &this->label_height, &this->label_baseline);

  send_event(g, REPACK, this->common.id);

  gunlock(g);
}

void xy_plot_set_tick_type(gui *_gui, widget *_this, int type)
{
  struct gui *g = _gui;
  struct xy_plot_widget *this = _this;

  glock(g);

  this->tick_type = type;

  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}
