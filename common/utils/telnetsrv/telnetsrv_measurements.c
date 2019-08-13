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

/*! \file common/utils/telnetsrv/telnetsrv_measurements.c
 * \brief: implementation of telnet commands related to measurments
 * \author Francois TABURET
 * \date 2019
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
#define TELNETSRV_MEASURMENTS_MAIN
#include "common/utils/LOG/log.h"
#include "common/config/config_userapi.h"
#include "telnetsrv_measurements.h"
#include "telnetsrv_ltemeasur_def.h"
#include "telnetsrv_cpumeasur_def.h"
#include "openair2/LAYER2/MAC/mac.h"
#include "openair1/PHY/phy_extern.h"

void measurcmd_display_macstats(telnet_printfunc_t prnt);
void measurcmd_display_macstats_ue(telnet_printfunc_t prnt);
void measurcmd_display_rlcstats(telnet_printfunc_t prnt);
void measurcmd_display_phycpu(telnet_printfunc_t prnt);
void measurcmd_display_maccpu(telnet_printfunc_t prnt);
void measurcmd_display_pdcpcpu(telnet_printfunc_t prnt);


static telnet_measurgroupdef_t measurgroups[] = {
  {"enb",   GROUP_LTESTATS,0, measurcmd_display_macstats,   {NULL}},
  {"enbues",GROUP_LTESTATS,0, measurcmd_display_macstats_ue,{NULL}},
  {"rlc",   GROUP_LTESTATS,0, measurcmd_display_rlcstats,   {NULL}},
  {"phycpu",GROUP_CPUSTATS,0, measurcmd_display_phycpu,     {NULL}},
  {"maccpu",GROUP_CPUSTATS,0, measurcmd_display_maccpu,      {NULL}},
  {"pdcpcpu",GROUP_CPUSTATS,0, measurcmd_display_pdcpcpu,      {NULL}},
};
#define TELNET_NUM_MEASURGROUPS (sizeof(measurgroups)/sizeof(telnet_measurgroupdef_t))

static int                     eNB_id =0;
static char                    *grouptypes[] = {"ltestats","cpustats"};
static double                  cpufreq;
#define TELNET_NUM_MEASURTYPES (sizeof(grouptypes)/sizeof(char *))

#define HDR "---------------------------------"

void measurcmd_display_groups(telnet_printfunc_t prnt) {
  prnt("  %*s %10s %s\n",TELNET_MAXMEASURNAME_LEN-1,"name","type","nombre de mesures");

  for(int i=0; i<TELNET_NUM_MEASURGROUPS; i++)
    prnt("%02d %*s %10s %i\n",i,TELNET_MAXMEASURNAME_LEN-1,measurgroups[i].groupname,
         grouptypes[measurgroups[i].type], measurgroups[i].size);
} /* measurcmd_display_groups */
/*----------------------------------------------------------------------------------------------------*/
/* cpu measurements functions                         */
void measurcmd_display_cpumeasures(telnet_printfunc_t prnt, telnet_cpumeasurdef_t  *cpumeasure, int cpumeasure_size) {
  for (int i=0; i<cpumeasure_size; i++) {
    prnt("%02d %*s:  %15.3f us; %15d %s",i,TELNET_MAXMEASURNAME_LEN-1,(cpumeasure+i)->statname,
         ((cpumeasure+i)->astatptr->trials!=0)?(((cpumeasure+i)->astatptr->diff)/((cpumeasure+i)->astatptr->trials))/cpufreq/1000:0,
         (cpumeasure+i)->astatptr->trials, ((i%2)==1)?"|\n":"  | " );
  }

  prnt("\n\n");
} /* measurcmd_display_measures */

