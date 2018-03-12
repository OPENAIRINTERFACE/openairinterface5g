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

/*! \file log.c
* \brief log implementaion
* \author Navid Nikaein
* \date 2009 - 2014
* \version 0.5
* @ingroup util

*/

#define _GNU_SOURCE  /* required for pthread_getname_np */
//#define LOG_TEST 1

#define COMPONENT_LOG
#define COMPONENT_LOG_IF
#include <ctype.h>
#define LOG_MAIN
#include "log.h"
#include "vcd_signal_dumper.h"
#include "assertions.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

# include <pthread.h>
# include <string.h>
#include "common/config/config_userapi.h"
// main log variables


mapping log_level_names[] = {
  {"emerg", LOG_EMERG},
  {"alert", LOG_ALERT},
  {"crit", LOG_CRIT},
  {"error", LOG_ERR},
  {"warn", LOG_WARNING},
  {"notice", LOG_NOTICE},
  {"info", LOG_INFO},
  {"debug", LOG_DEBUG},
  {"file", LOG_FILE},
  {"trace", LOG_TRACE},
  {NULL, -1}
};
mapping log_verbosity_names[] = {
  {"none", 0x0},
  {"low", 0x5},
  {"medium", 0x15},
  {"high", 0x35},
  {"full", 0x75},
  {NULL, -1}
};

// vars for the log thread
LOG_params log_list[2000];
int log_list_tail = 0;
int log_list_nb_elements = 0;

pthread_mutex_t log_lock;
pthread_cond_t log_notify;

#if !defined(LOG_NO_THREAD)
int log_list_head = 0;
int log_shutdown;
#endif

static int gfd;

static char *log_level_highlight_start[] = {LOG_RED, LOG_RED, LOG_RED, LOG_RED, LOG_ORANGE, LOG_BLUE, "", ""};  /*!< \brief Optional start-format strings for highlighting */
static char *log_level_highlight_end[]   = {LOG_RESET, LOG_RESET, LOG_RESET, LOG_RESET, LOG_RESET,LOG_RESET,  "",""};   /*!< \brief Optional end-format strings for highlighting */

#if defined(ENABLE_ITTI)
static log_instance_type_t log_instance_type;
#endif

