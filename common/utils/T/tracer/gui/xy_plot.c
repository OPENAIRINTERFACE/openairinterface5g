#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

printf("PAINT xy plot xywh %d %d %d %d\n", this->common.x, this->common.y, this->common.width, this->common.height);

//x_draw_rectangle(g->x, g->xwin, 1, this->common.x, this->common.y, this->common.width, this->common.height);

  /* plot zone */
  /* TODO: refine height - height of hrule text may be != from label */
  x_draw_rectangle(g->x, g->xwin, 1,
      this->common.x + this->vrule_width,
      this->common.y,
      this->common.width - this->vrule_width -1, /* -1 to see right border */
      this->common.height - this->label_height * 2);

  /* horizontal tics */
  wanted_plot_width = this->wanted_width;
  allocated_plot_width = this->common.width - this->vrule_width;
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
printf("pre x ticstep %g\n", ticstep);
  while (1) {
    if (ticstep / (allocated_xmax - allocated_xmin)
                * (allocated_plot_width - 1) > 40) break;
    ticstep *= 2;
  }
printf("post x ticstep %g\n", ticstep);
printf("xmin/max %g %g width wanted allocated %d %d alloc xmin/max %g %g ticstep %g\n", this->xmin, this->xmax, wanted_plot_width, allocated_plot_width, allocated_xmin, allocated_xmax, ticstep);
  kmin = ceil(allocated_xmin / ticstep);
  kmax = floor(allocated_xmax / ticstep);
  for (k = kmin; k <= kmax; k++) {
/*
    (k * ticstep - allocated_xmin) / (allocated_max - allocated_xmin) =
    (x - 0) / (allocated_plot_width-1 - 0)
 */
    char v[64];
    int vwidth, dummy;
    float x = (k * ticstep - allocated_xmin) /
              (allocated_xmax - allocated_xmin) *
              (allocated_plot_width - 1);
    x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width + x,
        this->common.y + this->common.height - this->label_height * 2,
        this->common.x + this->vrule_width + x,
        this->common.y + this->common.height - this->label_height * 2 - 5);
    sprintf(v, "%g", k * ticstep);
    x_text_get_dimensions(g->x, v, &vwidth, &dummy, &dummy);
    x_draw_string(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width + x - vwidth/2,
        this->common.y + this->common.height - this->label_height * 2 + this->label_baseline,
        v);
printf("tic k %d val %g x %g\n", k, k * ticstep, x);
  }

  /* vertical tics */
  wanted_plot_height = this->wanted_height;
  allocated_plot_height = this->common.height - this->label_height * 2;
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
printf("pre y ticstep %g\n", ticstep);
  while (1) {
    if (ticstep / (allocated_ymax - allocated_ymin)
                * (allocated_plot_height - 1) > 20) break;
    ticstep *= 2;
  }
printf("post y ticstep %g\n", ticstep);
printf("ymin/max %g %g height wanted allocated %d %d alloc ymin/max %g %g ticstep %g\n", this->ymin, this->ymax, wanted_plot_height, allocated_plot_height, allocated_ymin, allocated_ymax, ticstep);
  kmin = ceil(allocated_ymin / ticstep);
  kmax = floor(allocated_ymax / ticstep);
  for (k = kmin; k <= kmax; k++) {
    char v[64];
    int vwidth, dummy;
    float y = (k * ticstep - allocated_ymin) /
              (allocated_ymax - allocated_ymin) *
              (allocated_plot_height - 1);
    sprintf(v, "%g", k * ticstep);
    x_text_get_dimensions(g->x, v, &vwidth, &dummy, &dummy);
    x_draw_line(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width,
        this->common.y + y,
        this->common.x + this->vrule_width + 5,
        this->common.y + y);
    x_draw_string(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x + this->vrule_width - vwidth - 2,
        this->common.y + y - this->label_height / 2 + this->label_baseline,
        v);
  }

  /* label at bottom, in the middle */
  x_draw_string(g->x, g->xwin, FOREGROUND_COLOR,
      this->common.x + (this->common.width - this->label_width) / 2,
      this->common.y + this->common.height - this->label_height
          + this->label_baseline,
      this->label);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct xy_plot_widget *w = _w;
  *width = w->wanted_width + w->vrule_width;
  *height = w->wanted_height + w->label_height * 2; /* TODO: refine */
printf("HINTS xy plot wh %d %d (vrule_width %d) (wanted wh %d %d)\n", *width, *height, w->vrule_width, w->wanted_width, w->wanted_height);
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
  x_text_get_dimensions(g->x, label, &w->label_width, &w->label_height,
      &w->label_baseline);
printf("XY PLOT label wh %d %d\n", w->label_width, w->label_height);

  w->wanted_width = width;
  w->wanted_height = height;
  w->vrule_width = vruler_width;

  w->xmin = -1;
  w->xmax = 1;
  w->ymin = -1;
  w->ymax = 1;

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}
