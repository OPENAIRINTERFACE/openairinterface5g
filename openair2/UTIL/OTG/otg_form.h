/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
