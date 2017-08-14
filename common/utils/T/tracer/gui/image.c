#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

static void paint(gui *_gui, widget *_w)
{
  struct gui *g = _gui;
  struct image_widget *w = _w;
  LOGD("PAINT image %p\n", w);
  x_draw_image(g->x, g->xwin, w->x, w->common.x, w->common.y);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct image_widget *w = _w;
  LOGD("HINTS image %p\n", w);
  *width = w->width;
  *height = w->height;
}

struct png_reader {
  unsigned char *data;
  int size;
  int pos;
};

static void png_readfn(png_structp png_ptr, png_bytep data, png_size_t length)
{
  struct png_reader *r = png_get_io_ptr(png_ptr);
  if (length > r->size - r->pos) png_error(png_ptr, "bad png image");
  memcpy(data, r->data + r->pos, length);
  r->pos += length;
}

static void load_image(struct gui *g, struct image_widget *w,
    unsigned char *data, int length)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytepp image;
  int width, height, bit_depth, color_type, channels;
  unsigned char *img_data;
  struct png_reader r;
  int i;

  /* unpack PNG data */

  r.data = data;
  r.size = length;
  r.pos = 0;

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) abort();

  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) abort();

  if (setjmp(png_jmpbuf(png_ptr))) abort();

  png_set_read_fn(png_ptr, &r, png_readfn);

  png_read_png(png_ptr, info_ptr,
      PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING |
      PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_BGR, NULL);

  image = png_get_rows(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);
  channels = png_get_channels(png_ptr, info_ptr);

  if (width < 1 || width > 1000 || height < 1 || height > 1000 ||
      bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGBA || channels != 4)
    { printf("bad image\n"); abort(); }

  img_data = malloc(4 * width * height); if (img_data == NULL) abort();
  for (i = 0; i < height; i++)
    memcpy(img_data+i*4*width, image[i], width*4);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  /* create the X image */
  w->x = x_create_image(g->x, img_data, width, height);

  free(img_data);

  w->width = width;
  w->height = height;
}

widget *new_image(gui *_gui, unsigned char *data, int length)
{
  struct gui *g = _gui;
  struct image_widget *w;

  glock(g);

  w = new_widget(g, IMAGE, sizeof(struct image_widget));

  load_image(g, w, data, length);

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}
