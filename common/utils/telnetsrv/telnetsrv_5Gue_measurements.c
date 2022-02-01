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

/*! \file common/utils/telnetsrv/telnetsrv_nrue_measurements.c
 * \brief: implementation of telnet commands related to nrUE measurments
 * \author Francois TABURET
 * \date 2021
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>




#define TELNETSERVERCODE
#include "telnetsrv.h"

#include "common/utils/LOG/log.h"
#include "common/config/config_userapi.h"
#include "telnetsrv_measurements.h"
#include "telnetsrv_ltemeasur_def.h"
#include "telnetsrv_cpumeasur_def.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_defs.h"
#include "openair1/PHY/phy_extern_nr_ue.h"

void measurcmd_display_macstats(telnet_printfunc_t prnt);
void measurcmd_display_macstats_ue(telnet_printfunc_t prnt);
void measurcmd_display_rlcstats(telnet_printfunc_t prnt);
void measurcmd_display_phycpu(telnet_printfunc_t prnt);
void measurcmd_display_maccpu(telnet_printfunc_t prnt);
void measurcmd_display_pdcpcpu(telnet_printfunc_t prnt);


static telnet_measurgroupdef_t nrUEmeasurgroups[] = {
//  {"ue",   GROUP_LTESTATS,0, measurcmd_display_macstats,   {NULL}},
//  {"rlc",   GROUP_LTESTATS,0, measurcmd_display_rlcstats,   {NULL}},
  {"phycpu",GROUP_CPUSTATS,0, measurcmd_display_phycpu,     {NULL}},
//  {"maccpu",GROUP_CPUSTATS,0, measurcmd_display_maccpu,      {NULL}},
//  {"pdcpcpu",GROUP_CPUSTATS,0, measurcmd_display_pdcpcpu,      {NULL}},
};
#define TELNET_NUM_NRUEMEASURGROUPS (sizeof(nrUEmeasurgroups)/sizeof(telnet_measurgroupdef_t))


static double                  cpufreq;


#define HDR "---------------------------------"


int get_measurgroups(telnet_measurgroupdef_t **measurgroups) {
  *measurgroups = nrUEmeasurgroups;
  return TELNET_NUM_NRUEMEASURGROUPS;
}


void measurcmd_display_phycpu(telnet_printfunc_t prnt) {
  PHY_VARS_NR_UE *UE = PHY_vars_UE_g[0][0];
  telnet_cpumeasurdef_t  cpumeasur[]=CPU_PHYNRUE_MEASURE;
  prnt("%s cpu (%1.1g GHz) measurements: PHY (cpustats %s) %s\n",HDR,cpufreq,
       PRINT_CPUMEAS_STATE,HDR);
  measurcmd_display_cpumeasures(prnt, cpumeasur, sizeof(cpumeasur)/sizeof(telnet_cpumeasurdef_t));
}
/*
void measurcmd_display_maccpu(telnet_printfunc_t prnt) {
  eNB_MAC_INST *macvars = RC.mac[eNB_id];
  telnet_cpumeasurdef_t  cpumeasur[]=CPU_MACENB_MEASURE;
  prnt("%s cpu (%1.1g GHz) measurements: MAC (cpustats %s) %s\n",HDR,cpufreq,
       PRINT_CPUMEAS_STATE,HDR);
  measurcmd_display_cpumeasures(prnt, cpumeasur, sizeof(cpumeasur)/sizeof(telnet_cpumeasurdef_t));
}

void measurcmd_display_pdcpcpu(telnet_printfunc_t prnt) {
  pdcp_stats_t *pdcpvars = &(eNB_pdcp_stats[eNB_id]);
  telnet_cpumeasurdef_t  cpumeasur[]=CPU_PDCPENB_MEASURE;
  prnt("%s cpu (%1.1g GHz) measurements: PDCP (cpustats %s) %s \n",HDR,cpufreq,
       PRINT_CPUMEAS_STATE,HDR);
  measurcmd_display_cpumeasures(prnt, cpumeasur, sizeof(cpumeasur)/sizeof(telnet_cpumeasurdef_t));
}
//----------------------------------------------------------------------------------------------------


void measurcmd_display_macstats_ue(telnet_printfunc_t prnt) {
  UE_info_t *UE_info = &(RC.mac[eNB_id]->UE_info);

  for (int UE_id=UE_info->list.head; UE_id>=0; UE_id=UE_info->list.next[UE_id]) {
    for (int i=0; i<UE_info->numactiveCCs[UE_id]; i++) {
      int CC_id = UE_info->ordered_CCids[i][UE_id];
      prnt("%s UE %i Id %i CCid %i %s\n",HDR,i,UE_id,CC_id,HDR);
      eNB_UE_STATS *macuestatptr = &(UE_info->eNB_UE_stats[CC_id][UE_id]);
      telnet_ltemeasurdef_t  statsptr[]=LTEMAC_UEMEASURE;
      measurcmd_display_measures(prnt, statsptr, sizeof(statsptr)/sizeof(telnet_ltemeasurdef_t));
    }
  }
} // measurcmd_display_macstats_ue 

void measurcmd_display_macstats(telnet_printfunc_t prnt) {
  for (int CC_id=0 ; CC_id < MAX_NUM_CCs; CC_id++) {
    eNB_STATS *macstatptr=&(RC.mac[eNB_id]->eNB_stats[CC_id]);
    telnet_ltemeasurdef_t  statsptr[]=LTEMAC_MEASURE;
    prnt("%s eNB %i mac stats CC %i frame %u %s\n",
         HDR, eNB_id, CC_id, RC.mac[eNB_id]->frame,HDR);
    measurcmd_display_measures(prnt,statsptr,sizeof(statsptr)/sizeof(telnet_ltemeasurdef_t));
  }
} // measurcmd_display_macstats 


void measurcmd_display_one_rlcstat(telnet_printfunc_t prnt, int UE_id, telnet_ltemeasurdef_t *statsptr, int num_rlcmeasure, unsigned int *rlcstats,
                                   char *rbid_str, protocol_ctxt_t *ctxt, const srb_flag_t srb_flagP, const rb_id_t rb_idP)

{
  int rlc_status = rlc_stat_req(ctxt,srb_flagP,rb_idP,
                                rlcstats,   rlcstats+1, rlcstats+2, rlcstats+3, rlcstats+4, rlcstats+5,
                                rlcstats+6, rlcstats+7, rlcstats+8, rlcstats+9, rlcstats+10, rlcstats+11,
                                rlcstats+12, rlcstats+13, rlcstats+14, rlcstats+15, rlcstats+16, rlcstats+17,
                                rlcstats+18, rlcstats+19, rlcstats+20, rlcstats+21, rlcstats+22, rlcstats+23,
                                rlcstats+24, rlcstats+25, rlcstats+26, rlcstats+27);

  if (rlc_status == RLC_OP_STATUS_OK) {
    prnt("%s UE %i RLC %s mode %s %s\n",HDR,UE_id, rbid_str,
         (rlcstats[0]==RLC_MODE_AM)? "AM": (rlcstats[0]==RLC_MODE_UM)?"UM":"NONE",HDR);
    measurcmd_display_measures(prnt, statsptr, num_rlcmeasure);
  }
} // status measurcmd_rlc_stat_req 


void measurcmd_display_rlcstats(telnet_printfunc_t prnt) {
  protocol_ctxt_t      ctxt;
  UE_info_t *UE_info = &(RC.mac[eNB_id]->UE_info);
  telnet_ltemeasurdef_t  statsptr[]=LTE_RLCMEASURE;
  int num_rlcmeasure = sizeof(statsptr)/sizeof(telnet_ltemeasurdef_t );
  unsigned int *rlcstats = malloc(num_rlcmeasure*sizeof(unsigned int));
  eNB_MAC_INST *eNB = RC.mac[eNB_id];

  for(int i=0; i <num_rlcmeasure ; i++) {
    statsptr[i].vptr = rlcstats + i;
  }

  for (int UE_id=UE_info->list.head; UE_id>=0; UE_id=UE_info->list.next[UE_id]) {
#define NB_eNB_INST 1
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt,eNB_id, ENB_FLAG_YES,UE_info->eNB_UE_stats[0][UE_id].crnti,
                                   eNB->frame,eNB->subframe,eNB_id);
    measurcmd_display_one_rlcstat(prnt, UE_id, statsptr, num_rlcmeasure, rlcstats, "DCCH", &ctxt, SRB_FLAG_YES, DCCH);
    measurcmd_display_one_rlcstat(prnt, UE_id, statsptr, num_rlcmeasure, rlcstats, "DTCH", &ctxt, SRB_FLAG_NO,  DTCH-2);
  }
} // measurcmd_display_macstats_ue 

*/