/* get log parameters from configuration file */
void  log_getconfig(log_t *g_log) {
  char *gloglevel = NULL;
  char *glogverbo = NULL;
  int level,verbosity;
  paramdef_t logparams_defaults[] = LOG_GLOBALPARAMS_DESC;
  paramdef_t logparams_level[MAX_LOG_COMPONENTS];
  paramdef_t logparams_verbosity[MAX_LOG_COMPONENTS];
  paramdef_t logparams_logfile[MAX_LOG_COMPONENTS];
  
  int ret = config_get( logparams_defaults,sizeof(logparams_defaults)/sizeof(paramdef_t),CONFIG_STRING_LOG_PREFIX);
  if (ret <0) {
       fprintf(stderr,"[LOG] init aborted, configuration couldn't be performed");
       return;
  } 
  memset(logparams_level,    0, sizeof(paramdef_t)*MAX_LOG_COMPONENTS);
  memset(logparams_verbosity,0, sizeof(paramdef_t)*MAX_LOG_COMPONENTS);
  memset(logparams_logfile,  0, sizeof(paramdef_t)*MAX_LOG_COMPONENTS);
  for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
    if(g_log->log_component[i].name == NULL) {
       g_log->log_component[i].name = malloc(16);
       sprintf((char *)g_log->log_component[i].name,"comp%i?",i);
       logparams_logfile[i].paramflags = PARAMFLAG_DONOTREAD;
       logparams_level[i].paramflags = PARAMFLAG_DONOTREAD;
       logparams_verbosity[i].paramflags = PARAMFLAG_DONOTREAD;
    }
    sprintf(logparams_level[i].optname,    LOG_CONFIG_LEVEL_FORMAT,       g_log->log_component[i].name);
    sprintf(logparams_verbosity[i].optname,LOG_CONFIG_VERBOSITY_FORMAT,   g_log->log_component[i].name);
    sprintf(logparams_logfile[i].optname,  LOG_CONFIG_LOGFILE_FORMAT,     g_log->log_component[i].name);
/* workaround: all log options in existing configuration files use lower case component names
   where component names include uppercase char in log.h....                                */ 
    for (int j=0 ; j<strlen(logparams_level[i].optname); j++) 
          logparams_level[i].optname[j] = tolower(logparams_level[i].optname[j]);
    for (int j=0 ; j<strlen(logparams_level[i].optname); j++) 
          logparams_verbosity[i].optname[j] = tolower(logparams_verbosity[i].optname[j]);
    for (int j=0 ; j<strlen(logparams_level[i].optname); j++) 
          logparams_logfile[i].optname[j] = tolower(logparams_logfile[i].optname[j]);
/* */
    logparams_level[i].defstrval     = gloglevel;
    logparams_verbosity[i].defstrval = glogverbo; 

    logparams_level[i].type          = TYPE_STRING;
    logparams_verbosity[i].type      = TYPE_STRING;
    logparams_logfile[i].type        = TYPE_UINT;

    logparams_logfile[i].paramflags  = logparams_logfile[i].paramflags|PARAMFLAG_BOOL;
    }
  config_get( logparams_level,    MAX_LOG_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  config_get( logparams_verbosity,MAX_LOG_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  config_get( logparams_logfile,  MAX_LOG_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
    verbosity = map_str_to_int(log_verbosity_names,*(logparams_verbosity[i].strptr));
    level     = map_str_to_int(log_level_names,    *(logparams_level[i].strptr));
    set_comp_log(i, level,verbosity,1);
    set_component_filelog(*(logparams_logfile[i].uptr));
    }
}


int logInit (void)
{
  int i;
  g_log = calloc(1, sizeof(log_t));

  if (g_log == NULL) {
    perror ("cannot allocated memory for log generation module \n");
    exit(EXIT_FAILURE);
  }


#if ! defined(CN_BUILD)
  g_log->log_component[PHY].name = "PHY";
  g_log->log_component[PHY].level = LOG_EMERG;
  g_log->log_component[PHY].flag =  LOG_MED;
  g_log->log_component[PHY].interval =  1;
  g_log->log_component[PHY].fd = 0;
  g_log->log_component[PHY].filelog = 0;
  g_log->log_component[PHY].filelog_name = "/tmp/phy.log";

  g_log->log_component[MAC].name = "MAC";
  g_log->log_component[MAC].level = LOG_EMERG;
  g_log->log_component[MAC].flag =  LOG_MED;
  g_log->log_component[MAC].interval =  1;
  g_log->log_component[MAC].fd = 0;
  g_log->log_component[MAC].filelog = 0;
  g_log->log_component[MAC].filelog_name = "/tmp/mac.log";

  g_log->log_component[OPT].name = "OPT";
  g_log->log_component[OPT].level = LOG_EMERG;
  g_log->log_component[OPT].flag = LOG_MED;
  g_log->log_component[OPT].interval =  1;
  g_log->log_component[OPT].fd = 0;
  g_log->log_component[OPT].filelog = 0;
  g_log->log_component[OPT].filelog_name = "";

  g_log->log_component[RLC].name = "RLC";
  g_log->log_component[RLC].level = LOG_INFO;
  g_log->log_component[RLC].flag = LOG_MED;
  g_log->log_component[RLC].interval =  1;
  g_log->log_component[RLC].fd = 0;
  g_log->log_component[RLC].filelog = 0;
  g_log->log_component[RLC].filelog_name = "/tmp/rlc.log";

  g_log->log_component[PDCP].name = "PDCP";
  g_log->log_component[PDCP].level = LOG_INFO;
  g_log->log_component[PDCP].flag = LOG_MED;
  g_log->log_component[PDCP].interval =  1;
  g_log->log_component[PDCP].fd = 0;
  g_log->log_component[PDCP].filelog = 0;
  g_log->log_component[PDCP].filelog_name = "/tmp/pdcp.log";

  g_log->log_component[RRC].name = "RRC";
  g_log->log_component[RRC].level = LOG_TRACE;
  g_log->log_component[RRC].flag = LOG_MED;
  g_log->log_component[RRC].interval =  1;
  g_log->log_component[RRC].fd = 0;
  g_log->log_component[RRC].filelog = 0;
  g_log->log_component[RRC].filelog_name = "/tmp/rrc.log";

  g_log->log_component[EMU].name = "EMU";
  g_log->log_component[EMU].level = LOG_EMERG;
  g_log->log_component[EMU].flag =  LOG_MED;
  g_log->log_component[EMU].interval =  1;
  g_log->log_component[EMU].fd = 0;
  g_log->log_component[EMU].filelog = 0;
  g_log->log_component[EMU].filelog_name = "";

  g_log->log_component[OMG].name = "OMG";
  g_log->log_component[OMG].level = LOG_EMERG;
  g_log->log_component[OMG].flag =  LOG_MED;
  g_log->log_component[OMG].interval =  1;
  g_log->log_component[OMG].fd = 0;
  g_log->log_component[OMG].filelog = 0;
  g_log->log_component[OMG].filelog_name = "/tmp/omg.csv";

  g_log->log_component[OTG].name = "OTG";
  g_log->log_component[OTG].level = LOG_EMERG;
  g_log->log_component[OTG].flag =  LOG_MED;
  g_log->log_component[OTG].interval =  1;
  g_log->log_component[OTG].fd = 0;
  g_log->log_component[OTG].filelog = 0;
  g_log->log_component[OTG].filelog_name = "/tmp/otg.log";

  g_log->log_component[OTG_LATENCY].name = "OTG_LATENCY";
  g_log->log_component[OTG_LATENCY].level = LOG_EMERG;
  g_log->log_component[OTG_LATENCY].flag =  LOG_MED;
  g_log->log_component[OTG_LATENCY].interval =  1;
  g_log->log_component[OTG_LATENCY].fd = 0;
  g_log->log_component[OTG_LATENCY].filelog = 0;
  g_log->log_component[OTG_LATENCY].filelog_name = "/tmp/otg_latency.dat";

  g_log->log_component[OTG_LATENCY_BG].name = "OTG_LATENCY_BG";
  g_log->log_component[OTG_LATENCY_BG].level = LOG_EMERG;
  g_log->log_component[OTG_LATENCY_BG].flag =  LOG_MED;
  g_log->log_component[OTG_LATENCY_BG].interval =  1;
  g_log->log_component[OTG_LATENCY_BG].fd = 0;
  g_log->log_component[OTG_LATENCY_BG].filelog = 0;
  g_log->log_component[OTG_LATENCY_BG].filelog_name = "/tmp/otg_latency_bg.dat";

  g_log->log_component[OTG_GP].name = "OTG_GP";
  g_log->log_component[OTG_GP].level = LOG_EMERG;
  g_log->log_component[OTG_GP].flag =  LOG_MED;
  g_log->log_component[OTG_GP].interval =  1;
  g_log->log_component[OTG_GP].fd = 0;
  g_log->log_component[OTG_GP].filelog = 0;
  g_log->log_component[OTG_GP].filelog_name = "/tmp/otg_goodput.dat";

  g_log->log_component[OTG_GP_BG].name = "OTG_GP_BG";
  g_log->log_component[OTG_GP_BG].level = LOG_EMERG;
  g_log->log_component[OTG_GP_BG].flag =  LOG_MED;
  g_log->log_component[OTG_GP_BG].interval =  1;
  g_log->log_component[OTG_GP_BG].fd = 0;
  g_log->log_component[OTG_GP_BG].filelog = 0;
  g_log->log_component[OTG_GP_BG].filelog_name = "/tmp/otg_goodput_bg.dat";

  g_log->log_component[OTG_JITTER].name = "OTG_JITTER";
  g_log->log_component[OTG_JITTER].level = LOG_EMERG;
  g_log->log_component[OTG_JITTER].flag =  LOG_MED;
  g_log->log_component[OTG_JITTER].interval =  1;
  g_log->log_component[OTG_JITTER].fd = 0;
  g_log->log_component[OTG_JITTER].filelog = 0;
  g_log->log_component[OTG_JITTER].filelog_name = "/tmp/otg_jitter.dat";

  g_log->log_component[OCG].name = "OCG";
  g_log->log_component[OCG].level = LOG_EMERG;
  g_log->log_component[OCG].flag =  LOG_MED;
  g_log->log_component[OCG].interval =  1;
  g_log->log_component[OCG].fd = 0;
  g_log->log_component[OCG].filelog = 0;
  g_log->log_component[OCG].filelog_name = "";

  g_log->log_component[PERF].name = "PERF";
  g_log->log_component[PERF].level = LOG_EMERG;
  g_log->log_component[PERF].flag =  LOG_MED;
  g_log->log_component[PERF].interval =  1;
  g_log->log_component[PERF].fd = 0;
  g_log->log_component[PERF].filelog = 0;
  g_log->log_component[PERF].filelog_name = "";

  g_log->log_component[OIP].name = "OIP";
  g_log->log_component[OIP].level = LOG_EMERG;
  g_log->log_component[OIP].flag =  LOG_MED;
  g_log->log_component[OIP].interval =  1;
  g_log->log_component[OIP].fd = 0;
  g_log->log_component[OIP].filelog = 0;
  g_log->log_component[OIP].filelog_name = "";

  g_log->log_component[CLI].name = "CLI";
  g_log->log_component[CLI].level = LOG_EMERG;
  g_log->log_component[CLI].flag =  LOG_MED;
  g_log->log_component[CLI].interval =  1;
  g_log->log_component[CLI].fd = 0;
  g_log->log_component[CLI].filelog =  0;
  g_log->log_component[CLI].filelog_name = "";

  g_log->log_component[MSC].name = "MSC";
  g_log->log_component[MSC].level = LOG_EMERG;
  g_log->log_component[MSC].flag =  LOG_MED;
  g_log->log_component[MSC].interval =  1;
  g_log->log_component[MSC].fd = 0;
  g_log->log_component[MSC].filelog =  0;
  g_log->log_component[MSC].filelog_name = "/tmp/msc.log";

  g_log->log_component[OCM].name = "OCM";
  g_log->log_component[OCM].level = LOG_EMERG;
  g_log->log_component[OCM].flag =  LOG_MED;
  g_log->log_component[OCM].interval =  1;
  g_log->log_component[OCM].fd = 0;
  g_log->log_component[OCM].filelog =  0;
  g_log->log_component[OCM].filelog_name = "/tmp/ocm.log";

  g_log->log_component[HW].name = "HW";
  g_log->log_component[HW].level = LOG_EMERG;
  g_log->log_component[HW].flag = LOG_MED;
  g_log->log_component[HW].interval = 1;
  g_log->log_component[HW].fd = 0;
  g_log->log_component[HW].filelog = 0;
  g_log->log_component[HW].filelog_name = "";

  g_log->log_component[OSA].name = "OSA";
  g_log->log_component[OSA].level = LOG_EMERG;
  g_log->log_component[OSA].flag = LOG_MED;
  g_log->log_component[OSA].interval = 1;
  g_log->log_component[OSA].fd = 0;
  g_log->log_component[OSA].filelog = 0;
  g_log->log_component[OSA].filelog_name = "";

  g_log->log_component[RAL_ENB].name = "eRAL";
  g_log->log_component[RAL_ENB].level = LOG_EMERG;
  g_log->log_component[RAL_ENB].flag = LOG_MED;
  g_log->log_component[RAL_ENB].interval = 1;
  g_log->log_component[RAL_ENB].fd = 0;
  g_log->log_component[RAL_ENB].filelog = 0;
  g_log->log_component[RAL_ENB].filelog_name = "";

  g_log->log_component[RAL_UE].name = "mRAL";
  g_log->log_component[RAL_UE].level = LOG_EMERG;
  g_log->log_component[RAL_UE].flag = LOG_MED;
  g_log->log_component[RAL_UE].interval = 1;
  g_log->log_component[RAL_UE].fd = 0;
  g_log->log_component[RAL_UE].filelog = 0;
  g_log->log_component[RAL_UE].filelog_name = "";

  g_log->log_component[ENB_APP].name = "ENB_APP";
  g_log->log_component[ENB_APP].level = LOG_EMERG;
  g_log->log_component[ENB_APP].flag = LOG_MED;
  g_log->log_component[ENB_APP].interval = 1;
  g_log->log_component[ENB_APP].fd = 0;
  g_log->log_component[ENB_APP].filelog = 0;
  g_log->log_component[ENB_APP].filelog_name = "";

  g_log->log_component[FLEXRAN_AGENT].name = "FLEXRAN_AGENT";
  g_log->log_component[FLEXRAN_AGENT].level = LOG_DEBUG;
  g_log->log_component[FLEXRAN_AGENT].flag = LOG_MED;
  g_log->log_component[FLEXRAN_AGENT].interval = 1;
  g_log->log_component[FLEXRAN_AGENT].fd = 0;
  g_log->log_component[FLEXRAN_AGENT].filelog = 0;
  g_log->log_component[FLEXRAN_AGENT].filelog_name = "";
  
  g_log->log_component[TMR].name = "TMR";
  g_log->log_component[TMR].level = LOG_EMERG;
  g_log->log_component[TMR].flag = LOG_MED;
  g_log->log_component[TMR].interval = 1;
  g_log->log_component[TMR].fd = 0;
  g_log->log_component[TMR].filelog = 0;
  g_log->log_component[TMR].filelog_name = "";

  g_log->log_component[USIM].name = "USIM";
  g_log->log_component[USIM].level = LOG_DEBUG;
  g_log->log_component[USIM].flag = LOG_NONE;
  g_log->log_component[USIM].interval = 1;
  g_log->log_component[USIM].fd = 0;
  g_log->log_component[USIM].filelog = 0;
  g_log->log_component[USIM].filelog_name = "/tmp/usim.txt";

  /* following log component are used for the localization*/
  g_log->log_component[LOCALIZE].name = "LOCALIZE";
  g_log->log_component[LOCALIZE].level = LOG_EMERG;
  g_log->log_component[LOCALIZE].flag =  LOG_MED;
  g_log->log_component[LOCALIZE].interval =  1;
  g_log->log_component[LOCALIZE].fd = 0;
  g_log->log_component[LOCALIZE].filelog = 0;
  g_log->log_component[LOCALIZE].filelog_name = "/tmp/localize.log";
#endif // ! defined(CN_BUILD)

  g_log->log_component[NAS].name = "NAS";
  g_log->log_component[NAS].level = LOG_TRACE;
  g_log->log_component[NAS].flag = LOG_MED;
  g_log->log_component[NAS].interval =  1;
  g_log->log_component[NAS].fd = 0;
  g_log->log_component[NAS].filelog = 0;
  g_log->log_component[NAS].filelog_name = "/tmp/nas.log";

  g_log->log_component[UDP_].name = "UDP";
  g_log->log_component[UDP_].level = LOG_EMERG;
  g_log->log_component[UDP_].flag = LOG_FULL;
  g_log->log_component[UDP_].interval = 1;
  g_log->log_component[UDP_].fd = 0;
  g_log->log_component[UDP_].filelog = 0;
  g_log->log_component[UDP_].filelog_name = "";

  g_log->log_component[GTPU].name = "GTPV1U";
  g_log->log_component[GTPU].level = LOG_EMERG;
  g_log->log_component[GTPU].flag = LOG_FULL;
  g_log->log_component[GTPU].interval = 1;
  g_log->log_component[GTPU].fd = 0;
  g_log->log_component[GTPU].filelog = 0;
  g_log->log_component[GTPU].filelog_name = "";

  g_log->log_component[S1AP].name = "S1AP";
  g_log->log_component[S1AP].level = LOG_EMERG;
  g_log->log_component[S1AP].flag = LOG_FULL;
  g_log->log_component[S1AP].interval = 1;
  g_log->log_component[S1AP].fd = 0;
  g_log->log_component[S1AP].filelog = 0;
  g_log->log_component[S1AP].filelog_name = "";

  g_log->log_component[SCTP].name = "SCTP";
  g_log->log_component[SCTP].level = LOG_EMERG;
  g_log->log_component[SCTP].flag = LOG_MED;
  g_log->log_component[SCTP].interval = 1;
  g_log->log_component[SCTP].fd = 0;
  g_log->log_component[SCTP].filelog = 0;
  g_log->log_component[SCTP].filelog_name = "";
 
  g_log->log_component[RRH].name = "RRH";
  g_log->log_component[RRH].level = LOG_EMERG;
  g_log->log_component[RRH].flag = LOG_MED;
  g_log->log_component[RRH].interval = 1;
  g_log->log_component[RRH].fd = 0;
  g_log->log_component[RRH].filelog = 0;
  g_log->log_component[RRH].filelog_name = "";
  
  g_log->level2string[LOG_EMERG]         = "G"; //EMERG
  g_log->level2string[LOG_ALERT]         = "A"; // ALERT
  g_log->level2string[LOG_CRIT]          = "C"; // CRITIC
  g_log->level2string[LOG_ERR]           = "E"; // ERROR
  g_log->level2string[LOG_WARNING]       = "W"; // WARNING
  g_log->level2string[LOG_NOTICE]        = "N"; // NOTICE
  g_log->level2string[LOG_INFO]          = "I"; //INFO
  g_log->level2string[LOG_DEBUG]         = "D"; // DEBUG
  g_log->level2string[LOG_FILE]          = "F"; // file
  g_log->level2string[LOG_TRACE]         = "T"; // TRACE

  g_log->onlinelog = 1; //online log file
  g_log->syslog = 0;
  g_log->filelog   = 0;
  g_log->level  = LOG_TRACE;
  g_log->flag   = LOG_LOW;

  g_log->config.remote_ip      = 0;
  g_log->config.remote_level   = LOG_EMERG;
  g_log->config.facility       = LOG_LOCAL7;
  g_log->config.audit_ip       = 0;
  g_log->config.audit_facility = LOG_LOCAL6;
  g_log->config.format         = 0x00; // online debug inactive

  g_log->filelog_name = "/tmp/openair.log";

  if (g_log->syslog) {
#if ! defined(CN_BUILD)
    openlog(g_log->log_component[EMU].name, LOG_PID, g_log->config.facility);
#endif // ! defined(CN_BUILD)
  }
  log_getconfig(g_log);
  if (g_log->filelog) {
    gfd = open(g_log->filelog_name, O_WRONLY | O_CREAT, 0666);
  }

  // could put a loop here to check for all comps
  for (i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
    if (g_log->log_component[i].filelog == 1 ) {
      g_log->log_component[i].fd = open(g_log->log_component[i].filelog_name,
                                        O_WRONLY | O_CREAT | O_APPEND, 0666);
    }
  }

  printf("log init done\n");

  return 0;
}

void nfapi_log(char *file, char *func, int line, int comp, int level, const char* format, va_list args)
{
  //logRecord_mt(file,func,line, pthread_self(), comp, level, format, ##args)
  int len = 0;
  log_component_t *c;
  char *log_start;
  char *log_end;
  /* The main difference with the version above is the use of this local log_buffer.
   * The other difference is the return value of snprintf which was not used
   * correctly. It was not a big problem because in practice MAX_LOG_TOTAL is
   * big enough so that the buffer is never full.
   */
  char log_buffer[MAX_LOG_TOTAL];

  /* for no gcc warnings */
  (void)log_start;
  (void)log_end;

  c = &g_log->log_component[comp];

  // do not apply filtering for LOG_F
  // only log messages which are enabled and are below the global log level and component's level threshold
  if ((level != LOG_FILE) && ((level > c->level) || (level > g_log->level))) {
    /* if ((level != LOG_FILE) &&
          ((level > c->level) ||
           (level > g_log->level) ||
           ( c->level > g_log->level))) {
    */
    return;
  }

  //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD, VCD_FUNCTION_IN);

  // adjust syslog level for TRACE messages
  if (g_log->syslog) {
    if (g_log->level > LOG_DEBUG) {
      g_log->level = LOG_DEBUG;
    }
  }

  // make sure that for log trace the extra info is only printed once, reset when the level changes
  if ((level == LOG_FILE) || (c->flag == LOG_NONE) || (level == LOG_TRACE)) {
    log_start = log_buffer;
    len = vsnprintf(log_buffer, MAX_LOG_TOTAL, format, args);
    if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    log_end = log_buffer + len;
  } else {
    if ( (g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "%s",
                      log_level_highlight_start[level]);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    log_start = log_buffer + len;

    if ( (g_log->flag & FLAG_COMP) || (c->flag & FLAG_COMP) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s]",
                      g_log->log_component[comp].name);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    if ( (g_log->flag & FLAG_LEVEL) || (c->flag & FLAG_LEVEL) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s]",
                      g_log->level2string[level]);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    if ( (g_log->flag & FLAG_THREAD) || (c->flag & FLAG_THREAD) ) {
#     define THREAD_NAME_LEN 128
      char threadname[THREAD_NAME_LEN];
      if (pthread_getname_np(pthread_self(), threadname, THREAD_NAME_LEN) != 0)
      {
        perror("pthread_getname_np : ");
      } else {
        len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s]", threadname);
        if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
      }
#     undef THREAD_NAME_LEN
    }

    if ( (g_log->flag & FLAG_FUNCT) || (c->flag & FLAG_FUNCT) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s] ",
                      func);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    if ( (g_log->flag & FLAG_FILE_LINE) || (c->flag & FLAG_FILE_LINE) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s:%d]",
                      file, line);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    //len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%08lx]", thread_id);
    //if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;

    len += vsnprintf(&log_buffer[len], MAX_LOG_TOTAL - len, format, args);
    if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    log_end = log_buffer + len;

    if ( (g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "%s",
          log_level_highlight_end[level]);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }
  }

  va_end(args);

  // OAI printf compatibility
  if ((g_log->onlinelog == 1) && (level != LOG_FILE))
