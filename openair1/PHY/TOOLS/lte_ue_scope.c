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


/*! \file lte_ue_scope.c
 * \brief UE specific softsope implementation
 * \author Nokia BellLabs France, francois Taburet
 * \date 2019
 * \version 0.1
 * \company Nokia Bell-Labs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#include "targets/RT/USER/lte-softmodem.h"
#include "UTIL/OPT/opt.h"
#include "common/config/config_userapi.h"
#include "PHY/TOOLS/lte_phy_scope.h"
#include "targets/RT/USER/stats.h"
#include "PHY/phy_vars_ue.h"
// current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
// at eNB 0, an UL scope for every UE
FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
FD_lte_phy_scope_enb *form_enb[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
FD_stats_form                  *form_stats=NULL,*form_stats_l2=NULL;
char title[255];
unsigned char                   scope_enb_num_ue = 2;
static pthread_t                forms_thread; //xforms


void reset_stats(FL_OBJECT *button, long arg) {
  int i,j,k;
  PHY_VARS_eNB *phy_vars_eNB = RC.eNB[0][0];

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    for (k=0; k<8; k++) { //harq_processes
      for (j=0; j<phy_vars_eNB->dlsch[i][0]->Mlimit; j++) {
        phy_vars_eNB->UE_stats[i].dlsch_NAK[k][j]=0;
        phy_vars_eNB->UE_stats[i].dlsch_ACK[k][j]=0;
        phy_vars_eNB->UE_stats[i].dlsch_trials[k][j]=0;
      }

      phy_vars_eNB->UE_stats[i].dlsch_l2_errors[k]=0;
      phy_vars_eNB->UE_stats[i].ulsch_errors[k]=0;
      phy_vars_eNB->UE_stats[i].ulsch_consecutive_errors=0;
      phy_vars_eNB->UE_stats[i].dlsch_sliding_cnt=0;
      phy_vars_eNB->UE_stats[i].dlsch_NAK_round0=0;
      phy_vars_eNB->UE_stats[i].dlsch_mcs_offset=0;
    }
  }
}


static void *scope_thread_UE(void *arg) {
  char stats_buffer[16384];
  struct sched_param sched_param;
  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);
  printf("Scope thread has priority %d\n",sched_param.sched_priority);

  while (!oai_exit) {
    //      dump_ue_stats (PHY_vars_UE_g[0][0], &PHY_vars_UE_g[0][0]->proc.proc_rxtx[0],stats_buffer, 0, mode,rx_input_level_dBm);
    //fl_set_object_label(form_stats->stats_text, stats_buffer);
    fl_clear_browser(form_stats->stats_text);
    fl_add_browser_line(form_stats->stats_text, stats_buffer);
    phy_scope_UE(form_ue[0],
                 PHY_vars_UE_g[0][0],
                 0,
                 0,7);
    //  printf("%s",stats_buffer);
  }
  pthread_exit((void *)arg);
}


int uescope_autoinit(void) {
  int UE_id;
  int argc=config_get_if()->argc;
  char  **argv=config_get_if()->argv;
  printf("XFORMS\n");
  fl_initialize (&argc, argv, NULL, 0, 0);
  form_stats = create_form_stats_form();
  fl_show_form (form_stats->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "stats");
  UE_id = 0;
  form_ue[UE_id] = create_lte_phy_scope_ue();
  sprintf (title, "LTE DL SCOPE UE");
  fl_show_form (form_ue[UE_id]->lte_phy_scope_ue, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);
  /*
  if (openair_daq_vars.use_ia_receiver) {
  fl_set_button(form_ue[UE_id]->button_0,1);
  fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver ON");
  } else {
  fl_set_button(form_ue[UE_id]->button_0,0);
  fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver OFF");
  }*/
  fl_set_button(form_ue[UE_id]->button_0,0);
  fl_set_object_label(form_ue[UE_id]->button_0, "IA Receiver OFF");
  int ret = pthread_create(&forms_thread, NULL, scope_thread_UE, NULL);

  if (ret == 0)
    pthread_setname_np( forms_thread, "xforms" );

  printf("Scope thread created, ret=%d\n",ret);
  return 0;
} // start_forms_UE


void end_forms(void) {
  printf("waiting for XFORMS thread\n");
  pthread_join(forms_thread,NULL);
  fl_hide_form(form_stats->stats_form);
  fl_free_form(form_stats->stats_form);
  fl_hide_form(form_ue[0]->lte_phy_scope_ue);
  fl_free_form(form_ue[0]->lte_phy_scope_ue);
} // end_forms