#define PRINT_CPUMEAS_STATE  ((cpumeas(CPUMEAS_GETSTATE))?"enabled":"disabled")
void measurcmd_display_phycpu(telnet_printfunc_t prnt) {
  PHY_VARS_eNB *phyvars = RC.eNB[eNB_id][0];
  telnet_cpumeasurdef_t  cpumeasur[]=CPU_PHYENB_MEASURE;
  prnt("%s cpu (%1.1g GHz) measurements: PHY (cpustats %s) %s\n",HDR,cpufreq,
       PRINT_CPUMEAS_STATE,HDR);
  measurcmd_display_cpumeasures(prnt, cpumeasur, sizeof(cpumeasur)/sizeof(telnet_cpumeasurdef_t));
}

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
/*----------------------------------------------------------------------------------------------------*/
/* lte measurements functions                         */
uint64_t measurcmd_getstatvalue(telnet_ltemeasurdef_t *measur,telnet_printfunc_t prnt) {
  uint64_t val;

  switch(measur->vtyp) {
    case TELNET_VARTYPE_INT64:
      val = (uint64_t)(*((uint64_t *)(measur->vptr)));
      break;

    case TELNET_VARTYPE_INT32:
      val = (uint64_t)(*((uint32_t *)(measur->vptr)));
      break;

    case TELNET_VARTYPE_INT16:
      val = (uint64_t)(*((uint16_t *)(measur->vptr)));
      break;

    case TELNET_VARTYPE_INT8:
      val = (uint64_t)(*((uint8_t *)(measur->vptr)));
      break;

    case TELNET_VARTYPE_UINT:
      val = (uint64_t)(*((unsigned int *)(measur->vptr)));
      break;

    default:
      prnt("%s %i: unknown type \n",measur->statname,measur->vtyp);
      val = (uint64_t)(*((uint64_t *)(measur->vptr)));
      break;
  }

  return val;
} /* measurcmd_getstatvalue */

void measurcmd_display_measures(telnet_printfunc_t prnt, telnet_ltemeasurdef_t  *statsptr, int stats_size) {
  for (int i=0; i<stats_size; i++) {
    prnt("%*s = %15llu%s",TELNET_MAXMEASURNAME_LEN-1,statsptr[i].statname,
         measurcmd_getstatvalue(&(statsptr[i]),prnt), ((i%3)==2)?"\n":"   ");
  }

  prnt("\n\n");
} /* measurcmd_display_measures */

void measurcmd_display_macstats_ue(telnet_printfunc_t prnt) {
  UE_list_t *UE_list = &(RC.mac[eNB_id]->UE_list);

  for (int UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
    for (int i=0; i<UE_list->numactiveCCs[UE_id]; i++) {
      int CC_id = UE_list->ordered_CCids[i][UE_id];
      prnt("%s UE %i Id %i CCid %i %s\n",HDR,i,UE_id,CC_id,HDR);
      eNB_UE_STATS *macuestatptr = &(UE_list->eNB_UE_stats[CC_id][UE_id]);
      telnet_ltemeasurdef_t  statsptr[]=LTEMAC_UEMEASURE;
      measurcmd_display_measures(prnt, statsptr, sizeof(statsptr)/sizeof(telnet_ltemeasurdef_t));
    }
  }
} /* measurcmd_display_macstats_ue */

void measurcmd_display_macstats(telnet_printfunc_t prnt) {
  for (int CC_id=0 ; CC_id < MAX_NUM_CCs; CC_id++) {
    eNB_STATS *macstatptr=&(RC.mac[eNB_id]->eNB_stats[CC_id]);
    telnet_ltemeasurdef_t  statsptr[]=LTEMAC_MEASURE;
    prnt("%s eNB %i mac stats CC %i frame %u %s\n",
         HDR, eNB_id, CC_id, RC.mac[eNB_id]->frame,HDR);
    measurcmd_display_measures(prnt,statsptr,sizeof(statsptr)/sizeof(telnet_ltemeasurdef_t));
  }
} /* measurcmd_display_macstats */


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
} /* status measurcmd_rlc_stat_req */