#ifdef RTAI
    if (len > MAX_LOG_TOTAL) {
      rt_printk ("[OPENAIR] FIFO_PRINTF WROTE OUTSIDE ITS MEMORY BOUNDARY : ERRORS WILL OCCUR\n");
    }

  if (len > 0) {
    rtf_put (FIFO_PRINTF_NO, log_buffer, len);
  }

#else
  fwrite(log_buffer, len, 1, stdout);
#endif

#ifndef RTAI

  if (g_log->syslog) {
    syslog(g_log->level, "%s", log_buffer);
  }

  if (g_log->filelog) {
    if (write(gfd, log_buffer, len) < len) {
      // TODO assert ?
    }
  }

  if ((g_log->log_component[comp].filelog) && (level == LOG_FILE)) {
    if (write(g_log->log_component[comp].fd, log_buffer, len) < len) {
      // TODO assert ?
    }
  }

#else

  // online print messges
  if ((g_log->log_component[comp].filelog) && (level == LOG_FILE)) {
    printf(log_buffer);
  }

#endif

#if defined(ENABLE_ITTI)

  if (level <= LOG_DEBUG) {
    task_id_t origin_task_id = TASK_UNKNOWN;
    MessagesIds messages_id;
    MessageDef *message_p;
    size_t      message_string_size;
    char       *message_msg_p;

    message_string_size = log_end - log_start;

#if !defined(DISABLE_ITTI_DETECT_SUB_TASK_ID)

    /* Try to identify sub task ID from log information (comp, log_instance_type) */
    switch (comp) {
    case PHY:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_PHY_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_PHY_UE;
        break;

      default:
        break;
      }

      break;

    case MAC:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_MAC_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_MAC_UE;

      default:
        break;
      }

      break;

    case RLC:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_RLC_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_RLC_UE;

      default:
        break;
      }

      break;

    case PDCP:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_PDCP_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_PDCP_UE;

      default:
        break;
      }

      break;

    default:
      break;
    }

