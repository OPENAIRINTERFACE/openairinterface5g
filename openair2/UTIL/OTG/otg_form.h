#ifndef __OTG_FORM_H__
# define __OTG_FORM_H__

#include <forms.h>
#include <stdlib.h>
#include "otg.h"




/**** Forms and Objects ****/
typedef struct {
  FL_FORM *otg;
  FL_OBJECT *owd;
  FL_OBJECT *throughput;
  FL_OBJECT *loss_ratio;
  FL_OBJECT *simu_time;
} FD_otg;

extern FD_otg * create_form_otg(void);
void show_otg_form(void);
void add_tab_metric(int eNB_id, int UE_id, float owd, float throughput, int ctime);
void plot_graphes_ul(int src, int dst, int ctime);
void plot_graphes_dl(int src, int dst, int ctime);
void create_form_clock(void);
void exit_cb(FL_OBJECT *ob, long q);


#endif
