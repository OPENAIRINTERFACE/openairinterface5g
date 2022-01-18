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

/*! \file lte_enb_scope.c
 * \brief enb specific softsope implementation
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

// current status is that every UE has a DL scope for a SINGLE eNB (eNB_id=0)
// at eNB 0, an UL scope for every UE
FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
FD_lte_phy_scope_enb *form_enb[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
FD_stats_form        *form_stats=NULL,*form_stats_l2=NULL;
char                 title[255];
unsigned char        scope_enb_num_ue = 2;
static pthread_t     forms_thread; //xforms

void reset_stats(FL_OBJECT *button, long arg) {
  int i,j,k;
  PHY_VARS_eNB *phy_vars_eNB = RC.eNB[0][0];

  printf("XXX %d %d %d\n",
	 sizeofArray(phy_vars_eNB->UE_stats),
	 sizeofArray( phy_vars_eNB->UE_stats[i].dlsch_NAK),
	 sizeofArray( *phy_vars_eNB->UE_stats[i].dlsch_NAK)
	 );
 
  for (i=0; i<sizeofArray(phy_vars_eNB->UE_stats); i++) {
    for (k=0; k<sizeofArray(phy_vars_eNB->UE_stats[i].dlsch_NAK); k++) { //harq_processes
      for (j=0; j<sizeofArray(*phy_vars_eNB->UE_stats[i].dlsch_NAK); j++) {
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

static void *scope_thread_eNB(void *arg) {
  struct sched_param sched_param;
  int UE_id, CC_id;
  int ue_cnt=0;
  sched_param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;
  sched_setscheduler(0, SCHED_FIFO,&sched_param);
  printf("Scope thread has priority %d\n",sched_param.sched_priority);
  while (!oai_exit) {
    ue_cnt=0;

    for(UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
      for(CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        if ((ue_cnt<scope_enb_num_ue)) {
          phy_scope_eNB(form_enb[CC_id][ue_cnt],
                        RC.eNB[0][CC_id],
                        UE_id);
          ue_cnt++;
        }
      }
    }

    usleep(100*1000);
  }

  pthread_exit((void *)arg);
}


int enbscope_autoinit(void) {
  int UE_id;
  printf("XFORMS\n");
  int argc=config_get_if()->argc;
  char  **argv=config_get_if()->argv;
  fl_initialize (&argc, argv, NULL, 0, 0);
  form_stats_l2 = create_form_stats_form();
  fl_show_form (form_stats_l2->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "l2 stats");
  form_stats = create_form_stats_form();
  fl_show_form (form_stats->stats_form, FL_PLACE_HOTSPOT, FL_FULLBORDER, "stats");

  for(UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
    for(int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      form_enb[CC_id][UE_id] = create_lte_phy_scope_enb();
      sprintf (title, "LTE UL SCOPE eNB for CC_id %d, UE %d",CC_id,UE_id);
      fl_show_form (form_enb[CC_id][UE_id]->lte_phy_scope_enb, FL_PLACE_HOTSPOT, FL_FULLBORDER, title);

      if (0) {
        fl_set_button(form_enb[CC_id][UE_id]->button_0,1);
        fl_set_object_label(form_enb[CC_id][UE_id]->button_0,"DL Traffic ON");
      } else {
        fl_set_button(form_enb[CC_id][UE_id]->button_0,0);
        fl_set_object_label(form_enb[CC_id][UE_id]->button_0,"DL Traffic OFF");
      }
    } // CC_id
  } // UE_id

  int ret = pthread_create(&forms_thread, NULL, scope_thread_eNB, NULL);

  if (ret == 0)
    pthread_setname_np( forms_thread, "xforms" );

  printf("Scope thread created, ret=%d\n",ret);
  return 0;
} // start_forms_eNB



void end_forms(void) {
  printf("waiting for XFORMS thread\n");
  pthread_join(forms_thread,NULL);
  fl_hide_form(form_stats->stats_form);
  fl_free_form(form_stats->stats_form);
  fl_hide_form(form_stats_l2->stats_form);
  fl_free_form(form_stats_l2->stats_form);

  for(int UE_id=0; UE_id<scope_enb_num_ue; UE_id++) {
    for(int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      fl_hide_form(form_enb[CC_id][UE_id]->lte_phy_scope_enb);
      fl_free_form(form_enb[CC_id][UE_id]->lte_phy_scope_enb);
    }
  }
} // end_forms