#endif

    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      messages_id = ERROR_LOG;
      break;

    case LOG_WARNING:
      messages_id = WARNING_LOG;
      break;

    case LOG_NOTICE:
      messages_id = NOTICE_LOG;
      break;

    case LOG_INFO:
      messages_id = INFO_LOG;
      break;

    default:
      messages_id = DEBUG_LOG;
      break;
    }

    message_p = itti_alloc_new_message_sized(origin_task_id, messages_id, message_string_size);

    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      message_msg_p = (char *) &message_p->ittiMsg.error_log;
      break;

    case LOG_WARNING:
      message_msg_p = (char *) &message_p->ittiMsg.warning_log;
      break;

    case LOG_NOTICE:
      message_msg_p = (char *) &message_p->ittiMsg.notice_log;
      break;

    case LOG_INFO:
      message_msg_p = (char *) &message_p->ittiMsg.info_log;
      break;

    default:
      message_msg_p = (char *) &message_p->ittiMsg.debug_log;
      break;
    }

    memcpy(message_msg_p, log_start, message_string_size);

    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

#endif
#if 0
  LOG_params log_params;
  int        len;

  len = vsnprintf(log_params.l_buff_info, MAX_LOG_INFO-1, format, args);

  //2 first parameters must be passed as 'const' to the thread function
  log_params.file = strdup(file);
  log_params.func = strdup(func);
  log_params.line = line;
  log_params.comp = PHY;//comp;
  log_params.level = 6; // INFO - level;
  log_params.format = format;
  log_params.len = len;

  if (pthread_mutex_lock(&log_lock) != 0) {
    return;
  }

  log_list_tail++;
  log_list[log_list_tail - 1] = log_params;

  if (log_list_tail >= 1000) {
    log_list_tail = 0;
  }

  if (log_list_nb_elements < 1000) {
    log_list_nb_elements++;
  }

  if(pthread_cond_signal(&log_notify) != 0) {
    pthread_mutex_unlock(&log_lock);
    return;
  }

  if(pthread_mutex_unlock(&log_lock) != 0) {
    return;
  }

