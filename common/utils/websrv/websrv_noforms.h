/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file common/utils/websrv/websrv_noforms.h
 * \brief: include file to replace forms.h when compiling nr_phy_scope.c for the webserver shared lib
 * \author Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifndef FL_FORMS_H
#define FL_FORMS_H

#define FL_VERSION 1
#define FL_REVISION 2
#define FL_FIXLEVEL "3"
#define FL_INCLUDE_VERSION (FL_VERSION * 1000 + FL_REVISION)

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#if defined __cplusplus
extern "C" {
#endif

#if defined _WIN32
#define FL_WIN32
#include <windows.h>
#endif

typedef void *Window;
typedef void *Pixmap;
#define FL_EXPORT inline

/**
 * \file Basic.h
 *
 *  Basic definitions and limits.
 *  Window system independent prototypes
 *
 *  Modify with care
 */

#ifndef FL_BASIC_H
#define FL_BASIC_H

#include <math.h>

#if defined __GNUC__
#define FL_UNUSED_ARG __attribute__((unused))
#else
#define FL_UNUSED_ARG
#endif

/* Some general constants */
typedef int Visual;
typedef int XEvent;
typedef int *Display;
typedef void *Cursor;
typedef void *XVisualInfo;
typedef void *XFontStruct;
typedef void *XPoint;
typedef void *XRectangle;
typedef int GC;
typedef void *Colormap;
typedef void *XrmOptionDescRec;
#define LineSolid 0
#define LineOnOffDash 1
#define LineDoubleDash 2

typedef struct {
  int f1;
} XSetWindowAttributes;
typedef struct {
  int f1;
} Atom;

typedef int KeySym;
enum {
  FL_ON = 1,
  FL_OK = 1,
  FL_VALID = 1,
  FL_PREEMPT = 1,
  FL_AUTO = 2,
  FL_WHEN_NEEDED = FL_AUTO,
  FL_OFF = 0,
  FL_CANCEL = 0,
  FL_INVALID = 0,

  /* WM_DELETE_WINDOW callback return */

  FL_IGNORE = -1,
};

/* Max  directory length  */

#ifndef FL_PATH_MAX
#ifndef PATH_MAX
#define FL_PATH_MAX 1024
#else
#define FL_PATH_MAX PATH_MAX
#endif
#endif /* ! def FL_PATH_MAX */

/* The screen coordinate unit, FL_Coord, must be of signed type */

typedef int FL_Coord;
#define FL_COORD FL_Coord

typedef unsigned long FL_COLOR;

/* Coordinates can be in pixels, milli-meters or points (1/72inch) */

typedef enum {
  FL_COORD_PIXEL, /* default, Pixel           */
  FL_COORD_MM, /* milli-meter              */
  FL_COORD_POINT, /* point                    */
  FL_COORD_centiMM, /* one hundredth of a mm    */
  FL_COORD_centiPOINT /* one hundredth of a point */
} FL_COORD_UNIT;

/* All object classes. */

typedef enum {
  FL_INVALID_CLASS, /*  0 */
  FL_BUTTON, /*  1 */
  FL_LIGHTBUTTON, /*  2 */
  FL_ROUNDBUTTON, /*  3 */
  FL_ROUND3DBUTTON, /*  4 */
  FL_CHECKBUTTON, /*  5 */
  FL_BITMAPBUTTON, /*  6 */
  FL_PIXMAPBUTTON, /*  7 */
  FL_BITMAP, /*  8 */
  FL_PIXMAP, /*  9 */
  FL_BOX, /* 10 */
  FL_TEXT, /* 11 */
  FL_MENU, /* 12 */
  FL_CHART, /* 13 */
  FL_CHOICE, /* 14 */
  FL_COUNTER, /* 15 */
  FL_SLIDER, /* 16 */
  FL_VALSLIDER, /* 17 */
  FL_INPUT, /* 18 */
  FL_BROWSER, /* 19 */
  FL_DIAL, /* 20 */
  FL_TIMER, /* 21 */
  FL_CLOCK, /* 22 */
  FL_POSITIONER, /* 23 */
  FL_FREE, /* 24 */
  FL_XYPLOT, /* 25 */
  FL_FRAME, /* 26 */
  FL_LABELFRAME, /* 27 */
  FL_CANVAS, /* 28 */
  FL_GLCANVAS, /* 29 */
  FL_TABFOLDER, /* 30 */
  FL_SCROLLBAR, /* 31 */
  FL_SCROLLBUTTON, /* 32 */
  FL_MENUBAR, /* 33 */
  FL_TEXTBOX, /* 34, for internal use only */
  FL_LABELBUTTON, /* 35 */
  FL_COMBOBOX, /* 36 */
  FL_IMAGECANVAS, /* 37 */
  FL_THUMBWHEEL, /* 38 */
  FL_COLORWHEEL, /* 39 */
  FL_FORMBROWSER, /* 40 */
  FL_SELECT, /* 41 */
  FL_NMENU, /* 42 */
  FL_SPINNER, /* 43 */
  FL_TBOX, /* 44 */
  FL_CLASS_END /* sentinel */
} FL_CLASS;

#define FL_BEGIN_GROUP 10000
#define FL_END_GROUP 20000

#define FL_USER_CLASS_START 1001 /* min. user class  value */
#define FL_USER_CLASS_END 9999 /* max. user class  value */

/* Maximum border width (in pixel) */

#define FL_MAX_BW 10

/* How to display a form onto screen */

typedef enum {
  FL_PLACE_FREE = 0, /* size remain resizable      */
  FL_PLACE_MOUSE = 1, /* mouse centered on form     */
  FL_PLACE_CENTER = 2, /* center of the screen       */
  FL_PLACE_POSITION = 4, /* specific position          */
  FL_PLACE_SIZE = 8, /* specific size              */
  FL_PLACE_GEOMETRY = 16, /* specific size and position */
  FL_PLACE_ASPECT = 32, /* keep aspect ratio          */
  FL_PLACE_FULLSCREEN = 64, /* scale to fit to screen     */
  FL_PLACE_HOTSPOT = 128, /* so mouse fall on (x,y)     */
  FL_PLACE_ICONIC = 256, /* start in iconified form    */

  /* Modifiers */

  FL_FREE_SIZE = (1 << 14),
  FL_FIX_SIZE = (1 << 15) /* seems to be useless, but some
           programs seem to rely on it... */
} FL_PLACE;

#define FL_PLACE_FREE_CENTER (FL_PLACE_CENTER | FL_FREE_SIZE)
#define FL_PLACE_CENTERFREE (FL_PLACE_CENTER | FL_FREE_SIZE)

/* Window manager decoration request and forms attributes */

enum {
  FL_FULLBORDER = 1, /* normal                                  */
  FL_TRANSIENT, /* set TRANSIENT_FOR property              */
  FL_NOBORDER, /* use override_redirect to supress decor. */
};

/* All box types */

typedef enum {
  FL_NO_BOX, /*  0 */
  FL_UP_BOX, /*  1 */
  FL_DOWN_BOX, /*  2 */
  FL_BORDER_BOX, /*  3 */
  FL_SHADOW_BOX, /*  4 */
  FL_FRAME_BOX, /*  5 */
  FL_ROUNDED_BOX, /*  6 */
  FL_EMBOSSED_BOX, /*  7 */
  FL_FLAT_BOX, /*  8 */
  FL_RFLAT_BOX, /*  9 */
  FL_RSHADOW_BOX, /* 10 */
  FL_OVAL_BOX, /* 11 */
  FL_ROUNDED3D_UPBOX, /* 12 */
  FL_ROUNDED3D_DOWNBOX, /* 13 */
  FL_OVAL3D_UPBOX, /* 14 */
  FL_OVAL3D_DOWNBOX, /* 15 */
  FL_OVAL3D_FRAMEBOX, /* 16 */
  FL_OVAL3D_EMBOSSEDBOX, /* 17 */

  /* for internal use only */

  FL_TOPTAB_UPBOX,
  FL_SELECTED_TOPTAB_UPBOX,
  FL_BOTTOMTAB_UPBOX,
  FL_SELECTED_BOTTOMTAB_UPBOX,

  FL_MAX_BOX_STYLES /* sentinel */
} FL_BOX_TYPE;

#define FL_IS_UPBOX(t) ((t) == FL_UP_BOX || (t) == FL_OVAL3D_UPBOX || (t) == FL_ROUNDED3D_UPBOX)

#define FL_IS_DOWNBOX(t) ((t) == FL_DOWN_BOX || (t) == FL_OVAL3D_DOWNBOX || (t) == FL_ROUNDED3D_DOWNBOX)

#define FL_TO_DOWNBOX(t) ((t) == FL_UP_BOX ? FL_DOWN_BOX : ((t) == FL_ROUNDED3D_UPBOX ? FL_ROUNDED3D_DOWNBOX : ((t) == FL_OVAL3D_UPBOX ? FL_OVAL3D_DOWNBOX : (t))))

/* How to place text relative to a box */

typedef enum {
  FL_ALIGN_CENTER,
  FL_ALIGN_TOP = 1,
  FL_ALIGN_BOTTOM = 2,
  FL_ALIGN_LEFT = 4,
  FL_ALIGN_RIGHT = 8,
  FL_ALIGN_LEFT_TOP = (FL_ALIGN_TOP | FL_ALIGN_LEFT),
  FL_ALIGN_RIGHT_TOP = (FL_ALIGN_TOP | FL_ALIGN_RIGHT),
  FL_ALIGN_LEFT_BOTTOM = (FL_ALIGN_BOTTOM | FL_ALIGN_LEFT),
  FL_ALIGN_RIGHT_BOTTOM = (FL_ALIGN_BOTTOM | FL_ALIGN_RIGHT),
  FL_ALIGN_INSIDE = (1 << 13),
  FL_ALIGN_VERT = (1 << 14), /* not functional yet  */

  /* the rest is for backward compatibility only, don't use! */

  FL_ALIGN_TOP_LEFT = FL_ALIGN_LEFT_TOP,
  FL_ALIGN_TOP_RIGHT = FL_ALIGN_RIGHT_TOP,
  FL_ALIGN_BOTTOM_LEFT = FL_ALIGN_LEFT_BOTTOM,
  FL_ALIGN_BOTTOM_RIGHT = FL_ALIGN_RIGHT_BOTTOM
} FL_ALIGN;

FL_EXPORT int fl_is_inside_lalign(int align)
{
  return 0;
};

FL_EXPORT int fl_is_outside_lalign(int align)
{
  return 0;
};

FL_EXPORT int fl_is_center_lalign(int align)
{
  return 0;
};

FL_EXPORT int fl_to_inside_lalign(int align)
{
  return 0;
};

FL_EXPORT int fl_to_outside_lalign(int align)
{
  return 0;
};

/* Mouse buttons. Don't have to be consecutive */

enum { FL_MBUTTON1 = 1, FL_MBUTTON2, FL_MBUTTON3, FL_MBUTTON4, FL_MBUTTON5 };

#define FL_LEFT_MOUSE FL_MBUTTON1
#define FL_MIDDLE_MOUSE FL_MBUTTON2
#define FL_RIGHT_MOUSE FL_MBUTTON3
#define FL_SCROLLUP_MOUSE FL_MBUTTON4
#define FL_SCROLLDOWN_MOUSE FL_MBUTTON5

#define FL_LEFTMOUSE FL_LEFT_MOUSE
#define FL_MIDDLEMOUSE FL_MIDDLE_MOUSE
#define FL_RIGHTMOUSE FL_RIGHT_MOUSE
#define FL_SCROLLUPMOUSE FL_SCROLLUP_MOUSE
#define FL_SCROLLDOWNMOUSE FL_SCROLLDOWN_MOUSE

/* control when to return input, slider and dial etc. object. */

#define FL_RETURN_NONE 0U
#define FL_RETURN_CHANGED 1U
#define FL_RETURN_END 2U
#define FL_RETURN_END_CHANGED 4U
#define FL_RETURN_SELECTION 8U
#define FL_RETURN_DESELECTION 16U
#define FL_RETURN_TRIGGERED 1024U
#define FL_RETURN_ALWAYS (~FL_RETURN_END_CHANGED)

/*  Some special color indices for FL private colormap. It does not matter
 *  what the value of each enum is, but it must start from 0 and be
 *  consecutive. */

typedef enum {
  FL_BLACK,
  FL_RED,
  FL_GREEN,
  FL_YELLOW,
  FL_BLUE,
  FL_MAGENTA,
  FL_CYAN,
  FL_WHITE,
  FL_TOMATO,
  FL_INDIANRED,
  FL_SLATEBLUE,
  FL_COL1,
  FL_RIGHT_BCOL,
  FL_BOTTOM_BCOL,
  FL_TOP_BCOL,
  FL_LEFT_BCOL,
  FL_MCOL,
  FL_INACTIVE,
  FL_PALEGREEN,
  FL_DARKGOLD,
  FL_ORCHID,
  FL_DARKCYAN,
  FL_DARKTOMATO,
  FL_WHEAT,
  FL_DARKORANGE,
  FL_DEEPPINK,
  FL_CHARTREUSE,
  FL_DARKVIOLET,
  FL_SPRINGGREEN,
  FL_DODGERBLUE,
  FL_LIGHTER_COL1,
  FL_DARKER_COL1,
  FL_ALICEBLUE,
  FL_ANTIQUEWHITE,
  FL_AQUA,
  FL_AQUAMARINE,
  FL_AZURE,
  FL_BEIGE,
  FL_BISQUE,
  FL_BLANCHEDALMOND,
  FL_BLUEVIOLET,
  FL_BROWN,
  FL_BURLYWOOD,
  FL_CADETBLUE,
  FL_CHOCOLATE,
  FL_CORAL,
  FL_CORNFLOWERBLUE,
  FL_CORNSILK,
  FL_CRIMSON,
  FL_DARKBLUE,
  FL_DARKGOLDENROD,
  FL_DARKGRAY,
  FL_DARKGREEN,
  FL_DARKGREY,
  FL_DARKKHAKI,
  FL_DARKMAGENTA,
  FL_DARKOLIVEGREEN,
  FL_DARKORCHID,
  FL_DARKRED,
  FL_DARKSALMON,
  FL_DARKSEAGREEN,
  FL_DARKSLATEBLUE,
  FL_DARKSLATEGRAY,
  FL_DARKSLATEGREY,
  FL_DARKTURQUOISE,
  FL_DEEPSKYBLUE,
  FL_DIMGRAY,
  FL_DIMGREY,
  FL_FIREBRICK,
  FL_FLORALWHITE,
  FL_FORESTGREEN,
  FL_FUCHSIA,
  FL_GAINSBORO,
  FL_GHOSTWHITE,
  FL_GOLD,
  FL_GOLDENROD,
  FL_GRAY,
  FL_GREENYELLOW,
  FL_GREY,
  FL_HONEYDEW,
  FL_HOTPINK,
  FL_INDIGO,
  FL_IVORY,
  FL_KHAKI,
  FL_LAVENDER,
  FL_LAVENDERBLUSH,
  FL_LAWNGREEN,
  FL_LEMONCHIFFON,
  FL_LIGHTBLUE,
  FL_LIGHTCORAL,
  FL_LIGHTCYAN,
  FL_LIGHTGOLDENRODYELLOW,
  FL_LIGHTGRAY,
  FL_LIGHTGREEN,
  FL_LIGHTGREY,
  FL_LIGHTPINK,
  FL_LIGHTSALMON,
  FL_LIGHTSEAGREEN,
  FL_LIGHTSKYBLUE,
  FL_LIGHTSLATEGRAY,
  FL_LIGHTSLATEGREY,
  FL_LIGHTSTEELBLUE,
  FL_LIGHTYELLOW,
  FL_LIME,
  FL_LIMEGREEN,
  FL_LINEN,
  FL_MAROON,
  FL_MEDIUMAQUAMARINE,
  FL_MEDIUMBLUE,
  FL_MEDIUMORCHID,
  FL_MEDIUMPURPLE,
  FL_MEDIUMSEAGREEN,
  FL_MEDIUMSLATEBLUE,
  FL_MEDIUMSPRINGGREEN,
  FL_MEDIUMTURQUOISE,
  FL_MEDIUMVIOLETRED,
  FL_MIDNIGHTBLUE,
  FL_MINTCREAM,
  FL_MISTYROSE,
  FL_MOCCASIN,
  FL_NAVAJOWHITE,
  FL_NAVY,
  FL_OLDLACE,
  FL_OLIVE,
  FL_OLIVEDRAB,
  FL_ORANGE,
  FL_ORANGERED,
  FL_PALEGOLDENROD,
  FL_PALETURQUOISE,
  FL_PALEVIOLETRED,
  FL_PAPAYAWHIP,
  FL_PEACHPUFF,
  FL_PERU,
  FL_PINK,
  FL_PLUM,
  FL_POWDERBLUE,
  FL_PURPLE,
  FL_ROSYBROWN,
  FL_ROYALBLUE,
  FL_SADDLEBROWN,
  FL_SALMON,
  FL_SANDYBROWN,
  FL_SEAGREEN,
  FL_SEASHELL,
  FL_SIENNA,
  FL_SILVER,
  FL_SKYBLUE,
  FL_SLATEGRAY,
  FL_SLATEGREY,
  FL_SNOW,
  FL_STEELBLUE,
  FL_TAN,
  FL_TEAL,
  FL_THISTLE,
  FL_TURQUOISE,
  FL_VIOLET,
  FL_WHITESMOKE,
  FL_YELLOWGREEN,
  FL_FREE_COL1 = 256,
  FL_FREE_COL2,
  FL_FREE_COL3,
  FL_FREE_COL4,
  FL_FREE_COL5,
  FL_FREE_COL6,
  FL_FREE_COL7,
  FL_FREE_COL8,
  FL_FREE_COL9,
  FL_FREE_COL10,
  FL_FREE_COL11,
  FL_FREE_COL12,
  FL_FREE_COL13,
  FL_FREE_COL14,
  FL_FREE_COL15,
  FL_FREE_COL16,
  FL_NOCOLOR = INT_MAX
} FL_PD_COL;

#define FL_BUILT_IN_COLS (FL_YELLOWGREEN + 1)
#define FL_INACTIVE_COL FL_INACTIVE

/* Some aliases for a number of colors */

#define FL_GRAY16 FL_RIGHT_BCOL
#define FL_GRAY35 FL_BOTTOM_BCOL
#define FL_GRAY80 FL_TOP_BCOL
#define FL_GRAY90 FL_LEFT_BCOL
#define FL_GRAY63 FL_COL1
#define FL_GRAY75 FL_MCOL
#define FL_LCOL FL_BLACK
#define FL_NoColor FL_NOCOLOR

/* An alias probably for an earlier typo */

#define FL_DOGERBLUE FL_DODGERBLUE

/* Events that a form reacts to  */

typedef enum {
  FL_NOEVENT, /*  0 No event */
  FL_DRAW, /*  1 object is asked to redraw itself */
  FL_PUSH, /*  2 mouse button was pressed on the object */
  FL_RELEASE, /*  3 mouse button was release gain */
  FL_ENTER, /*  4 mouse entered the object */
  FL_LEAVE, /*  5 mouse left the object */
  FL_MOTION, /*  6 mouse motion over the object happend */
  FL_FOCUS, /*  7 object obtained focus */
  FL_UNFOCUS, /*  8 object lost focus */
  FL_KEYPRESS, /*  9 key was pressed while object has focus */
  FL_UPDATE, /* 10 for objects that need to update something
                   from time to time */
  FL_STEP, /* 11 */
  FL_SHORTCUT, /* 12 */
  FL_FREEMEM, /* 13 object is asked to free all its memory */
  FL_OTHER, /* 14 property, selection etc */
  FL_DRAWLABEL, /* 15 */
  FL_DBLCLICK, /* 16 double click on object */
  FL_TRPLCLICK, /* 17 triple click on object */
  FL_ATTRIB, /* 18 an object attribute changed */
  FL_KEYRELEASE, /* 19 key was released while object has focus */
  FL_PS, /* 20 dump a form into EPS      */
  FL_MOVEORIGIN, /* 21 dragging the form across the screen
                       changes its absolute x,y coords. Objects
                       that themselves contain forms should
                       ensure that they are up to date. */
  FL_RESIZED, /* 22 the object has been resized by scale_form
                    Tell it that this has happened so that
                    it can resize any FL_FORMs that it
                    contains. */
  FL_PASTE, /* 23 text was pasted into input object */
  FL_TRIGGER, /* 24 result of fl_trigger_object() */

  /* The following are only for backward compatibility, not used anymore */

  FL_MOVE = FL_MOTION,
  FL_KEYBOARD = FL_KEYPRESS,
  FL_MOUSE = FL_UPDATE

} FL_EVENTS;

/* Resize policies */

typedef enum { FL_RESIZE_NONE, FL_RESIZE_X, FL_RESIZE_Y, FL_RESIZE_ALL = (FL_RESIZE_X | FL_RESIZE_Y) } FL_RESIZE_T;

/* Keyboard focus control */

typedef enum {
  FL_KEY_NORMAL = 1, /* normal keys(0-255) - tab +left/right */
  FL_KEY_TAB = 2, /* normal keys + 4 direction cursor     */
  FL_KEY_SPECIAL = 4, /* only needs special keys (>255)       */
  FL_KEY_ALL = 7 /* all keys                             */
} FL_KEY;

#define FL_ALT_MASK (1L << 25) /* alt + Key --> FL_ALT_MASK + key */
#define FL_CONTROL_MASK (1L << 26)
#define FL_SHIFT_MASK (1L << 27)
#define FL_ALT_VAL FL_ALT_MASK /* Don' use! */

#define MAX_SHORTCUTS 8

/* Pop-up menu item attributes. NOTE if more than 8, need to change
 * choice and menu class where mode is kept by a single byte */

enum { FL_PUP_NONE, FL_PUP_GREY = 1, FL_PUP_BOX = 2, FL_PUP_CHECK = 4, FL_PUP_RADIO = 8 };

#define FL_PUP_GRAY FL_PUP_GREY
#define FL_PUP_TOGGLE FL_PUP_BOX /* not used anymore */
#define FL_PUP_INACTIVE FL_PUP_GREY

/* Popup and menu entries */

typedef int (*FL_PUP_CB)(int); /* callback prototype  */

typedef struct {
  const char *text; /* label of a popup/menu item   */
  FL_PUP_CB callback; /* the callback function        */
  const char *shortcut; /* hotkeys                      */
  int mode; /* FL_PUP_GRAY, FL_PUP_CHECK etc */
} FL_PUP_ENTRY;

#define FL_MENU_ENTRY FL_PUP_ENTRY

/*******************************************************************
 * FONTS
 ******************************************************************/

#define FL_MAXFONTS 48 /* max number of fonts */

typedef enum {
  FL_INVALID_STYLE = -1,
  FL_NORMAL_STYLE,
  FL_BOLD_STYLE,
  FL_ITALIC_STYLE,
  FL_BOLDITALIC_STYLE,

  FL_FIXED_STYLE,
  FL_FIXEDBOLD_STYLE,
  FL_FIXEDITALIC_STYLE,
  FL_FIXEDBOLDITALIC_STYLE,

  FL_TIMES_STYLE,
  FL_TIMESBOLD_STYLE,
  FL_TIMESITALIC_STYLE,
  FL_TIMESBOLDITALIC_STYLE,

  FL_MISC_STYLE,
  FL_MISCBOLD_STYLE,
  FL_MISCITALIC_STYLE,
  FL_SYMBOL_STYLE,

  /* modfier masks. Need to fit a short  */

  FL_SHADOW_STYLE = (1 << 9),
  FL_ENGRAVED_STYLE = (1 << 10),
  FL_EMBOSSED_STYLE = (1 << 11)
} FL_TEXT_STYLE;

#define FL_FONT_STYLE FL_TEXT_STYLE

#define special_style(a) ((a) >= FL_SHADOW_STYLE && (a) <= (FL_EMBOSSED_STYLE + FL_MAXFONTS))

/* Standard sizes in XForms */

#define FL_TINY_SIZE 8
#define FL_SMALL_SIZE 10
#define FL_NORMAL_SIZE 12
#define FL_MEDIUM_SIZE 14
#define FL_LARGE_SIZE 18
#define FL_HUGE_SIZE 24

#define FL_DEFAULT_SIZE FL_SMALL_SIZE

/* Defines for compatibility */

#define FL_TINY_FONT FL_TINY_SIZE
#define FL_SMALL_FONT FL_SMALL_SIZE
#define FL_NORMAL_FONT FL_NORMAL_SIZE
#define FL_MEDIUM_FONT FL_MEDIUM_SIZE
#define FL_LARGE_FONT FL_LARGE_SIZE
#define FL_HUGE_FONT FL_HUGE_SIZE

#define FL_NORMAL_FONT1 FL_SMALL_FONT
#define FL_NORMAL_FONT2 FL_NORMAL_FONT
#define FL_DEFAULT_FONT FL_SMALL_FONT

#define FL_BOUND_WIDTH (FL_Coord)1 /* Border width of boxes */

/* Definition of basic struct that holds an object */

#define FL_CLICK_TIMEOUT 400 /* double click interval */

typedef struct FL_FORM_ FL_FORM;
typedef struct FL_OBJECT_ FL_OBJECT;
typedef struct FL_pixmap_ FL_pixmap;

struct FL_OBJECT_ {
  FL_FORM *form; /* the form this object belongs to */
  void *u_vdata; /* anything the user likes */
  char *u_cdata; /* anything the user likes */
  long u_ldata; /* anything the user likes */

  int objclass; /* class of object, button, slider etc */
  int type; /* type within the class */
  int boxtype; /* what kind of box type */
  FL_Coord x, /* current obj. location and size */
      y, w, h;
  double fl1, /* distances of upper left hand (1) and */
      fr1, /* lower right hand corner (2) to left, */
      ft1, /* right, top and bottom of enclosing   */
      fb1, /* form */
      fl2, fr2, ft2, fb2;
  FL_Coord bw;
  FL_COLOR col1, /* colors of obj */
      col2;
  char *label; /* object label */
  FL_COLOR lcol; /* label color */
  int align;
  int lsize, /* label size and style */
      lstyle;
  long *shortcut;
  int (*handle)(FL_OBJECT *, int, FL_Coord, FL_Coord, int, void *);
  void (*object_callback)(FL_OBJECT *, long);
  long argument;
  void *spec; /* instantiation */

  int (*prehandle)(FL_OBJECT *, int, FL_Coord, FL_Coord, int, void *);
  int (*posthandle)(FL_OBJECT *, int, FL_Coord, FL_Coord, int, void *);
  void (*set_return)(FL_OBJECT *, unsigned int);

  /* Re-configure preference */

  unsigned int resize; /* what to do if WM resizes the FORM     */
  unsigned int nwgravity; /* how to re-position top-left corner    */
  unsigned int segravity; /* how to re-position lower-right corner */

  FL_OBJECT *prev; /* prev. obj in form */
  FL_OBJECT *next; /* next. obj in form */

  FL_OBJECT *parent;
  FL_OBJECT *child;
  FL_OBJECT *nc; /* next child */

  FL_pixmap *flpixmap; /* pixmap double buffering stateinfo */
  int use_pixmap; /* true to use pixmap double buffering*/

  /* Some interaction flags */

  int returned; /* what last interaction returned */
  unsigned int how_return; /* under which conditions to return */
  int double_buffer; /* only used by mesa/gl canvas */
  int pushed;
  int focus;
  int belowmouse;
  int active; /* if object accepts events */
  int input;
  int wantkey;
  int radio;
  int automatic;
  int redraw;
  int visible;
  int is_under; /* if (partially) hidden by other object */
  int clip;
  unsigned long click_timeout;
  void *c_vdata; /* for class use */
  char *c_cdata; /* for class use */
  long c_ldata; /* for class use */
  FL_COLOR dbl_background; /* double buffer background */
  char *tooltip;
  int tipID;
  int group_id;
  int want_motion;
  int want_update;
};

/* Callback function for an entire form */

typedef void (*FL_FORMCALLBACKPTR)(FL_OBJECT *, void *);
/* Object callback function      */

typedef void (*FL_CALLBACKPTR)(FL_OBJECT *, long);

/* Preemptive callback function  */

typedef int (*FL_RAW_CALLBACK)(FL_FORM *, void *);

/* At close (WM menu delete/close etc.) function */

typedef int (*FL_FORM_ATCLOSE)(FL_FORM *, void *);
/* Deactivate/activate callback */

typedef void (*FL_FORM_ATDEACTIVATE)(FL_FORM *, void *);
typedef void (*FL_FORM_ATACTIVATE)(FL_FORM *, void *);

typedef int (*FL_HANDLEPTR)(FL_OBJECT *, int, FL_Coord, FL_Coord, int, void *);

/* Error callback */

typedef void (*FL_ERROR_FUNC)(const char *, const char *, ...);

__attribute__((unused)) static FL_OBJECT *FL_EVENT;

/*** FORM ****/

/* Form visibility state: form->visible */

enum { FL_BEING_HIDDEN = -1, FL_HIDDEN = 0, FL_INVISIBLE = FL_HIDDEN, FL_VISIBLE = 1 };

struct FL_FORM_ {
  void *fdui; /* for fdesign */
  void *u_vdata; /* for application */
  char *u_cdata; /* for application */
  long u_ldata; /* for application */

  char *label; /* window title */
  Window window; /* X resource ID for window */
  FL_Coord x, /* current geometry info */
      y, w, h;
  int handle_dec_x, handle_dec_y;
  FL_Coord hotx, /* hot-spot of the form */
      hoty;
  double w_hr, /* high resolution width and height */
      h_hr; /* (needed for precise scaling) */

  FL_OBJECT *first;
  FL_OBJECT *last;
  FL_OBJECT *focusobj;

  FL_FORMCALLBACKPTR form_callback;
  FL_FORM_ATACTIVATE activate_callback;
  FL_FORM_ATDEACTIVATE deactivate_callback;
  void *form_cb_data;
  void *activate_data;
  void *deactivate_data;

  FL_RAW_CALLBACK key_callback;
  FL_RAW_CALLBACK push_callback;
  FL_RAW_CALLBACK crossing_callback;
  FL_RAW_CALLBACK motion_callback;
  FL_RAW_CALLBACK all_callback;

  unsigned long compress_mask;
  unsigned long evmask;

  /* WM_DELETE_WINDOW message handler */

  FL_FORM_ATCLOSE close_callback;
  void *close_data;

  FL_pixmap *flpixmap; /* back buffer */

  Pixmap icon_pixmap;
  Pixmap icon_mask;

  /* Interaction and other flags */

  int deactivated; /* non-zero if deactivated */
  int use_pixmap; /* true if dbl buffering */
  int frozen; /* true if sync change */
  int visible; /* true if mapped */
  int wm_border; /* window manager info */
  unsigned int prop; /* other attributes */
  int num_auto_objects;
  int needs_full_redraw;
  int sort_of_modal; /* internal use */
  FL_FORM *parent;
  FL_FORM *child;
  FL_OBJECT *parent_obj;
  int attached; /* not independent anymore */
  void (*pre_attach)(FL_FORM *);
  void *attach_data;
  int in_redraw;
};

/* All FD_xxx structure emitted by fdesign contains at least the
 * following */

typedef struct {
  FL_FORM *form;
  void *vdata;
  char *cdata;
  long ldata;
} FD_Any;

/* Async IO stuff */

enum { FL_READ = 1, FL_WRITE = 2, FL_EXCEPT = 4 };

/* IO other than XEvent Q */

typedef void (*FL_IO_CALLBACK)(int, void *);

FL_EXPORT void fl_add_io_callback(int fd, unsigned int mask, FL_IO_CALLBACK callback, void *data){};

FL_EXPORT void fl_remove_io_callback(int fd, unsigned int mask, FL_IO_CALLBACK cb){};

/* signals */

typedef void (*FL_SIGNAL_HANDLER)(int, void *);

FL_EXPORT void fl_add_signal_callback(int s, FL_SIGNAL_HANDLER cb, void *data){};

FL_EXPORT void fl_remove_signal_callback(int s){};

FL_EXPORT void fl_signal_caught(int s){};

FL_EXPORT void fl_app_signal_direct(int y){};

enum { FL_INPUT_END_EVENT_CLASSIC = 0, FL_INPUT_END_EVENT_ALWAYS = 1 };

FL_EXPORT int fl_input_end_return_handling(int type)
{
  return 0;
};



/** Generic routines that deal with FORMS **/

FL_EXPORT FL_FORM *fl_bgn_form(int type, FL_Coord w, FL_Coord h)
{
  return NULL;
};

FL_EXPORT void fl_end_form(void){};

FL_EXPORT FL_OBJECT *fl_do_forms(void)
{
  return NULL;
};

FL_EXPORT FL_OBJECT *fl_check_forms(void)
{
  return NULL;
};

FL_EXPORT FL_OBJECT *fl_do_only_forms(void)
{
  return NULL;
};

FL_EXPORT FL_OBJECT *fl_check_only_forms(void)
{
  return NULL;
};

FL_EXPORT void fl_freeze_form(FL_FORM *form){};

FL_EXPORT void fl_set_focus_object(FL_FORM *form, FL_OBJECT *obj){};

FL_EXPORT FL_OBJECT *fl_get_focus_object(FL_FORM *form)
{
  return NULL;
};

FL_EXPORT void fl_reset_focus_object(FL_OBJECT *ob){};

#define fl_set_object_focus fl_set_focus_object

FL_EXPORT FL_FORM_ATCLOSE fl_set_form_atclose(FL_FORM *form, FL_FORM_ATCLOSE fmclose, void *data){};

FL_EXPORT FL_FORM_ATCLOSE fl_set_atclose(FL_FORM_ATCLOSE fmclose, void *data){};

FL_EXPORT FL_FORM_ATACTIVATE fl_set_form_atactivate(FL_FORM *form, FL_FORM_ATACTIVATE cb, void *data){};

FL_EXPORT FL_FORM_ATDEACTIVATE fl_set_form_atdeactivate(FL_FORM *form, FL_FORM_ATDEACTIVATE cb, void *data){};

FL_EXPORT void fl_unfreeze_form(FL_FORM *form){};

FL_EXPORT int fl_form_is_activated(FL_FORM *form){};

FL_EXPORT void fl_deactivate_form(FL_FORM *form){};

FL_EXPORT void fl_activate_form(FL_FORM *form){};

FL_EXPORT void fl_deactivate_all_forms(void){};

FL_EXPORT void fl_activate_all_forms(void){};

FL_EXPORT void fl_freeze_all_forms(void){};

FL_EXPORT void fl_unfreeze_all_forms(void){};

FL_EXPORT void fl_scale_form(FL_FORM *form, double xsc, double ysc){};

FL_EXPORT void fl_set_form_position(FL_FORM *form, FL_Coord x, FL_Coord y){};

FL_EXPORT void fl_set_form_title(FL_FORM *form, const char *name){};

FL_EXPORT void fl_set_form_title_f(FL_FORM *form, const char *fmt, ...){};

FL_EXPORT void fl_set_app_mainform(FL_FORM *form){};

FL_EXPORT FL_FORM *fl_get_app_mainform(void)
{
  return NULL;
};

FL_EXPORT void fl_set_app_nomainform(int flag){};

FL_EXPORT void fl_set_form_callback(FL_FORM *form, FL_FORMCALLBACKPTR callback, void *d){};

#define fl_set_form_call_back fl_set_form_callback

FL_EXPORT void fl_set_form_size(FL_FORM *form, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_set_form_background_color(FL_FORM *form, FL_COLOR color){};

FL_EXPORT FL_COLOR fl_get_form_background_color(FL_FORM *form){};

FL_EXPORT void fl_set_form_hotspot(FL_FORM *form, FL_Coord x, FL_Coord y){};

FL_EXPORT void fl_set_form_hotobject(FL_FORM *form, FL_OBJECT *ob){};

FL_EXPORT void fl_set_form_minsize(FL_FORM *form, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_set_form_maxsize(FL_FORM *form, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_set_form_event_cmask(FL_FORM *form, unsigned long cmask){};

FL_EXPORT unsigned long fl_get_form_event_cmask(FL_FORM *form){};

FL_EXPORT void fl_set_form_geometry(FL_FORM *form, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

#define fl_set_initial_placement fl_set_form_geometry

FL_EXPORT Window fl_show_form(FL_FORM *form, int place, int border, const char *name)
{
  return NULL;
};

FL_EXPORT Window fl_show_form_f(FL_FORM *form, int place, int border, const char *fmt, ...)
{
  return NULL;
};

FL_EXPORT void fl_hide_form(FL_FORM *form){};

FL_EXPORT void fl_free_form(FL_FORM *form){};

FL_EXPORT void fl_redraw_form(FL_FORM *form){};

FL_EXPORT void fl_set_form_dblbuffer(FL_FORM *form, int y){};

FL_EXPORT Window fl_prepare_form_window(FL_FORM *form, int place, int border, const char *name)
{
  return NULL;
};

FL_EXPORT Window fl_prepare_form_window_f(FL_FORM *form, int place, int border, const char *fmt, ...)
{
  return NULL;
};

FL_EXPORT Window fl_show_form_window(FL_FORM *form)
{
  return NULL;
};

FL_EXPORT double fl_adjust_form_size(FL_FORM *form)
{
  return 0;
};

FL_EXPORT int fl_form_is_visible(FL_FORM *form)
{
  return 0;
};

FL_EXPORT int fl_form_is_iconified(FL_FORM *form)
{
  return 0;
};

FL_EXPORT FL_RAW_CALLBACK fl_register_raw_callback(FL_FORM *form, unsigned long mask, FL_RAW_CALLBACK rcb){};

#define fl_register_call_back fl_register_raw_callback

FL_EXPORT FL_OBJECT *fl_bgn_group(void)
{
  return NULL;
};

FL_EXPORT void fl_end_group(void){};

FL_EXPORT FL_OBJECT *fl_addto_group(FL_OBJECT *group)
{
  return NULL;
};

/****** Routines that deal with FL_OBJECTS ********/

FL_EXPORT int fl_get_object_objclass(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT int fl_get_object_type(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_set_object_boxtype(FL_OBJECT *ob, int boxtype){};

FL_EXPORT int fl_get_object_boxtype(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_set_object_bw(FL_OBJECT *ob, int bw){};

FL_EXPORT int fl_get_object_bw(FL_OBJECT *ob)
{
  return 0;
};

FL_EXPORT void fl_set_object_resize(FL_OBJECT *ob, unsigned int what){};

FL_EXPORT void fl_get_object_resize(FL_OBJECT *ob, unsigned int *what){};

FL_EXPORT void fl_set_object_gravity(FL_OBJECT *ob, unsigned int nw, unsigned int se){};

FL_EXPORT void fl_get_object_gravity(FL_OBJECT *ob, unsigned int *nw, unsigned int *se){};

FL_EXPORT void fl_set_object_lsize(FL_OBJECT *obj, int lsize){};

FL_EXPORT int fl_get_object_lsize(FL_OBJECT *obj){};

FL_EXPORT void fl_set_object_lstyle(FL_OBJECT *obj, int lstyle){};

FL_EXPORT int fl_get_object_lstyle(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_set_object_lcol(FL_OBJECT *ob, FL_COLOR lcol){};

FL_EXPORT FL_COLOR fl_get_object_lcol(FL_OBJECT *obj){};

FL_EXPORT int fl_set_object_return(FL_OBJECT *ob, unsigned int when){};

FL_EXPORT void fl_set_object_lalign(FL_OBJECT *obj, int align){};

FL_EXPORT int fl_get_object_lalign(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_set_object_shortcut(FL_OBJECT *obj, const char *sstr, int showit){};

FL_EXPORT void fl_set_object_shortcutkey(FL_OBJECT *obj, unsigned int keysym){};

FL_EXPORT void fl_set_object_dblbuffer(FL_OBJECT *ob, int y){};

FL_EXPORT void fl_set_object_color(FL_OBJECT *ob, FL_COLOR col1, FL_COLOR col2){};

FL_EXPORT void fl_get_object_color(FL_OBJECT *obj, FL_COLOR *col1, FL_COLOR *col2){};

FL_EXPORT void fl_set_object_label(FL_OBJECT *ob, const char *label){};

FL_EXPORT void fl_set_object_label_f(FL_OBJECT *obj, const char *fmt, ...){};

FL_EXPORT const char *fl_get_object_label(FL_OBJECT *obj)
{
  return NULL;
};

FL_EXPORT void fl_set_object_helper(FL_OBJECT *ob, const char *tip){};

FL_EXPORT void fl_set_object_helper_f(FL_OBJECT *ob, const char *fmt, ...){};

FL_EXPORT void fl_set_object_position(FL_OBJECT *obj, FL_Coord x, FL_Coord y){};

FL_EXPORT void fl_get_object_size(FL_OBJECT *obj, FL_Coord *w, FL_Coord *h){};

FL_EXPORT void fl_set_object_size(FL_OBJECT *obj, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_set_object_automatic(FL_OBJECT *obj, int flag){};

FL_EXPORT int fl_object_is_automatic(FL_OBJECT *obj){};

FL_EXPORT void fl_draw_object_label(FL_OBJECT *ob){};

FL_EXPORT void fl_draw_object_label_outside(FL_OBJECT *ob){};

FL_EXPORT FL_OBJECT *fl_get_object_component(FL_OBJECT *composite, int objclass, int type, int numb)
{
  return NULL;
};

FL_EXPORT void fl_for_all_objects(FL_FORM *form, int (*cb)(FL_OBJECT *, void *), void *v){};

#define fl_draw_object_outside_label fl_draw_object_label_outside

FL_EXPORT void fl_set_object_dblclick(FL_OBJECT *obj, unsigned long timeout){};

FL_EXPORT unsigned long fl_get_object_dblclick(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_set_object_geometry(FL_OBJECT *obj, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_move_object(FL_OBJECT *obj, FL_Coord dx, FL_Coord dy){};

#define fl_set_object_lcolor fl_set_object_lcol
#define fl_get_object_lcolor fl_get_object_lcol

FL_EXPORT void fl_fit_object_label(FL_OBJECT *obj, FL_Coord xmargin, FL_Coord ymargin){};

FL_EXPORT void fl_get_object_geometry(FL_OBJECT *ob, FL_Coord *x, FL_Coord *y, FL_Coord *w, FL_Coord *h){};

FL_EXPORT void fl_get_object_position(FL_OBJECT *ob, FL_Coord *x, FL_Coord *y){};

/* This one takes into account the label */

FL_EXPORT void fl_get_object_bbox(FL_OBJECT *obj, FL_Coord *x, FL_Coord *y, FL_Coord *w, FL_Coord *h)
{
  *x = *y = *w = *h = 0;
};

#define fl_compute_object_geometry fl_get_object_bbox

FL_EXPORT void fl_call_object_callback(FL_OBJECT *ob){};

FL_EXPORT FL_HANDLEPTR fl_set_object_prehandler(FL_OBJECT *ob, FL_HANDLEPTR phandler){};

FL_EXPORT FL_HANDLEPTR fl_set_object_posthandler(FL_OBJECT *ob, FL_HANDLEPTR post){};

FL_EXPORT FL_CALLBACKPTR fl_set_object_callback(FL_OBJECT *obj, FL_CALLBACKPTR callback, long argument){};

#define fl_set_object_align fl_set_object_lalign
#define fl_set_call_back fl_set_object_callback

FL_EXPORT void fl_redraw_object(FL_OBJECT *obj){};

FL_EXPORT void fl_show_object(FL_OBJECT *ob){};

FL_EXPORT void fl_hide_object(FL_OBJECT *ob){};

FL_EXPORT int fl_object_is_visible(FL_OBJECT *obj){};

FL_EXPORT void fl_free_object(FL_OBJECT *obj){};

FL_EXPORT void fl_delete_object(FL_OBJECT *obj){};

FL_EXPORT int fl_get_object_return_state(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_trigger_object(FL_OBJECT *obj){};

FL_EXPORT void fl_activate_object(FL_OBJECT *ob){};

FL_EXPORT void fl_deactivate_object(FL_OBJECT *ob){};

FL_EXPORT int fl_object_is_active(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT int fl_enumerate_fonts(void (*output)(const char *s), int shortform)
{
  return 0;
};

FL_EXPORT int fl_set_font_name(int n, const char *name)
{
  return 0;
};

FL_EXPORT int fl_set_font_name_f(int n, const char *fmt, ...)
{
  return 0;
};

FL_EXPORT void fl_set_font(int numb, int size){};

/* Routines that facilitate free object */

FL_EXPORT int fl_get_char_height(int style, int size, int *asc, int *desc)
{
  return 0;
};

FL_EXPORT int fl_get_char_width(int style, int size)
{
  return 0;
};

FL_EXPORT int fl_get_string_height(int style, int size, const char *s, int len, int *asc, int *desc)
{
  return 0;
};

FL_EXPORT int fl_get_string_width(int style, int size, const char *s, int len)
{
  return 0;
};

FL_EXPORT int fl_get_string_widthTAB(int style, int size, const char *s, int len)
{
  return 0;
};

FL_EXPORT void fl_get_string_dimension(int fntstyle, int fntsize, const char *s, int len, int *width, int *height){};

#define fl_get_string_size fl_get_string_dimension

FL_EXPORT void fl_get_align_xy(int align, int x, int y, int w, int h, int xsize, int ysize, int xoff, int yoff, int *xx, int *yy){};

FL_EXPORT int fl_get_label_char_at_mouse(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_drw_text(int align, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR c, int style, int size, const char *istr){};

FL_EXPORT void fl_drw_text_beside(int align, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR c, int style, int size, const char *str){};

FL_EXPORT void fl_drw_text_cursor(int align, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR c, int style, int size, const char *str, int cc, int pos){};

FL_EXPORT void fl_drw_box(int style, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR c, int bw_in){};

typedef void (*FL_DRAWPTR)(FL_Coord, FL_Coord, FL_Coord, FL_Coord, int, FL_COLOR);

FL_EXPORT int fl_add_symbol(const char *name, FL_DRAWPTR drawit, int scalable)
{
  return 0;
};

FL_EXPORT int fl_delete_symbol(const char *name)
{
  return 0;
};

FL_EXPORT int fl_draw_symbol(const char *label, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR col)
{
  return 0;
};

FL_EXPORT unsigned long fl_mapcolor(FL_COLOR col, int r, int g, int b)
{
  return 0;
};

FL_EXPORT long fl_mapcolorname(FL_COLOR col, const char *name)
{
  return 0;
};

#define fl_mapcolor_name fl_mapcolorname

FL_EXPORT void fl_free_colors(FL_COLOR *c, int n){};

FL_EXPORT void fl_free_pixels(unsigned long *pix, int n){};

FL_EXPORT void fl_set_color_leak(int y){};

FL_EXPORT unsigned long fl_getmcolor(FL_COLOR i, int *r, int *g, int *b)
{
  return 0;
};

FL_EXPORT unsigned long fl_get_pixel(FL_COLOR col)
{
  return 0;
};

#define fl_get_flcolor fl_get_pixel

FL_EXPORT void fl_get_icm_color(FL_COLOR col, int *r, int *g, int *b){};

FL_EXPORT void fl_set_icm_color(FL_COLOR col, int r, int g, int b){};

FL_EXPORT void fl_color(FL_COLOR col){};

FL_EXPORT void fl_bk_color(FL_COLOR col){};

FL_EXPORT void fl_set_gamma(double r, double g, double b){};

FL_EXPORT void fl_show_errors(int y){};

/* Some macros */

#define FL_max(a, b) ((a) > (b) ? (a) : (b))
#define FL_min(a, b) ((a) < (b) ? (a) : (b))
#define FL_abs(a) ((a) > 0 ? (a) : (-(a)))
#define FL_nint(a) ((int)((a) > 0 ? ((a) + 0.5) : ((a)-0.5)))
#define FL_clamp(a, amin, amax) ((a) < (amin) ? (amin) : ((a) > (amax) ? (amax) : (a)))
#define FL_crnd(a) ((FL_Coord)((a) > 0 ? ((a) + 0.5) : ((a)-0.5)))

typedef int (*FL_FSCB)(const char *, void *);

/* Utilities for new objects */

__attribute__((unused)) static FL_FORM *fl_current_form;

FL_EXPORT void fl_add_object(FL_FORM *form, FL_OBJECT *obj){};

FL_EXPORT FL_FORM *fl_addto_form(FL_FORM *form)
{
  return NULL;
};

FL_EXPORT FL_OBJECT *fl_make_object(int objclass, int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label, FL_HANDLEPTR handle)
{
  return NULL;
};

FL_EXPORT void fl_add_child(FL_OBJECT *oa, FL_OBJECT *ob){};

FL_EXPORT void fl_set_coordunit(int u){};

FL_EXPORT void fl_set_border_width(int bw){};

FL_EXPORT void fl_set_scrollbar_type(int t){};

#define fl_set_thinscrollbar(t) fl_set_scrollbar_type(t ? FL_THIN_SCROLLBAR : FL_NORMAL_SCROLLBAR)

FL_EXPORT void fl_flip_yorigin(void){};

FL_EXPORT int fl_get_coordunit(void)
{
  return 0;
};

FL_EXPORT int fl_get_border_width(void)
{
  return 0;
};

/* Misc. routines */

FL_EXPORT void fl_ringbell(int percent){};

FL_EXPORT void fl_gettime(long *sec, long *usec){};

FL_EXPORT const char *fl_now(void)
{
  return NULL;
};

FL_EXPORT const char *fl_whoami(void)
{
  return NULL;
};

FL_EXPORT long fl_mouse_button(void)
{
  return 0;
};

FL_EXPORT int fl_current_event(void)
{
  return 0;
};

FL_EXPORT char *fl_strdup(const char *s)
{
  return NULL;
};

FL_EXPORT void fl_set_err_logfp(FILE *fp){};

FL_EXPORT void fl_set_error_handler(FL_ERROR_FUNC user_func){};

FL_EXPORT char **fl_get_cmdline_args(int *ia)
{
  return NULL;
};

  /* This function was called 'fl_set_error_logfp/' in XForms 0.89. */

#define fl_set_error_logfp fl_set_err_logfp

#define fl_mousebutton fl_mouse_button

/* These give more flexibility for future changes. Also application
 * can re-assign these pointers to whatever function it wants, e.g.,
 * to a shared memory pool allocator. */
/*
FL_EXPORT void ( * fl_free )( void * ){};

FL_EXPORT void * ( * fl_malloc )( size_t ){};

FL_EXPORT void * ( * fl_calloc )( size_t,
                                  size_t ){};

FL_EXPORT void * ( * fl_realloc )( void *,
                                   size_t ){};
*/
FL_EXPORT int fl_msleep(unsigned long msec)
{
  return 0;
};

#define FL_MAX_MENU_CHOICE_ITEMS 128

typedef const char *(*FL_VAL_FILTER)(FL_OBJECT *, double, int);

FL_EXPORT int fl_is_same_object(FL_OBJECT *obj1, FL_OBJECT *obj2)
{
  return 0;
};

#endif /* ! defined FL_BASIC_H */

  /**
   * \file XBasic.h
   *
   *  X Window dependent stuff
   *
   */

#ifndef FL_XBASIC_H
#define FL_XBASIC_H

/* Draw mode */

enum { FL_XOR = 0, FL_COPY = 1, FL_AND = 2 };

#define FL_MINDEPTH 1

/* FL_xxx does not do anything anymore, but kept for compatibility */

enum {
  FL_IllegalVisual = -1,
  FL_StaticGray = 0,
  FL_GrayScale = 1,
  FL_StaticColor = 2,
  FL_PseudoColor = 3,
  FL_TrueColor = 4,
  FL_DirectColor = 5,
  FL_DefaultVisual = 10 /* special request */
};

enum { FL_North = 0, FL_NorthEast, FL_NorthWest, FL_South, FL_SouthEast, FL_SouthWest, FL_East, FL_West, FL_NoGravity, FL_ForgetGravity };

#ifndef GreyScale
#define GreyScale GrayScale
#define StaticGrey StaticGray
#endif

#define FL_is_gray(v) ((v) == GrayScale || (v) == StaticGray)
#define FL_is_rgb(v) ((v) == TrueColor || (v) == DirectColor)

/* Internal colormap size. Not really very meaningful as fl_mapcolor
 * and company allow color "leakage", that is, although only FL_MAX_COLS
 * are kept in the internal colormap, the server might have substantially
 * more colors allocated */

#define FL_MAX_COLORS 1024
#define FL_MAX_COLS FL_MAX_COLORS

/* FL graphics state information. Some are redundant. */

typedef struct {
  //    XVisualInfo   * xvinfo;
  //    XFontStruct   * cur_fnt;            /* current font in default GC */
  //    Colormap        colormap;           /* colormap valid for xvinfo */
  Window trailblazer; /* a valid window for xvinfo */
  int vclass, /* visual class and color depth */
      depth;
  int rgb_bits; /* primary color resolution */
  int dithered; /* true if dithered color */
  int pcm; /* true if colormap is not shared */
  //    GC              gc[ 16 ];           /* working GC */
  //    GC              textgc[ 16 ];       /* GC used exclusively for text */
  //    GC              dimmedGC;           /* A GC having a checkboard stipple */
  unsigned long lut[FL_MAX_COLS]; /* secondary lookup table */
  unsigned int rshift, rmask, rbits;
  unsigned int gshift, gmask, gbits;
  unsigned int bshift, bmask, bbits;
} FL_State;

#define FL_STATE FL_State /* for compatibility */

/***** Global variables ******/

// static Display *fl_display;

__attribute__((unused)) static int fl_screen;

__attribute__((unused)) static Window fl_root; /* root window */
__attribute__((unused)) static Window fl_vroot; /* virtual root window */
__attribute__((unused)) static int fl_scrh, /* screen dimension in pixels */
    fl_scrw;
__attribute__((unused)) static int fl_vmode;

/* Current version only runs in single visual mode */

#define fl_get_vclass() fl_vmode
#define fl_get_form_vclass(a) fl_vmode
#define fl_get_gc() fl_state[fl_vmode].gc[0]

//__attribute__((unused)) static FL_State fl_state[];

__attribute__((unused)) static char *fl_ul_magic_char;

FL_EXPORT int fl_mode_capable(int mode, int warn)
{
  return 0;
};

#define fl_default_win() (fl_state[fl_vmode].trailblazer)
#define fl_default_window() (fl_state[fl_vmode].trailblazer)

/* All pixmaps used by FL_OBJECT to simulate double buffering have the
 * following entries in the structure. FL_Coord x,y are used to shift
 * the origin of the drawing routines */

struct FL_pixmap_ {
  Pixmap pixmap;
  Window win;
  Visual *visual;
  FL_Coord x, y, w, h;
  int depth;
  FL_COLOR dbl_background;
  FL_COLOR pixel;
};

/* Fonts related */

#define FL_MAX_FONTSIZES 10
#define FL_MAX_FONTNAME_LENGTH 80

typedef struct {
  XFontStruct *fs[FL_MAX_FONTSIZES]; /* cached fontstruct */
  short size[FL_MAX_FONTSIZES]; /* cached sizes */
  short nsize; /* cached so far */
  char fname[FL_MAX_FONTNAME_LENGTH + 1]; /* without size info */
} FL_FONT;

/* Some basic drawing routines */

typedef XPoint FL_POINT;
typedef XRectangle FL_RECT;

/* Rectangles */

FL_EXPORT void fl_rectangle(int fill, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR col){};

FL_EXPORT void fl_rectbound(FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR col){};

#define fl_rectf(x, y, w, h, c) fl_rectangle(1, x, y, w, h, c)
#define fl_rect(x, y, w, h, c) fl_rectangle(0, x, y, w, h, c)

/* Rectangle with rounded-corners */

FL_EXPORT void fl_roundrectangle(int fill, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, FL_COLOR col){};

#define fl_roundrectf(x, y, w, h, c) fl_roundrectangle(1, x, y, w, h, c)
#define fl_roundrect(x, y, w, h, c) fl_roundrectangle(0, x, y, w, h, c)

/* General polygon and polylines */

FL_EXPORT void fl_polygon(int fill, FL_POINT *xp, int n, FL_COLOR col){};

#define fl_polyf(p, n, c) fl_polygon(1, p, n, c)
#define fl_polyl(p, n, c) fl_polygon(0, p, n, c)

#define fl_polybound(p, n, c)      \
  do {                             \
    fl_polygon(1, p, n, c);        \
    fl_polygon(0, p, n, FL_BLACK); \
  } while (0)

FL_EXPORT void fl_lines(FL_POINT *xp, int n, FL_COLOR col){};

FL_EXPORT void fl_line(FL_Coord xi, FL_Coord yi, FL_Coord xf, FL_Coord yf, FL_COLOR c){};

FL_EXPORT void fl_point(FL_Coord x, FL_Coord y, FL_COLOR c){};

FL_EXPORT void fl_points(FL_POINT *p, int np, FL_COLOR c){};

#define fl_simple_line fl_line

FL_EXPORT void fl_dashedlinestyle(const char *dash, int ndash){};

FL_EXPORT void fl_update_display(int block){};

#define fl_diagline(x, y, w, h, c) fl_line(x, y, (x) + (w)-1, (y) + (h)-1, c)

/* Line attributes */

enum { FL_SOLID = LineSolid, FL_USERDASH = LineOnOffDash, FL_USERDOUBLEDASH = LineDoubleDash, FL_DOT, FL_DOTDASH, FL_DASH, FL_LONGDASH };

FL_EXPORT void fl_linewidth(int n){};

FL_EXPORT void fl_linestyle(int n){};

FL_EXPORT void fl_drawmode(int request){};

FL_EXPORT int fl_get_linewidth(void)
{
  return 0;
};

FL_EXPORT int fl_get_linestyle(void)
{
  return 0;
};

FL_EXPORT int fl_get_drawmode(void)
{
  return 0;
};

#define fl_set_linewidth fl_linewidth
#define fl_set_linestyle fl_linestyle
#define fl_set_drawmode fl_drawmode

/*
 * Interfaces
 */

FL_EXPORT XFontStruct *fl_get_fontstruct(int style, int size){};

#define fl_get_font_struct fl_get_fontstruct
#define fl_get_fntstruct fl_get_font_struct

FL_EXPORT Window fl_get_mouse(FL_Coord *x, FL_Coord *y, unsigned int *keymask)
{
  return NULL;
};

FL_EXPORT void fl_set_mouse(FL_Coord mx, FL_Coord my){};

FL_EXPORT Window fl_get_win_mouse(Window win, FL_Coord *x, FL_Coord *y, unsigned int *keymask)
{
  return NULL;
};

FL_EXPORT Window fl_get_form_mouse(FL_FORM *fm, FL_Coord *x, FL_Coord *y, unsigned int *keymask)
{
  return NULL;
};

FL_EXPORT FL_FORM *fl_win_to_form(Window win)
{
  return NULL;
};

FL_EXPORT void fl_set_form_icon(FL_FORM *form, Pixmap p, Pixmap m){};

FL_EXPORT int fl_get_decoration_sizes(FL_FORM *form, int *top, int *right, int *bottom, int *left)
{
  return 0;
};

FL_EXPORT void fl_raise_form(FL_FORM *form){};

FL_EXPORT void fl_lower_form(FL_FORM *form){};

FL_EXPORT void fl_set_foreground(GC gc, FL_COLOR color){};
FL_EXPORT void fl_set_background(GC gc, FL_COLOR color){};

/* General windowing support */

FL_EXPORT Window fl_wincreate(const char *label)
{
  return NULL;
};

FL_EXPORT Window fl_winshow(Window win)
{
  return NULL;
};

FL_EXPORT Window fl_winopen(const char *label)
{
  return NULL;
};

FL_EXPORT void fl_winhide(Window win){};

FL_EXPORT void fl_winclose(Window win){};

FL_EXPORT void fl_winset(Window win){};

FL_EXPORT int fl_winreparent(Window win, Window new_parent)
{
  return 0;
};

FL_EXPORT void fl_winfocus(Window win){};

FL_EXPORT Window fl_winget(void)
{
  return NULL;
};

FL_EXPORT int fl_iconify(Window win)
{
  return 0;
};

FL_EXPORT void fl_winresize(Window win, FL_Coord neww, FL_Coord newh){};

FL_EXPORT void fl_winmove(Window win, FL_Coord dx, FL_Coord dy){};

FL_EXPORT void fl_winreshape(Window win, FL_Coord dx, FL_Coord dy, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_winicon(Window win, Pixmap p, Pixmap m){};

FL_EXPORT void fl_winbackground(Window win, unsigned long bk){};

FL_EXPORT void fl_winstepsize(Window win, FL_Coord dx, FL_Coord dy){};

FL_EXPORT int fl_winisvalid(Window win)
{
  return 0;
};

FL_EXPORT void fl_wintitle(Window win, const char *title){};

FL_EXPORT void fl_wintitle_f(Window win, const char *fmt, ...){};

FL_EXPORT void fl_winicontitle(Window win, const char *title){};

FL_EXPORT void fl_winicontitle_f(Window win, const char *fmt, ...){};

FL_EXPORT void fl_winposition(FL_Coord x, FL_Coord y){};

#define fl_pref_winposition fl_winposition
#define fl_win_background fl_winbackground
#define fl_winstepunit fl_winstepsize
#define fl_set_winstepunit fl_winstepsize

FL_EXPORT void fl_winminsize(Window win, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_winmaxsize(Window win, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_winaspect(Window win, FL_Coord x, FL_Coord y){};

FL_EXPORT void fl_reset_winconstraints(Window win){};

FL_EXPORT void fl_winsize(FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_initial_winsize(FL_Coord w, FL_Coord h){};

#define fl_pref_winsize fl_winsize

FL_EXPORT void fl_initial_winstate(int state){};

FL_EXPORT Colormap fl_create_colormap(XVisualInfo *xv, int nfill){};

FL_EXPORT void fl_wingeometry(FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

#define fl_pref_wingeometry fl_wingeometry

FL_EXPORT void fl_initial_wingeometry(FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_noborder(void){};

FL_EXPORT void fl_transient(void){};

FL_EXPORT void fl_get_winsize(Window win, FL_Coord *w, FL_Coord *h){};

FL_EXPORT void fl_get_winorigin(Window win, FL_Coord *x, FL_Coord *y){};

FL_EXPORT void fl_get_wingeometry(Window win, FL_Coord *x, FL_Coord *y, FL_Coord *w, FL_Coord *h){};

/* For compatibility */

#define fl_get_win_size fl_get_winsize
#define fl_get_win_origin fl_get_winorigin
#define fl_get_win_geometry fl_get_wingeometry
#define fl_initial_winposition fl_pref_winposition

#define fl_get_display() fl_display
#define FL_FormDisplay(form) fl_display
#define FL_ObjectDisplay(object) fl_display
#define FL_IS_CANVAS(o) ((o)->objclass == FL_CANVAS || (o)->objclass == FL_GLCANVAS)

/* The window an object belongs to - for drawing */

#define FL_ObjWin(o) (FL_IS_CANVAS(o) ? fl_get_canvas_id(o) : (o)->form->window)

FL_EXPORT Window fl_get_real_object_window(FL_OBJECT *ob){};

#define FL_OBJECT_WID FL_ObjWin

/*  All registerable events, including Client Message */

#define FL_ALL_EVENT (KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | ButtonMotionMask | PointerMotionMask)

/* Replacements for X functions that access the event queue */

FL_EXPORT int fl_XNextEvent(XEvent *xev)
{
  return 0;
};

FL_EXPORT int fl_XPeekEvent(XEvent *xev)
{
  return 0;
};

FL_EXPORT int fl_XEventsQueued(int mode)
{
  return 0;
};

FL_EXPORT void fl_XPutBackEvent(XEvent *xev){};

FL_EXPORT const XEvent *fl_last_event(void)
{
  return NULL;
};

typedef int (*FL_APPEVENT_CB)(XEvent *, void *);

FL_EXPORT FL_APPEVENT_CB fl_set_event_callback(FL_APPEVENT_CB callback, void *user_data){};

FL_EXPORT FL_APPEVENT_CB fl_set_idle_callback(FL_APPEVENT_CB callback, void *user_data){};

FL_EXPORT long fl_addto_selected_xevent(Window win, long mask)
{
  return 0;
};

FL_EXPORT long fl_remove_selected_xevent(Window win, long mask)
{
  return 0;
};

#define fl_add_selected_xevent fl_addto_selected_xevent

FL_EXPORT void fl_set_idle_delta(long delta){};

FL_EXPORT FL_APPEVENT_CB fl_add_event_callback(Window win, int ev, FL_APPEVENT_CB wincb, void *user_data){};

FL_EXPORT void fl_remove_event_callback(Window win, int ev){};

FL_EXPORT void fl_activate_event_callbacks(Window win){};

FL_EXPORT XEvent *fl_print_xevent_name(const char *where, const XEvent *xev){};

FL_EXPORT void fl_XFlush(void){};

#define metakey_down(mask) ((mask)&Mod1Mask)
#define shiftkey_down(mask) ((mask)&ShiftMask)
#define controlkey_down(mask) ((mask)&ControlMask)
#define button_down(mask) (((mask)&Button1Mask) || ((mask)&Button2Mask) || ((mask)&Button3Mask) || ((mask)&Button4Mask) || ((mask)&Button5Mask))

#define fl_keypressed fl_keysym_pressed

/****************** Resources ***************/

typedef enum { FL_NONE, FL_SHORT = 10, FL_BOOL, FL_INT, FL_LONG, FL_FLOAT, FL_STRING } FL_RTYPE;

typedef struct {
  const char *res_name; /* resource name                        */
  const char *res_class; /* resource class                       */
  FL_RTYPE type; /* FL_INT, FL_FLOAT, FL_BOOL, FL_STRING */
  void *var; /* address for the variable             */
  const char *defval; /* default setting in string form       */
  int nbytes; /* used only for strings                */
} FL_RESOURCE;

#define FL_resource FL_RESOURCE

#define FL_CMD_OPT XrmOptionDescRec

FL_EXPORT Display *fl_initialize(int *na, char *arg[], const char *appclass, FL_CMD_OPT *appopt, int nappopt)
{
  return NULL;
};

FL_EXPORT void fl_finish(void){};

FL_EXPORT const char *fl_get_resource(const char *rname, const char *cname, FL_RTYPE dtype, const char *defval, void *val, int size)
{
  return NULL;
};

FL_EXPORT void fl_set_resource(const char *str, const char *val){};

FL_EXPORT void fl_get_app_resources(FL_RESOURCE *appresource, int n){};

FL_EXPORT void fl_set_visualID(long id){};

FL_EXPORT int fl_keysym_pressed(KeySym k)
{
  return 0;
};

#define buttonLabelSize buttonFontSize
#define sliderLabelSize sliderFontSize
#define inputLabelSize inputFontSize

/* All Form control variables. Named closely as its resource name */

typedef struct {
  float rgamma, ggamma, bgamma;
  int debug, sync;
  int depth, vclass, doubleBuffer;
  int ulPropWidth, /* underline stuff       */
      ulThickness;
  int buttonFontSize;
  int sliderFontSize;
  int inputFontSize;
  int browserFontSize;
  int menuFontSize;
  int choiceFontSize;
  int labelFontSize; /* all other labels fonts */
  int pupFontSize, /* font for pop-up menus  */
      pupFontStyle;
  int privateColormap;
  int sharedColormap;
  int standardColormap;
  int scrollbarType;
  int backingStore;
  int coordUnit;
  int borderWidth;
  int safe;
  char *rgbfile; /* where RGB file is, not used */
  char vname[24];
} FL_IOPT;

#define FL_PDButtonLabelSize FL_PDButtonFontSize
#define FL_PDSliderLabelSize FL_PDSliderFontSize
#define FL_PDInputLabelSize FL_PDInputFontSize

/* Program default masks */

enum {
  FL_PDDepth = (1 << 1),
  FL_PDClass = (1 << 2),
  FL_PDDouble = (1 << 3),
  FL_PDSync = (1 << 4),
  FL_PDPrivateMap = (1 << 5),
  FL_PDScrollbarType = (1 << 6),
  FL_PDPupFontSize = (1 << 7),
  FL_PDButtonFontSize = (1 << 8),
  FL_PDInputFontSize = (1 << 9),
  FL_PDSliderFontSize = (1 << 10),
  FL_PDVisual = (1 << 11),
  FL_PDULThickness = (1 << 12),
  FL_PDULPropWidth = (1 << 13),
  FL_PDBS = (1 << 14),
  FL_PDCoordUnit = (1 << 15),
  FL_PDDebug = (1 << 16),
  FL_PDSharedMap = (1 << 17),
  FL_PDStandardMap = (1 << 18),
  FL_PDBorderWidth = (1 << 19),
  FL_PDSafe = (1 << 20),
  FL_PDMenuFontSize = (1 << 21),
  FL_PDBrowserFontSize = (1 << 22),
  FL_PDChoiceFontSize = (1 << 23),
  FL_PDLabelFontSize = (1 << 24)
};

#define FL_PDButtonLabel FL_PDButtonLabelSize

FL_EXPORT void fl_set_defaults(unsigned long mask, FL_IOPT *cntl){};

FL_EXPORT void fl_set_tabstop(const char *s){};

FL_EXPORT int fl_get_visual_depth(void)
{
  return 0;
};

FL_EXPORT int fl_is_global_clipped(void)
{
  return 0;
};

FL_EXPORT int fl_is_clipped(int include_global)
{
  return 0;
};

FL_EXPORT int fl_is_text_clipped(int include_global)
{
  return 0;
};

FL_EXPORT void fl_set_clipping(FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_unset_clipping(){};

FL_EXPORT void fl_set_text_clipping(FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_unset_text_clipping(void){};

FL_EXPORT int fl_get_global_clipping(FL_COORD *x, FL_COORD *y, FL_COORD *w, FL_COORD *h)
{
  return 0;
};

FL_EXPORT int fl_get_clipping(int include_global, FL_COORD *x, FL_COORD *y, FL_COORD *w, FL_COORD *h)
{
  return 0;
};

FL_EXPORT int fl_get_text_clipping(int include_global, FL_COORD *x, FL_COORD *y, FL_COORD *w, FL_COORD *h)
{
  return 0;
};

FL_EXPORT void fl_set_gc_clipping(GC gc, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h){};

FL_EXPORT void fl_unset_gc_clipping(GC gc){};

/* How we pack and unpack colors */

#ifndef FL_PCBITS
typedef unsigned char FL_PCTYPE; /* primary color type */
#define FL_PCBITS 8 /* primary color bits */
#define FL_PCMAX ((1 << FL_PCBITS) - 1)
#define FL_PCCLAMP(a) ((a) > (FL_PCMAX) ? (FL_PCMAX) : ((a) < 0 ? 0 : (a)))
typedef unsigned int FL_PACKED4;
#define FL_PACKED FL_PACKED4

#define FL_RMASK 0x000000ff
#define FL_RSHIFT 0
#define FL_GMASK 0x0000ff00
#define FL_GSHIFT 8
#define FL_BMASK 0x00ff0000
#define FL_BSHIFT 16
#define FL_AMASK 0xff000000
#define FL_ASHIFT 24

/* If PCBITS is not 8, we need to apply the RGBmask */

#define FL_GETR(packed) (((packed) >> FL_RSHIFT) & FL_RMASK)
#define FL_GETG(packed) (((packed) >> FL_GSHIFT) & FL_PCMAX)
#define FL_GETB(packed) (((packed) >> FL_BSHIFT) & FL_PCMAX)
#define FL_GETA(packed) (((packed) >> FL_ASHIFT) & FL_PCMAX)

#define FL_PACK3(r, g, b) (((r) << FL_RSHIFT) | ((g) << FL_GSHIFT) | ((b) << FL_BSHIFT))

#define FL_PACK FL_PACK3

#define FL_PACK4(r, g, b, a) (FL_PACK3(r, g, b) | ((a) << FL_ASHIFT))

#define FL_UNPACK(p, r, g, b) \
  do {                        \
    r = FL_GETR(p);           \
    g = FL_GETG(p);           \
    b = FL_GETB(p);           \
  } while (0)

#define FL_UNPACK3 FL_UNPACK

#define FL_UNPACK4(p, r, g, b, a) \
  do {                            \
    FL_UNPACK3(p, r, g, b);       \
    a = FL_GETA(p);               \
  } while (0)

#endif

typedef struct {
  unsigned int rshift, rmask, rbits;
  unsigned int gshift, gmask, gbits;
  unsigned int bshift, bmask, bbits;
  int bits_per_rgb;
  int colormap_size;
} FL_RGB2PIXEL_;

#define FL_RGB2PIXEL FL_RGB2PIXEL_

#endif /* ! defined FL_XBASIC_H */


/**
 * \file box.h
 *
 */

#ifndef FL_BOX_H
#define FL_BOX_H

/* Type is already defined in Basic.h */

FL_EXPORT FL_OBJECT *fl_create_box(int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

FL_EXPORT FL_OBJECT *fl_add_box(int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

#endif /* ! defined FL_BOX_H */



/**
 * \file canvas.h
 *
 * Header for FL_CANVAS
 *
 */

#ifndef FL_CANVAS_H_
#define FL_CANVAS_H_

typedef enum { FL_NORMAL_CANVAS, FL_SCROLLED_CANVAS } FL_CANVAS_TYPE;

typedef int (*FL_HANDLE_CANVAS)(FL_OBJECT *, Window, int, int, XEvent *, void *);

typedef int (*FL_MODIFY_CANVAS_PROP)(FL_OBJECT *);

/******************** Default *********************/

#define FL_CANVAS_BOXTYPE FL_DOWN_BOX /* really the decoration frame */
#define FL_CANVAS_ALIGN FL_ALIGN_TOP

/************ Interfaces    ************************/

FL_EXPORT FL_OBJECT *fl_create_generic_canvas(int canvas_class, int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

extern FL_OBJECT *websrv_fl_add_canvas(int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label);

FL_EXPORT FL_OBJECT *fl_create_canvas(int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

  /* backward compatibility */

#define fl_set_canvas_decoration fl_set_object_boxtype

FL_EXPORT void fl_set_canvas_colormap(FL_OBJECT *ob, Colormap colormap){};

FL_EXPORT void fl_set_canvas_visual(FL_OBJECT *obj, Visual *vi){};

FL_EXPORT void fl_set_canvas_depth(FL_OBJECT *obj, int depth){};

FL_EXPORT void fl_set_canvas_attributes(FL_OBJECT *ob, unsigned int mask, XSetWindowAttributes *xswa){};

FL_EXPORT FL_HANDLE_CANVAS fl_add_canvas_handler(FL_OBJECT *ob, int ev, FL_HANDLE_CANVAS h, void *udata)
{
  return NULL;
};


FL_EXPORT Window fl_get_canvas_id(FL_OBJECT *ob)
{
  return NULL;
};

FL_EXPORT Colormap fl_get_canvas_colormap(FL_OBJECT *ob)
{
  return NULL;
};

FL_EXPORT int fl_get_canvas_depth(FL_OBJECT *obj)
{
  return 0;
};

FL_EXPORT void fl_remove_canvas_handler(FL_OBJECT *ob, int ev, FL_HANDLE_CANVAS h){};

FL_EXPORT void fl_share_canvas_colormap(FL_OBJECT *ob, Colormap colormap){};

FL_EXPORT void fl_clear_canvas(FL_OBJECT *ob){};

FL_EXPORT void fl_modify_canvas_prop(FL_OBJECT *obj, FL_MODIFY_CANVAS_PROP init, FL_MODIFY_CANVAS_PROP activate, FL_MODIFY_CANVAS_PROP cleanup){};

FL_EXPORT void fl_canvas_yield_to_shortcut(FL_OBJECT *ob, int yes){};

/* This is an attempt to maintain some sort of backwards compatibility
 * with old code whilst also getting rid of the old, system-specific
 * hack. */

#ifdef AUTOINCLUDE_GLCANVAS_H
#include <glcanvas.h>
#endif

#endif /* ! defined FL_CANVAS_H */



  /**
   * \file text.h
   */

#ifndef FL_TEXT_H
#define FL_TEXT_H

enum { FL_NORMAL_TEXT };

#define FL_TEXT_BOXTYPE FL_FLAT_BOX
#define FL_TEXT_COL1 FL_COL1
#define FL_TEXT_COL2 FL_MCOL
#define FL_TEXT_LCOL FL_LCOL
#define FL_TEXT_ALIGN (FL_ALIGN_LEFT | FL_ALIGN_INSIDE)

FL_EXPORT FL_OBJECT *fl_create_text(int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

FL_EXPORT FL_OBJECT *fl_add_text(int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

#endif /* ! defined FL_TEXT_H */


/**
 * \file xyplot.h
 */

#ifndef FL_XYPLOT_H
#define FL_XYPLOT_H

/*  Class FL_XYPLOT */

typedef enum {
  FL_NORMAL_XYPLOT, /* solid line                        */
  FL_SQUARE_XYPLOT, /* with added square                 */
  FL_CIRCLE_XYPLOT, /* with added circle                 */
  FL_FILL_XYPLOT, /* fill completely                   */
  FL_POINTS_XYPLOT, /* only data points                  */
  FL_DASHED_XYPLOT, /* dashed line                       */
  FL_IMPULSE_XYPLOT,
  FL_ACTIVE_XYPLOT, /* accepts interactive manipulations */
  FL_EMPTY_XYPLOT,
  FL_DOTTED_XYPLOT,
  FL_DOTDASHED_XYPLOT,
  FL_LONGDASHED_XYPLOT,
  FL_LINEPOINTS_XYPLOT /* line & points                     */
} FL_XYPLOT_TYPE;

enum { FL_LINEAR, FL_LOG };

enum { FL_GRID_NONE = 0, FL_GRID_MAJOR = 1, FL_GRID_MINOR = 2 };

/***** Defaults *****/

#define FL_XYPLOT_BOXTYPE FL_FLAT_BOX
#define FL_XYPLOT_COL1 FL_COL1
#define FL_XYPLOT_LCOL FL_LCOL
#define FL_XYPLOT_ALIGN FL_ALIGN_BOTTOM
#define FL_MAX_XYPLOTOVERLAY 32

/***** Others   *****/

FL_EXPORT FL_OBJECT *fl_create_xyplot(int t, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label)
{
  return NULL;
};

extern FL_OBJECT *websrv_fl_add_xyplot(int t, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label);

FL_EXPORT void fl_set_xyplot_data(FL_OBJECT *ob, float *x, float *y, int n, const char *title, const char *xlabel, const char *ylabel){};

FL_EXPORT void fl_set_xyplot_data_double(FL_OBJECT *ob, double *x, double *y, int n, const char *title, const char *xlabel, const char *ylabel){};

FL_EXPORT int fl_set_xyplot_file(FL_OBJECT *ob, const char *f, const char *title, const char *xl, const char *yl)
{
  return 0;
};

FL_EXPORT void fl_insert_xyplot_data(FL_OBJECT *ob, int id, int n, double x, double y){};

#define fl_set_xyplot_datafile fl_set_xyplot_file

FL_EXPORT void fl_add_xyplot_text(FL_OBJECT *ob, double x, double y, const char *text, int al, FL_COLOR col){};

FL_EXPORT void fl_delete_xyplot_text(FL_OBJECT *ob, const char *text){};

FL_EXPORT int fl_set_xyplot_maxoverlays(FL_OBJECT *ob, int maxover)
{
  return 0;
};

FL_EXPORT void fl_add_xyplot_overlay(FL_OBJECT *ob, int id, float *x, float *y, int n, FL_COLOR col)
{
  return;
};

FL_EXPORT void fl_set_xyplot_xbounds(FL_OBJECT *ob, double xmin, double xmax){};

FL_EXPORT void fl_set_xyplot_ybounds(FL_OBJECT *ob, double ymin, double ymax){};

FL_EXPORT void fl_get_xyplot_xbounds(FL_OBJECT *ob, float *xmin, float *xmax){};

FL_EXPORT void fl_get_xyplot_ybounds(FL_OBJECT *ob, float *ymin, float *ymax){};

FL_EXPORT void fl_get_xyplot(FL_OBJECT *ob, float *x, float *y, int *i){};

FL_EXPORT int fl_get_xyplot_data_size(FL_OBJECT *obj)
{
  return 0;
};

extern void websrv_fl_get_xyplot_data(FL_OBJECT *ob, float *x, float *y, int *n);

extern void websrv_fl_get_xyplot_data_pointer(FL_OBJECT *ob, int id, float **x, float **y, int *n);



FL_EXPORT void fl_set_xyplot_xgrid(FL_OBJECT *ob, int xgrid){};


typedef void (*FL_XYPLOT_SYMBOL)(FL_OBJECT *, int, FL_POINT *, int, int, int);

FL_EXPORT FL_XYPLOT_SYMBOL fl_set_xyplot_symbol(FL_OBJECT *ob, int id, FL_XYPLOT_SYMBOL symbol)
{
  return NULL;
};

#endif  /* FL_XYPLOT_H */

/*----------------------------------------------------------------------*/
/* new functions for interfacing with webserver                         */

extern int websrv_nf_getdata(FL_OBJECT *graph, int layer, websrv_scopedata_msg_t **msg);
extern void websrv_free_xyplot(FL_OBJECT *obj);
#if defined __cplusplus
}
#endif

#endif /* FL_FORMS_H */
