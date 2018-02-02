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