#endif
}

//log record: add to a list
void logRecord(const char *file, const char *func, int line,  int comp,
               int level, const char *format, ...)
{
  va_list    args;
  LOG_params log_params;
  int        len;

  va_start(args, format);
  len = vsnprintf(log_params.l_buff_info, MAX_LOG_INFO-1, format, args);
  va_end(args);

  //2 first parameters must be passed as 'const' to the thread function
  log_params.file = strdup(file);
  log_params.func = strdup(func);
  log_params.line = line;
  log_params.comp = comp;
  log_params.level = level;
  log_params.format = format;
  log_params.len = len;

  if (pthread_mutex_lock(&log_lock) != 0) {
    return;
  }

  log_list_tail++;
  log_list[log_list_tail - 1] = log_params;

  if (log_list_tail >= 1000) {
    log_list_tail = 0;
  }

  if (log_list_nb_elements < 1000) {
    log_list_nb_elements++;
  }

  if(pthread_cond_signal(&log_notify) != 0) {
    pthread_mutex_unlock(&log_lock);
    return;
  }

  if(pthread_mutex_unlock(&log_lock) != 0) {
    return;
  }

  //log = malloc(sizeof(LOG_elt));
  //log->next = NULL;
  //log->log_params = log_params;
  /* Add log task to queue */
  //log_list_add_tail_eurecom(log, &log_list);

}

void logRecord_thread_safe(const char *file, const char *func,
                           int line,  int comp, int level,
                           int len, const char *params_string)
{
  log_component_t *c;
  int total_len = 0;
  char log_buffer[MAX_LOG_TOTAL];

  c = &g_log->log_component[comp];

  // do not apply filtering for LOG_F
  // only log messages which are enabled and are below the global log level and component's level threshold
  if ((level != LOG_FILE) && ((c->level > g_log->level) ||
                              (level > c->level) || (level > g_log->level))) {
    return;
  }


  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD,
                                          VCD_FUNCTION_IN);

  // adjust syslog level for TRACE messages
  if (g_log->syslog) {
    if (g_log->level > LOG_DEBUG) {
      g_log->level = LOG_DEBUG;
    }
  }

  // make sure that for log trace the extra info is only printed once, reset when the level changes
  if ((level == LOG_FILE) ||  (c->flag == LOG_NONE)  || (level ==LOG_TRACE )) {
    total_len = snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - 1, "%s",
                         params_string);
  } else {
    if ((g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR)) {
      total_len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - total_len, "%s",
                            log_level_highlight_start[level]);
    }

    if ((g_log->flag & FLAG_COMP) || (c->flag & FLAG_COMP) ) {
      total_len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - total_len, "[%s]",
                            g_log->log_component[comp].name);
    }

    if ((g_log->flag & FLAG_LEVEL) || (c->flag & FLAG_LEVEL)) {
      total_len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - total_len, "[%s]",
                            g_log->level2string[level]);
    }

    if ((g_log->flag & FLAG_FUNCT) || (c->flag & FLAG_FUNCT)) {
      total_len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - total_len, "[%s] ",
                            func);
    }

    if ((g_log->flag & FLAG_FILE_LINE) || (c->flag & FLAG_FILE_LINE) )  {
      total_len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - total_len, "[%s:%d]",
                            file, line);
    }

    len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - len, "%s",
                    params_string);

    if ((g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR)) {
      total_len += snprintf(&log_buffer[total_len], MAX_LOG_TOTAL - total_len, "%s",
                            log_level_highlight_end[level]);
    }
  }

  // OAI printf compatibility
  if ((g_log->onlinelog == 1) && (level != LOG_FILE)) {
    fprintf(stdout, "%s", log_buffer);
  }

  if (g_log->syslog) {
    syslog(g_log->level, "%s", log_buffer);
  }

  if (g_log->filelog) {
    if (write(gfd, log_buffer, total_len) < total_len) {
      // TODO assert ?
    }
  }

  if ((g_log->log_component[comp].filelog) && (level == LOG_FILE)) {
    if (write(g_log->log_component[comp].fd, log_buffer, total_len) < total_len) {
      // TODO assert ?
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD,
                                          VCD_FUNCTION_OUT);
}

