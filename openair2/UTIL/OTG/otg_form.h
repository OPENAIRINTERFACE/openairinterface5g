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
