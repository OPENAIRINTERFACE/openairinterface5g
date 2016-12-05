#ifndef _GUI_DEFS_H_
#define _GUI_DEFS_H_

/* defines the private API of the GUI */

extern int volatile gui_logd;
#define LOGD(...) do { if (gui_logd) printf(__VA_ARGS__); } while (0)

/*************************************************************************/
/*                            logging macros                             */
/*************************************************************************/

#define ERR(...) \
  do { \
    printf("%s:%d:%s: ERROR: ", __FILE__, __LINE__, __FUNCTION__); \
    printf(__VA_ARGS__); \
    abort(); \
  } while (0)

#define WARN(...) \
  do { \
    printf("%s:%d:%s: WARNING: ", __FILE__, __LINE__, __FUNCTION__); \
    printf(__VA_ARGS__); \
  } while (0)

#define OOM ERR("out of memory\n")

/*************************************************************************/
/*                             widgets                                   */
/*************************************************************************/

enum widget_type {
  TOPLEVEL_WINDOW, CONTAINER, POSITIONER, TEXT_LIST, XY_PLOT, BUTTON, LABEL,
  TEXTAREA, TIMELINE, SPACE, IMAGE
};

struct widget_list;

struct widget {
  enum widget_type type;
  int id;
  int x;            /* allocated x after packing */
  int y;            /* allocated y after packing */
  int width;        /* allocated width after packing */
  int height;       /* allocated height after packing */
  struct widget_list *children;
  struct widget *parent;
  void (*repack)(gui *g, widget *this);
  void (*add_child)(gui *g, widget *this, widget *child, int position);
  void (*del_child)(gui *g, widget *this, widget *child);
  void (*allocate)(gui *g, widget *this, int x, int y, int width, int height);
  void (*hints)(gui *g, widget *this, int *width, int *height);
  void (*paint)(gui *g, widget *this);
  void (*clear)(gui *g, widget *this);
  /* user input */
  void (*button)(gui *g, widget *this, int x, int y, int key_modifiers,
      int button, int up);
};

struct widget_list {
  struct widget *item;
  struct widget_list *next;
  //struct widget_list *prev;  /* unused? */
  struct widget_list *last;  /* valid only for the head of the list */
};

struct toplevel_window_widget {
  struct widget common;
  void *x;                /* opaque X data (type x_window), used in x.c */
};

struct container_widget {
  struct widget common;
  int vertical;
  int hints_are_valid;     /* used to cache hints values */
  int hint_width;          /* cached hint values - invalid if */
  int hint_height;         /* repack_was_called == 1          */
  int *growable;
  int nchildren;
};

struct positioner_widget {
  struct widget common;
};

struct textlist_widget {
  struct widget common;
  char **text;
  int *color;
  int text_count;
  int wanted_width;
  int wanted_nlines;    /* number of lines of text the user wants to see */
  int allocated_nlines; /* actual number of visible lines */
  int starting_line;    /* points to the first visible line of text */
  int line_height;
  int baseline;
  int background_color;
};

struct xy_plot_plot {
  float *x;
  float *y;
  int npoints;
  int color;
};

struct xy_plot_widget {
  struct widget common;
  char *label;
  int label_width;
  int label_height;
  int label_baseline;
  int vrule_width;       /* the width of the vertical ruler text zone */
  float xmin, xmax;
  float ymin, ymax;
  int wanted_width;
  int wanted_height;
  struct xy_plot_plot *plots;
  int nplots;
  int tick_type;
};

struct timeline_subline {
  int *color;                  /* length = width of timeline widget
                                * value = -1 if no color
                                */
  int width;
  int background;              /* background color of the subline */
};

struct timeline_widget {
  struct widget common;
  int n;                         /* number of sublines */
  struct timeline_subline *s;
  int subline_height;
  int wanted_width;
};

struct button_widget {
  struct widget common;
};

struct label_widget {
  struct widget common;
  char *t;
  int color;
  int width;         /* as given by the graphic's backend */
  int height;        /* as given by the graphic's backend */
  int baseline;      /* as given by the graphic's backend */
};

struct textarea_widget {
  struct widget common;
  char *t;
  int tmaxsize;
  int color;
  int wanted_width;
  int wanted_height;
  int baseline;      /* as given by the graphic's backend */
  int text_width;    /* as given by the graphic's backend */
};

struct space_widget {
  struct widget common;
  int wanted_width;
  int wanted_height;
};

struct image_widget {
  struct widget common;
  int width;
  int height;
  void *x;             /* opaque X data (type x_image), used in x.c */
};

/*************************************************************************/
/*                             events                                    */
/*************************************************************************/

typedef void event;

enum event_type {
  DIRTY, REPACK
};

struct event {
  enum event_type type;
};

struct event_list {
  struct event *item;
  struct event_list *next;
  struct event_list *last;
};

struct dirty_event {
  struct event common;
  int id;
};

struct repack_event {
  struct event common;
  int id;
};

/*************************************************************************/
/*                           notifications                               */
/*************************************************************************/

/* same type as in gui.h */
typedef void (*notifier)(void *private, gui *g,
    char *notification, widget *w, void *notification_data);

struct notifier {
  notifier handler;
  unsigned long id;
  char *notification;
  widget *w;
  void *private;
  /* done is used bu gui_notify */
  int done;
};

/*************************************************************************/
/*                          main structure                               */
/*************************************************************************/

struct gui {
  void                *lock;
  void                *x; /* opaque X data (type x_connection), used in x.c */
  struct widget_list  *toplevel;
  struct event_list   *queued_events;
  int                 event_pipe[2];
  int                 next_id;         /* tells what is the ID of
                                          the next created widget */
  int                 repainted;       /* set to 1 when some widget has
                                        * been repainted (TODO: can be any,
                                        * to be optimized) */
  void                *xwin;           /* set by a toplevel_window when
                                        * it paints itself, to be used
                                        * by its children */
  struct notifier     *notifiers;
  int                 notifiers_count;
  unsigned long       next_notifier_id;
};

/*************************************************************************/
/*                            internal functions                         */
/*************************************************************************/

widget *new_widget(struct gui *g, enum widget_type type, int size);
void widget_add_child_internal(
    gui *_gui, widget *parent, widget *child, int position);
void widget_del_child_internal(gui *_gui, widget *parent, widget *child);
int widget_get_child_position(gui *_gui, widget *parent, widget *child);

const char *widget_name(enum widget_type type);

void send_event(gui *gui, enum event_type type, ...);
void gui_events(gui *gui);

struct widget *find_widget(struct gui *g, int id);

void gui_notify(struct gui *g, char *notification, widget *w,
    void *notification_data);

#endif /* _GUI_DEFS_H_ */