#if !defined(LOG_NO_THREAD)
void *log_thread_function(void *list)
{

  LOG_params log_params;

  for(;;) {

    //log_elt = NULL;

    /* Lock must be taken to wait on conditional variable */
    pthread_mutex_lock(&log_lock);

    /* Wait on condition variable, check for spurious wakeups.
       When returning from pthread_cond_wait(), we own the lock. */

    // sleep
    while((log_list_nb_elements == 0) && (log_shutdown == 0)) {
      pthread_cond_wait(&log_notify, &log_lock);
    }

    // exit
    if ((log_shutdown==1) && (log_list_nb_elements == 0)) {
      break;
    }

    /* Grab our task */
    //log_elt = log_list_remove_head(&log_list);
    log_params = log_list[log_list_head];
    log_list_head++;
    log_list_nb_elements--;

    if (log_list_head >= 1000) {
      log_list_head = 0;
    }


    /* Unlock */
    pthread_mutex_unlock(&log_lock);

    /* Get to work */
    logRecord_thread_safe(log_params.file,
                          log_params.func,
                          log_params.line,
                          log_params.comp,
                          log_params.level,
                          log_params.len,
                          log_params.l_buff_info);

    //free(log_elt);
  }
}
#endif

#if 0
/* This function does not work. When multiple threads call it at the same time
 * for the same component, both threads end up using the same buffer.
 * The output is the completely messed up/wrong.
 * Kept in case the fix below is not correct or acceptable.
 */
//log record, format, and print:  executed in the main thread (mt)
void logRecord_mt(const char *file, const char *func, int line, int comp,
                  int level, const char *format, ...)
{
  int len = 0;
  va_list args;
  log_component_t *c;
  char *log_start;
  char *log_end;

  /* for no gcc warnings */
  (void)log_start;
  (void)log_end;

  c = &g_log->log_component[comp];

  // do not apply filtering for LOG_F
  // only log messages which are enabled and are below the global log level and component's level threshold
  if ((level != LOG_FILE) && ((level > c->level) || (level > g_log->level))) {
    /* if ((level != LOG_FILE) &&
          ((level > c->level) ||
           (level > g_log->level) ||
           ( c->level > g_log->level))) {
    */
    return;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD,
                                          VCD_FUNCTION_IN);

  va_start(args, format);

  // adjust syslog level for TRACE messages
  if (g_log->syslog) {
    if (g_log->level > LOG_DEBUG) {
      g_log->level = LOG_DEBUG;
    }
  }

  // make sure that for log trace the extra info is only printed once, reset when the level changes
  if ((level == LOG_FILE) || (c->flag == LOG_NONE) || (level == LOG_TRACE)) {
    log_start = c->log_buffer;
    len = vsnprintf(c->log_buffer, MAX_LOG_TOTAL-1, format, args);
    log_end = c->log_buffer + len;
  } else {
    if ( (g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR) ) {
      len += snprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, "%s",
                      log_level_highlight_start[level]);
    }

    log_start = c->log_buffer + len;

    if ( (g_log->flag & FLAG_COMP) || (c->flag & FLAG_COMP) ) {
      len += snprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, "[%s]",
                      g_log->log_component[comp].name);
    }

    if ( (g_log->flag & FLAG_LEVEL) || (c->flag & FLAG_LEVEL) ) {
      len += snprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, "[%s]",
                      g_log->level2string[level]);
    }

    if ( (g_log->flag & FLAG_FUNCT) || (c->flag & FLAG_FUNCT) ) {
      len += snprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, "[%s] ",
                      func);
    }

    if ( (g_log->flag & FLAG_FILE_LINE) || (c->flag & FLAG_FILE_LINE) ) {
      len += snprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, "[%s:%d]",
                      file, line);
    }

    len += vsnprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, format, args);
    log_end = c->log_buffer + len;

    if ( (g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR) ) {
      len += snprintf(&c->log_buffer[len], MAX_LOG_TOTAL - len, "%s",
                      log_level_highlight_end[level]);
    }
  }

  va_end(args);

  // OAI printf compatibility
  if ((g_log->onlinelog == 1) && (level != LOG_FILE))
    fwrite(c->log_buffer, len, 1, stdout);

  if (g_log->syslog) {
    syslog(g_log->level, "%s", c->log_buffer);
  }

  if (g_log->filelog) {
    if (write(gfd, c->log_buffer, len) < len) {
      // TODO assert ?
    }
  }

  if ((g_log->log_component[comp].filelog) && (level == LOG_FILE)) {
    if (write(g_log->log_component[comp].fd, c->log_buffer, len) < len) {
      // TODO assert ?
    }
  }

#if defined(ENABLE_ITTI)

  if (level <= LOG_DEBUG) {
    task_id_t origin_task_id = TASK_UNKNOWN;
    MessagesIds messages_id;
    MessageDef *message_p;
    size_t      message_string_size;
    char       *message_msg_p;

    message_string_size = log_end - log_start;

#if !defined(DISABLE_ITTI_DETECT_SUB_TASK_ID)

    /* Try to identify sub task ID from log information (comp, log_instance_type) */
    switch (comp) {
    case PHY:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_PHY_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_PHY_UE;
        break;

      default:
        break;
      }

      break;

    case MAC:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_MAC_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_MAC_UE;

      default:
        break;
      }

      break;

    case RLC:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_RLC_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_RLC_UE;

      default:
        break;
      }

      break;

    case PDCP:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_PDCP_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_PDCP_UE;

      default:
        break;
      }

      break;

    default:
      break;
    }