void measurcmd_display_rlcstats(telnet_printfunc_t prnt) {
  protocol_ctxt_t      ctxt;
  UE_list_t *UE_list = &(RC.mac[eNB_id]->UE_list);
  telnet_ltemeasurdef_t  statsptr[]=LTE_RLCMEASURE;
  int num_rlcmeasure = sizeof(statsptr)/sizeof(telnet_ltemeasurdef_t );
  unsigned int *rlcstats = malloc(num_rlcmeasure*sizeof(unsigned int));
  eNB_MAC_INST *eNB = RC.mac[eNB_id];

  for(int i=0; i <num_rlcmeasure ; i++) {
    statsptr[i].vptr = rlcstats + i;
  }

  for (int UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
#define NB_eNB_INST 1
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt,eNB_id, ENB_FLAG_YES,UE_list->eNB_UE_stats[0][UE_id].crnti,
                                   eNB->frame,eNB->subframe,eNB_id);
    measurcmd_display_one_rlcstat(prnt, UE_id, statsptr, num_rlcmeasure, rlcstats, "DCCH", &ctxt, SRB_FLAG_YES, DCCH);
    measurcmd_display_one_rlcstat(prnt, UE_id, statsptr, num_rlcmeasure, rlcstats, "DTCH", &ctxt, SRB_FLAG_NO,  DTCH-2);
  }
} /* measurcmd_display_macstats_ue */

/*------------------------------------------------------------------------------------------------------------------------*/
/* function called by the telnet server when measur command is entered */
int measurcmd_show(char *buf, int debug, telnet_printfunc_t prnt) {
  char *subcmd=NULL;
  int idx1, idx2;
  int badcmd=1;

  if (debug > 0)
    prnt(" measurcmd_show received %s\n",buf);

  //   char tmp[20480];
  //   dump_eNB_l2_stats(tmp, 0);
  //   prnt("%s\n",tmp);
  int s = sscanf(buf,"%ms %i-%i\n",&subcmd, &idx1,&idx2);

  if (s>0) {
    if ( strcmp(subcmd,"groups") == 0) {
      measurcmd_display_groups(prnt);
      badcmd=0;
    } else {
      for (int i=0; i<TELNET_NUM_MEASURTYPES; i++) {
        if(strcmp(subcmd,grouptypes[i]) == 0) {
          for(int j=0; j<TELNET_NUM_MEASURGROUPS; j++) {
            if(i == measurgroups[j].type) {
              badcmd=0;
              measurgroups[j].displayfunc(prnt);
            }
          } /* for j...*/
        }
      }/* for i...*/

      for (int i=0; i<TELNET_NUM_MEASURGROUPS; i++) {
        if(strcmp(subcmd,measurgroups[i].groupname) == 0) {
          measurgroups[i].displayfunc(prnt);
          badcmd=0;
          break;
        }
      }
    }

    free(subcmd);
  } /* s>0 */

  if (badcmd) {
    prnt("%s: unknown measur command\n",buf);
  }

  return CMDSTATUS_FOUND;
}


int measurcmd_cpustats(char *buf, int debug, telnet_printfunc_t prnt) {
  char *subcmd=NULL;
  int idx1, idx2;
  int badcmd=1;

  if (debug > 0)
    prnt(" measurcmd_show received %s\n",buf);

  int s = sscanf(buf,"%ms %i-%i\n",&subcmd, &idx1,&idx2);

  if (s>0) {
    if ( strcmp(subcmd,"enable") == 0) {
      cpumeas(CPUMEAS_ENABLE);
      badcmd=0;
    } else if ( strcmp(subcmd,"disable") == 0) {
      cpumeas(CPUMEAS_DISABLE);
      badcmd=0;
    }
  }

  if (badcmd) {
    prnt("Cpu measurments state: %s\n",PRINT_CPUMEAS_STATE);
  }

  free(subcmd);
  return CMDSTATUS_FOUND;
}
/*-------------------------------------------------------------------------------------*/

/* function called at telnet server init to add the measur command */
void add_measur_cmds(void) {
  add_telnetcmd("measur",measur_vardef,measur_cmdarray);
  cpufreq = get_cpu_freq_GHz();
}
