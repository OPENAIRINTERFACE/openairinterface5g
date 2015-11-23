/* Form definition file generated with fdesign. */

#include <forms.h>
#include <stdlib.h>
#include "phy_procedures_sim_form.h"

FD_phy_procedures_sim *create_form_phy_procedures_sim(void)
{
  FL_OBJECT *obj;
  FD_phy_procedures_sim *fdui = (FD_phy_procedures_sim *) fl_calloc(1, sizeof(*fdui));

  fdui->phy_procedures_sim = fl_bgn_form(FL_NO_BOX, 640, 320);
  obj = fl_add_box(FL_UP_BOX,0,0,640,320,"");
  fdui->pusch_constellation = obj = fl_add_xyplot(FL_POINTS_XYPLOT,50,30,220,190,"PUSCH constellation");
  fl_set_object_color(obj,FL_BLACK,FL_YELLOW);
  fdui->pdsch_constellation = obj = fl_add_xyplot(FL_POINTS_XYPLOT,370,30,220,190,"PDSCH constellation");
  fl_set_object_color(obj,FL_BLACK,FL_YELLOW);
  fdui->ch00 = obj = fl_add_xyplot(FL_IMPULSE_XYPLOT,370,240,210,60,"CH00");
  fl_set_object_color(obj,FL_BLACK,FL_YELLOW);
  fl_end_form();

  fdui->phy_procedures_sim->fdui = fdui;

  return fdui;
}
/*---------------------------------------*/