#endif

    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      messages_id = ERROR_LOG;
      break;

    case LOG_WARNING:
      messages_id = WARNING_LOG;
      break;

    case LOG_NOTICE:
      messages_id = NOTICE_LOG;
      break;

    case LOG_INFO:
      messages_id = INFO_LOG;
      break;

    default:
      messages_id = DEBUG_LOG;
      break;
    }

    message_p = itti_alloc_new_message_sized(origin_task_id, messages_id, message_string_size);

    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      message_msg_p = (char *) &message_p->ittiMsg.error_log;
      break;

    case LOG_WARNING:
      message_msg_p = (char *) &message_p->ittiMsg.warning_log;
      break;

    case LOG_NOTICE:
      message_msg_p = (char *) &message_p->ittiMsg.notice_log;
      break;

    case LOG_INFO:
      message_msg_p = (char *) &message_p->ittiMsg.info_log;
      break;

    default:
      message_msg_p = (char *) &message_p->ittiMsg.debug_log;
      break;
    }

    memcpy(message_msg_p, log_start, message_string_size);

    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD,
                                          VCD_FUNCTION_OUT);
}
#endif /* #if 0 */

//log record, format, and print:  executed in the main thread (mt)
void logRecord_mt(const char *file, const char *func, int line, int comp,
                  int level, const char *format, ...)
{
  int len = 0;
  va_list args;
  log_component_t *c;
  char *log_start;
  char *log_end;
  /* The main difference with the version above is the use of this local log_buffer.
   * The other difference is the return value of snprintf which was not used
   * correctly. It was not a big problem because in practice MAX_LOG_TOTAL is
   * big enough so that the buffer is never full.
   */
  char log_buffer[MAX_LOG_TOTAL];

  /* for no gcc warnings */
  (void)log_start;
  (void)log_end;

  c = &g_log->log_component[comp];

  // do not apply filtering for LOG_F
  // only log messages which are enabled and are below the global log level and component's level threshold
  if ((level != LOG_FILE) && ((level > c->level) || (level > g_log->level))) {
    /* if ((level != LOG_FILE) &&
          ((level > c->level) ||
           (level > g_log->level) ||
           ( c->level > g_log->level))) {
    */
    return;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD,
                                          VCD_FUNCTION_IN);

  va_start(args, format);

  // adjust syslog level for TRACE messages
  if (g_log->syslog) {
    if (g_log->level > LOG_DEBUG) {
      g_log->level = LOG_DEBUG;
    }
  }

  // make sure that for log trace the extra info is only printed once, reset when the level changes
  if ((level == LOG_FILE) || (c->flag == LOG_NONE) || (level == LOG_TRACE)) {
    log_start = log_buffer;
    len = vsnprintf(log_buffer, MAX_LOG_TOTAL, format, args);
    if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    log_end = log_buffer + len;
  } else {
    if ( (g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "%s",
                      log_level_highlight_start[level]);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    log_start = log_buffer + len;

    if ( (g_log->flag & FLAG_COMP) || (c->flag & FLAG_COMP) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s]",
                      g_log->log_component[comp].name);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    if ( (g_log->flag & FLAG_LEVEL) || (c->flag & FLAG_LEVEL) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s]",
                      g_log->level2string[level]);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    if ( (g_log->flag & FLAG_THREAD) || (c->flag & FLAG_THREAD) ) {
#     define THREAD_NAME_LEN 128
      char threadname[THREAD_NAME_LEN];
      if (pthread_getname_np(pthread_self(), threadname, THREAD_NAME_LEN) != 0)
      {
        perror("pthread_getname_np : ");
      } else {
        len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s]", threadname);
        if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
      }
#     undef THREAD_NAME_LEN
    }

    if ( (g_log->flag & FLAG_FUNCT) || (c->flag & FLAG_FUNCT) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s] ",
                      func);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    if ( (g_log->flag & FLAG_FILE_LINE) || (c->flag & FLAG_FILE_LINE) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "[%s:%d]",
                      file, line);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }

    len += vsnprintf(&log_buffer[len], MAX_LOG_TOTAL - len, format, args);
    if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    log_end = log_buffer + len;

    if ( (g_log->flag & FLAG_COLOR) || (c->flag & FLAG_COLOR) ) {
      len += snprintf(&log_buffer[len], MAX_LOG_TOTAL - len, "%s",
                      log_level_highlight_end[level]);
      if (len > MAX_LOG_TOTAL) len = MAX_LOG_TOTAL;
    }
  }

  va_end(args);

  // OAI printf compatibility
  if ((g_log->onlinelog == 1) && (level != LOG_FILE))
    fwrite(log_buffer, len, 1, stdout);

  if (g_log->syslog) {
    syslog(g_log->level, "%s", log_buffer);
  }

  if (g_log->filelog) {
    if (write(gfd, log_buffer, len) < len) {
      // TODO assert ?
    }
  }

  if ((g_log->log_component[comp].filelog) && (level == LOG_FILE)) {
    if (write(g_log->log_component[comp].fd, log_buffer, len) < len) {
      // TODO assert ?
    }
  }

#if defined(ENABLE_ITTI)

  if (level <= LOG_DEBUG) {
    task_id_t origin_task_id = TASK_UNKNOWN;
    MessagesIds messages_id;
    MessageDef *message_p;
    size_t      message_string_size;
    char       *message_msg_p;

    message_string_size = log_end - log_start;

#if !defined(DISABLE_ITTI_DETECT_SUB_TASK_ID)

    /* Try to identify sub task ID from log information (comp, log_instance_type) */
    switch (comp) {
    case PHY:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_PHY_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_PHY_UE;
        break;

      default:
        break;
      }

      break;

    case MAC:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_MAC_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_MAC_UE;

      default:
        break;
      }

      break;

    case RLC:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_RLC_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_RLC_UE;

      default:
        break;
      }

      break;

    case PDCP:
      switch (log_instance_type) {
      case LOG_INSTANCE_ENB:
        origin_task_id = TASK_PDCP_ENB;
        break;

      case LOG_INSTANCE_UE:
        origin_task_id = TASK_PDCP_UE;

      default:
        break;
      }

      break;

    default:
      break;
    }

#endif

    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      messages_id = ERROR_LOG;
      break;

    case LOG_WARNING:
      messages_id = WARNING_LOG;
      break;

    case LOG_NOTICE:
      messages_id = NOTICE_LOG;
      break;

    case LOG_INFO:
      messages_id = INFO_LOG;
      break;

    default:
      messages_id = DEBUG_LOG;
      break;
    }

    message_p = itti_alloc_new_message_sized(origin_task_id, messages_id, message_string_size);

    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      message_msg_p = (char *) &message_p->ittiMsg.error_log;
      break;

    case LOG_WARNING:
      message_msg_p = (char *) &message_p->ittiMsg.warning_log;
      break;

    case LOG_NOTICE:
      message_msg_p = (char *) &message_p->ittiMsg.notice_log;
      break;

    case LOG_INFO:
      message_msg_p = (char *) &message_p->ittiMsg.info_log;
      break;

    default:
      message_msg_p = (char *) &message_p->ittiMsg.debug_log;
      break;
    }

    memcpy(message_msg_p, log_start, message_string_size);

    itti_send_msg_to_task(TASK_UNKNOWN, INSTANCE_DEFAULT, message_p);
  }

