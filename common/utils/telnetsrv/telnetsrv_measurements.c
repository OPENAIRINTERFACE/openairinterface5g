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
 * \brief: implementation of telnet measurement functions common to all softmodems
 * \author Francois TABURET
 * \date 2020
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


static char                    *grouptypes[] = {"ltestats","cpustats"};
static double                  cpufreq;
#define TELNET_NUM_MEASURTYPES (sizeof(grouptypes)/sizeof(char *))

#define HDR "---------------------------------"

void measurcmd_display_groups(telnet_printfunc_t prnt,telnet_measurgroupdef_t *measurgroups,int groups_size) {
  prnt("  %*s %10s %s\n",TELNET_MAXMEASURNAME_LEN-1,"name","type","nombre de mesures");

  for(int i=0; i<groups_size; i++)
    prnt("%02d %*s %10s %i\n",i,TELNET_MAXMEASURNAME_LEN-1,measurgroups[i].groupname,
         grouptypes[measurgroups[i].type], measurgroups[i].size);
} /* measurcmd_display_groups */
/*----------------------------------------------------------------------------------------------------*/
/* cpu measurements functions                         */


static char *stridx(int max,int i, char *buff) {
    if (max>1)
        sprintf(buff,"[%d]",i);
    else
        sprintf(buff,"   ");
    return buff;

}
void measurcmd_display_cpumeasures(telnet_printfunc_t prnt, telnet_cpumeasurdef_t  *cpumeasure, int cpumeasure_size) {
  int p=0;
  char stridx1[16];
  char stridx2[16];
  for (int i=0; i<cpumeasure_size; i++)
    for (int o1=0;o1<cpumeasure[i].num_occur1;o1++)
      for (int o2=0;o2<=cpumeasure[i].num_occur2;o2++)
      {
      prnt("%02d %*s%s%s:  %15.3f us; %15d %s",p,TELNET_MAXMEASURNAME_LEN+7,(cpumeasure+i)->statname,
            stridx(cpumeasure[i].num_occur1,o1,stridx1),stridx(cpumeasure[i].num_occur2,o2,stridx2),
           ((cpumeasure+i+o1+o2)->astatptr->trials!=0)?(((cpumeasure+i+o1+o2)->astatptr->diff)/((cpumeasure+i+o1+o2)->astatptr->trials))/cpufreq/1000:0,
           (cpumeasure+i+o1+o2)->astatptr->trials, ((p%2)==1)?"|\n":"  | " );
      p++;
      }
  prnt("\n\n");
} /* measurcmd_display_measures */


/*----------------------------------------------------------------------------------------------------*/
/* cellular measurements functions                         */
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



/*------------------------------------------------------------------------------------------------------------------------*/
/* function called by the telnet server when measur command is entered */
int measurcmd_show(char *buf, int debug, telnet_printfunc_t prnt) {
  char *subcmd=NULL;
  int idx1, idx2;
  int badcmd=1;

  if (debug > 0)
    prnt(" measurcmd_show received %s\n",buf);
  int (*fptr)(telnet_measurgroupdef_t **) = dlsym(RTLD_DEFAULT,"get_measurgroups");
  if ( fptr == NULL) {
    prnt("No measures available for this softmodem\n");
    return 0;
  }
  telnet_measurgroupdef_t *measurgroups;
  int num_measurgroups = fptr( &measurgroups);

  int s = sscanf(buf,"%ms %i-%i\n",&subcmd, &idx1,&idx2);

  if (s>0) {
    if ( strcmp(subcmd,"inq") == 0) {
                notifiedFIFO_elt_t *msg =newNotifiedFIFO_elt(sizeof(time_stats_msg_t),0,NULL,NULL);
                time_stats_msg_t *msgdata=NotifiedFifoData(msg);
                msgdata->msgid = TIMESTAT_MSGID_DISPLAY;
                msgdata->displayFunc = (void (*)(void *))prnt;
                pushNotifiedFIFO(&measur_fifo, msg);
    } else if ( strcmp(subcmd,"groups") == 0){
      measurcmd_display_groups(prnt,measurgroups,num_measurgroups);
      badcmd=0;
    } else {
      for (int i=0; i<TELNET_NUM_MEASURTYPES; i++) {
        if(strcmp(subcmd,grouptypes[i]) == 0) {
          for(int j=0; j<num_measurgroups; j++) {
            if(i == measurgroups[j].type) {
              badcmd=0;
              measurgroups[j].displayfunc(prnt);
            }
          } /* for j...*/
        }
      }/* for i...*/

      for (int i=0; i<num_measurgroups; i++) {
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
    prnt(" measurcmd_cpustats received %s\n",buf);

  int s = sscanf(buf,"%ms %i-%i\n",&subcmd, &idx1,&idx2);

  if (s>0) {
    if ( strcmp(subcmd,"enable") == 0) {
      
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

void measurcmd_async_help(telnet_printfunc_t prnt) {
}

int measurcmd_async(char *buf, int debug, telnet_printfunc_t prnt) {
  char *subcmd=NULL;
  int idx1, idx2;
  int okcmd=0;

  if (buf == NULL) {
	  measurcmd_async_help(prnt);
	  return CMDSTATUS_FOUND;
  }
  if (debug > 0)
    prnt(" measurcmd_async received %s\n",buf);
  

  int s = sscanf(buf,"%ms %i-%i\n",&subcmd, &idx1,&idx2);

  if (s==1) {
    if ( strcmp(subcmd,"enable") == 0) {
      init_meas();
      okcmd=1;
    } else if ( strcmp(subcmd,"disable") == 0) {
      end_meas();
      okcmd=1;
    }
  } else if ( s == 3 ) {
	int msgid;
    if ( strcmp(subcmd,"enable") == 0) {
      msgid = TIMESTAT_MSGID_ENABLE;
      okcmd=1;
    } else if ( strcmp(subcmd,"disable") == 0) {
      msgid = TIMESTAT_MSGID_DISABLE;
      okcmd=1;
    } else if ( strcmp(subcmd,"display") == 0) {
      msgid = TIMESTAT_MSGID_DISPLAY;
      okcmd=1;
    }
    if (okcmd) {
      notifiedFIFO_elt_t *nfe = newNotifiedFIFO_elt(sizeof(time_stats_msg_t),0,NULL,NULL);
	  time_stats_msg_t *msg = (time_stats_msg_t *)NotifiedFifoData(nfe);
      msg->msgid = msgid ;
      msg->displayFunc = prnt;
	  for(int i=idx1; i<idx2; i++) {
		msg->timestat_id =i;
        pushNotifiedFIFO(&measur_fifo, nfe);
	  }
    }	  
  }

  if (!(okcmd)) {
    prnt("Unknown command: %s\n",buf);
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