#endif

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOG_RECORD,
                                          VCD_FUNCTION_OUT);
}

int set_log(int component, int level, int interval)
{
  /* Checking parameters */
  DevCheck((component >= MIN_LOG_COMPONENTS) && (component < MAX_LOG_COMPONENTS),
           component, MIN_LOG_COMPONENTS, MAX_LOG_COMPONENTS);
  DevCheck((level <= LOG_TRACE) && (level >= LOG_EMERG), level, LOG_TRACE,
           LOG_EMERG);
  DevCheck((interval >= 0) && (interval <= 0xFF), interval, 0, 0xFF);

  g_log->log_component[component].level = level;

  switch (level) {
  case LOG_TRACE:
    g_log->log_component[component].flag = LOG_MED ;
    break;

  case LOG_DEBUG:
    g_log->log_component[component].flag = LOG_MED ;
    break;

  case LOG_INFO:
    g_log->log_component[component].flag = LOG_LOW ;
    break;

  default:
    g_log->log_component[component].flag = LOG_NONE ;
    break;
  }

  g_log->log_component[component].interval = interval;

  return 0;
}

int set_comp_log(int component, int level, int verbosity, int interval)
{
  /* Checking parameters */
  DevCheck((component >= MIN_LOG_COMPONENTS) && (component < MAX_LOG_COMPONENTS),
           component, MIN_LOG_COMPONENTS, MAX_LOG_COMPONENTS);
  DevCheck((level <= LOG_TRACE) && (level >= LOG_EMERG), level, LOG_TRACE,
           LOG_EMERG);
  DevCheck((interval >= 0) && (interval <= 0xFF), interval, 0, 0xFF);

#if 0
  if ((verbosity == LOG_NONE) || (verbosity == LOG_LOW) ||
      (verbosity == LOG_MED) || (verbosity == LOG_FULL) ||
      (verbosity == LOG_HIGH)) {
    g_log->log_component[component].flag = verbosity;
  }
#else
  g_log->log_component[component].flag = verbosity;
#endif

  g_log->log_component[component].level = level;
  g_log->log_component[component].interval = interval;

  return 0;
}

void set_glog(int level, int verbosity)
{
  if( g_log->level >= 0) g_log->level = level;
  if( g_log->flag >= 0)  g_log->flag = verbosity;
}
void set_glog_syslog(int enable)
{
  g_log->syslog = enable;
}
void set_glog_onlinelog(int enable)
{
  g_log->onlinelog = enable;
}
void set_glog_filelog(int enable)
{
  g_log->filelog = enable;
}

void set_component_filelog(int comp)
{
  if (g_log->log_component[comp].filelog ==  0) {
    g_log->log_component[comp].filelog =  1;

    if (g_log->log_component[comp].fd == 0) {
      g_log->log_component[comp].fd = open(g_log->log_component[comp].filelog_name,
                                           O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
  }
}

/*
 * for the two functions below, the passed array must have a final entry
 * with string value NULL
 */
/* map a string to an int. Takes a mapping array and a string as arg */
int map_str_to_int(mapping *map, const char *str)
{
  while (1) {
    if (map->name == NULL) {
      return(-1);
    }

    if (!strcmp(map->name, str)) {
      return(map->value);
    }

    map++;
  }
}

/* map an int to a string. Takes a mapping array and a value */
char *map_int_to_str(mapping *map, int val)
{
  while (1) {
    if (map->name == NULL) {
      return NULL;
    }

    if (map->value == val) {
      return map->name;
    }

    map++;
  }
}

int is_newline( char *str, int size)
{
  int i;

  for (  i = 0; i < size; i++ ) {
    if ( str[i] == '\n' ) {
      return 1;
    }
  }

  /* if we get all the way to here, there must not have been a newline! */
  return 0;
}

void logClean (void)
{
  int i;

  if (g_log->syslog) {
    closelog();
  }

  if (g_log->filelog) {
    close(gfd);
  }

  for (i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
    if (g_log->log_component[i].filelog) {
      close(g_log->log_component[i].fd);
    }
  }
}

#if defined(ENABLE_ITTI)
void log_set_instance_type (log_instance_type_t instance)
{
  log_instance_type = instance;
}
#endif

#ifdef LOG_TEST

int main(int argc, char *argv[])
{

  logInit();

  //set_log_syslog(1);
  test_log();

  return 1;
}

int test_log(void)
{
  LOG_ENTER(MAC); // because the default level is DEBUG
  LOG_I(EMU, "1 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "1 debug  MAC \n");
  LOG_N(MAC, "1 notice MAC \n");
  LOG_W(MAC, "1 warning MAC \n");

  set_comp_log(EMU, LOG_INFO, FLAG_ONLINE);
  set_comp_log(MAC, LOG_WARNING, 0);

  LOG_I(EMU, "2 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_E(MAC, "2 emerge MAC\n");
  LOG_D(MAC, "2 debug  MAC \n");
  LOG_N(MAC, "2 notice MAC \n");
  LOG_W(MAC, "2 warning MAC \n");
  LOG_I(MAC, "2 info MAC \n");


  set_comp_log(MAC, LOG_NOTICE, 1);

  LOG_ENTER(MAC);
  LOG_I(EMU, "3 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "3 debug  MAC \n");
  LOG_N(MAC, "3 notice MAC \n");
  LOG_W(MAC, "3 warning MAC \n");
  LOG_I(MAC, "3 info MAC \n");

  set_comp_log(MAC, LOG_DEBUG,1);
  set_comp_log(EMU, LOG_DEBUG,1);

  LOG_ENTER(MAC);
  LOG_I(EMU, "4 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "4 debug  MAC \n");
  LOG_N(MAC, "4 notice MAC \n");
  LOG_W(MAC, "4 warning MAC \n");
  LOG_I(MAC, "4 info MAC \n");


  set_comp_log(MAC, LOG_DEBUG,0);
  set_comp_log(EMU, LOG_DEBUG,0);

  LOG_I(LOG, "5 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "5 debug  MAC \n");
  LOG_N(MAC, "5 notice MAC \n");
  LOG_W(MAC, "5 warning MAC \n");
  LOG_I(MAC, "5 info MAC \n");


  set_comp_log(MAC, LOG_TRACE,0X07F);
  set_comp_log(EMU, LOG_TRACE,0X07F);

  LOG_ENTER(MAC);
  LOG_I(LOG, "6 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "6 debug  MAC \n");
  LOG_N(MAC, "6 notice MAC \n");
  LOG_W(MAC, "6 warning MAC \n");
  LOG_I(MAC, "6 info MAC \n");
  LOG_EXIT(MAC);

  return 0;
}
#endif
