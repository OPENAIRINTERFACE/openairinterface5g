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


typedef struct {
	char* buf_p;
	int buf_index;
  int enable_flag;
} log_mem_cnt_t;

log_mem_cnt_t log_mem_d[2];
int log_mem_flag=0;
int log_mem_multi=1;
volatile int log_mem_side=0;
pthread_mutex_t log_mem_lock;
pthread_cond_t log_mem_notify;
pthread_t log_mem_thread;
int log_mem_file_cnt=0;
volatile int log_mem_write_flag=0;
volatile int log_mem_write_side=0;

char __log_mem_filename[1024]={0};
char * log_mem_filename = &__log_mem_filename[0];

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
  {"matlab", LOG_MATLAB},
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

int write_file_matlab(const char *fname,const char *vname,void *data,int length,int dec,char format)
{

  FILE *fp=NULL;
  int i;


  //printf("Writing %d elements of type %d to %s\n",length,format,fname);


  if (format == 10 || format ==11 || format == 12 || format == 13 || format == 14) {
    fp = fopen(fname,"a+");
  } else if (format != 10 && format !=11  && format != 12 && format != 13 && format != 14) {
    fp = fopen(fname,"w+");
  }



  if (fp== NULL) {
    printf("[OPENAIR][FILE OUTPUT] Cannot open file %s\n",fname);
    return(-1);
  }

  if (format != 10 && format !=11  && format != 12 && format != 13 && format != 14)
    fprintf(fp,"%s = [",vname);


  switch (format) {
  case 0:   // real 16-bit

    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((short *)data)[i]);
    }

    break;

  case 1:  // complex 16-bit
  case 13:
  case 14:
  case 15:

    for (i=0; i<length<<1; i+=(2*dec)) {
      fprintf(fp,"%d + j*(%d)\n",((short *)data)[i],((short *)data)[i+1]);

    }


    break;

  case 2:  // real 32-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((int *)data)[i]);
    }

    break;

  case 3: // complex 32-bit
    for (i=0; i<length<<1; i+=(2*dec)) {
      fprintf(fp,"%d + j*(%d)\n",((int *)data)[i],((int *)data)[i+1]);
    }

    break;

  case 4: // real 8-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((char *)data)[i]);
    }

    break;

  case 5: // complex 8-bit
    for (i=0; i<length<<1; i+=(2*dec)) {
      fprintf(fp,"%d + j*(%d)\n",((char *)data)[i],((char *)data)[i+1]);
    }

    break;

  case 6:  // real 64-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%lld\n",((long long*)data)[i]);
    }

    break;

  case 7: // real double
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%g\n",((double *)data)[i]);
    }

    break;

  case 8: // complex double
    for (i=0; i<length<<1; i+=2*dec) {
      fprintf(fp,"%g + j*(%g)\n",((double *)data)[i], ((double *)data)[i+1]);
    }

    break;

  case 9: // real unsigned 8-bit
    for (i=0; i<length; i+=dec) {
      fprintf(fp,"%d\n",((unsigned char *)data)[i]);
    }

    break;


  case 10 : // case eren 16 bit complex :

    for (i=0; i<length<<1; i+=(2*dec)) {

      if((i < 2*(length-1)) && (i > 0))
        fprintf(fp,"%d + j*(%d),",((short *)data)[i],((short *)data)[i+1]);
      else if (i == 2*(length-1))
        fprintf(fp,"%d + j*(%d);",((short *)data)[i],((short *)data)[i+1]);
      else if (i == 0)
        fprintf(fp,"\n%d + j*(%d),",((short *)data)[i],((short *)data)[i+1]);



    }

    break;

  case 11 : //case eren 16 bit real for channel magnitudes:
    for (i=0; i<length; i+=dec) {

      if((i <(length-1))&& (i > 0))
        fprintf(fp,"%d,",((short *)data)[i]);
      else if (i == (length-1))
        fprintf(fp,"%d;",((short *)data)[i]);
      else if (i == 0)
        fprintf(fp,"\n%d,",((short *)data)[i]);
    }

    printf("\n erennnnnnnnnnnnnnn: length :%d",length);
    break;

  case 12 : // case eren for log2_maxh real unsigned 8 bit
    fprintf(fp,"%d \n",((unsigned char *)&data)[0]);
    break;

  }

  if (format != 10 && format !=11 && format !=12 && format != 13 && format != 15) {
    fprintf(fp,"];\n");
    fclose(fp);
    return(0);
  } else if (format == 10 || format ==11 || format == 12 || format == 13 || format == 15) {
    fclose(fp);
    return(0);
  }

  return 0;
}

/* get log parameters from configuration file */
void  log_getconfig(log_t *g_log) {
  char *gloglevel = NULL;
  char *glogverbo = NULL;
  int level,verbosity;
  paramdef_t logparams_defaults[] = LOG_GLOBALPARAMS_DESC;
  paramdef_t logparams_level[MAX_LOG_PREDEF_COMPONENTS];
  paramdef_t logparams_verbosity[MAX_LOG_PREDEF_COMPONENTS];
  paramdef_t logparams_logfile[MAX_LOG_PREDEF_COMPONENTS];
  
  int ret = config_get( logparams_defaults,sizeof(logparams_defaults)/sizeof(paramdef_t),CONFIG_STRING_LOG_PREFIX);
  if (ret <0) {
       fprintf(stderr,"[LOG] init aborted, configuration couldn't be performed");
       return;
  } 

  memset(logparams_level,    0, sizeof(paramdef_t)*MAX_LOG_PREDEF_COMPONENTS);
  memset(logparams_verbosity,0, sizeof(paramdef_t)*MAX_LOG_PREDEF_COMPONENTS);
  memset(logparams_logfile,  0, sizeof(paramdef_t)*MAX_LOG_PREDEF_COMPONENTS);
  for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_PREDEF_COMPONENTS; i++) {
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
    logparams_logfile[i].defstrval   = malloc(strlen(g_log->log_component[i].name) + 16);
    sprintf(logparams_logfile[i].defstrval,"/tmp/oai%s.log",g_log->log_component[i].name);
    logparams_logfile[i].numelt      = 0;
    logparams_verbosity[i].numelt    = 0;
    logparams_level[i].numelt        = 0;
    logparams_level[i].type          = TYPE_STRING;
    logparams_verbosity[i].type      = TYPE_STRING;
    logparams_logfile[i].type        = TYPE_UINT;

    logparams_logfile[i].paramflags  = logparams_logfile[i].paramflags|PARAMFLAG_BOOL;
    }
  config_get( logparams_level,    MAX_LOG_PREDEF_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  config_get( logparams_verbosity,MAX_LOG_PREDEF_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  config_get( logparams_logfile,  MAX_LOG_PREDEF_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_PREDEF_COMPONENTS; i++) {
    verbosity = map_str_to_int(log_verbosity_names,*(logparams_verbosity[i].strptr));
    level     = map_str_to_int(log_level_names,    *(logparams_level[i].strptr));
    set_comp_log(i, level,verbosity,1);
    /* HOTFIX: the following statement crashes, it is thus commented
     * TODO: proper fix
     */
    /*set_component_filelog(*(logparams_logfile[i].uptr));*/
    if ( logparams_logfile[i].defstrval != NULL) {
       free (logparams_logfile[i].defstrval);
    }
    }
}

int register_log_component(char *name, char *fext, int compidx)
{
int computed_compidx=compidx;

  if (strlen(fext) > 3) {
      fext[3]=0;  /* limit log file extension to 3 chars */
  }
  if (compidx < 0) { /* this is not a pre-defined component */
      for (int i = MAX_LOG_PREDEF_COMPONENTS; i< MAX_LOG_COMPONENTS; i++) {
            if (g_log->log_component[i].name == NULL) {
                computed_compidx=i;
                break;
            }
      }
  }
  if (computed_compidx >= 0 && computed_compidx <MAX_LOG_COMPONENTS) {
      g_log->log_component[computed_compidx].name = strdup(name);
      g_log->log_component[computed_compidx].level = LOG_ALERT;
      g_log->log_component[computed_compidx].flag =  LOG_MED;
      g_log->log_component[computed_compidx].interval =  1;
      g_log->log_component[computed_compidx].fd = 0;
      g_log->log_component[computed_compidx].filelog = 0;
      g_log->log_component[computed_compidx].filelog_name = malloc(strlen(name)+16);/* /tmp/<name>.%s rounded to ^2 */
      sprintf(g_log->log_component[computed_compidx].filelog_name,"/tmp/%s.%s",name,fext);
  } else {
      fprintf(stderr,"{LOG} %s %d Couldn't register componemt %s\n",__FILE__,__LINE__,name);
  }
return computed_compidx;
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
  register_log_component("PHY","log",PHY);
  register_log_component("NR_PHY","log",NR_PHY);
  register_log_component("MAC","log",MAC);
  register_log_component("NR_MAC","log",NR_MAC);
  register_log_component("EMU","log",EMU);
  register_log_component("OPT","log",OPT);
  register_log_component("RLC","log",RLC);
  register_log_component("PDCP","log",PDCP);
  register_log_component("RRC","log",RRC);
  register_log_component("NR_RRC","log",NR_RRC);
  register_log_component("SIM","log",SIM);
  register_log_component("OMG","csv",OMG);
  register_log_component("OTG","log",OTG);
  register_log_component("OTG_LATENCY","dat",OTG_LATENCY);
  register_log_component("OTG_LATENCY_BG","dat",OTG_LATENCY_BG);
  register_log_component("OTG_GP","dat",OTG_GP);
  register_log_component("OTG_GP_BG","dat",OTG_GP_BG);
  register_log_component("OTG_JITTER","dat",OTG_JITTER);
  register_log_component("OCG","",OCG);
  register_log_component("PERF","",PERF);
  register_log_component("OIP","",OIP); 
  register_log_component("CLI","",CLI); 
  register_log_component("MSC","log",MSC); 
  register_log_component("OCM","log",OCM); 
  register_log_component("HW","",HW); 
  register_log_component("OSA","",OSA); 
  register_log_component("eRAL","",RAL_ENB); 
  register_log_component("mRAL","",RAL_UE); 
  register_log_component("ENB_APP","log",ENB_APP);
  register_log_component("GNB_APP","log",GNB_APP); 
  register_log_component("FLEXRAN_AGENT","log",FLEXRAN_AGENT); 
  register_log_component("TMR","",TMR); 
  register_log_component("USIM","txt",USIM);   


  /* following log component are used for the localization*/
  register_log_component("LOCALIZE","log",LOCALIZE);
#endif // ! defined(CN_BUILD)
  register_log_component("NAS","log",NAS);
  register_log_component("UDP","",UDP_);
  g_log->log_component[UDP_].flag = LOG_FULL;

  register_log_component("GTPV1U","",GTPU);
  g_log->log_component[GTPU].flag = LOG_FULL;

  register_log_component("S1AP","",S1AP);
  g_log->log_component[S1AP].flag = LOG_FULL;

  register_log_component("SCTP","",SCTP);
  register_log_component("RRH","",RRH);
 

  
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
  g_log->level2string[LOG_MATLAB]        = "M"; // MATLAB

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
    openlog(g_log->log_component[SIM].name, LOG_PID, g_log->config.facility);
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
  // set all unused component items to 0, they are for non predefined components
  for (i=MAX_LOG_PREDEF_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
        memset(&(g_log->log_component[i]),0,sizeof(log_component_t));
  }
  printf("log init done\n");

  return 0;
}


extern int oai_exit;
void log_mem_write(void)
{
  int fp;
  char f_name[1024];
  struct timespec slp_tm;
  slp_tm.tv_sec = 0;
  slp_tm.tv_nsec = 10000;
  
  pthread_setname_np( pthread_self(), "log_mem_write");

  while (!oai_exit) {
    pthread_mutex_lock(&log_mem_lock);
    log_mem_write_flag=0;
    pthread_cond_wait(&log_mem_notify, &log_mem_lock);
    log_mem_write_flag=1;
    pthread_mutex_unlock(&log_mem_lock);
    // write!
    if(log_mem_d[log_mem_write_side].enable_flag==0){
      if(log_mem_file_cnt>1){
        log_mem_file_cnt=1;
        printf("log over write!!!\n");
      }
      snprintf(f_name,1024, "%s_%d.log",log_mem_filename,log_mem_file_cnt);
      fp=open(f_name, O_WRONLY | O_CREAT, 0666);
      int ret = write(fp, log_mem_d[log_mem_write_side].buf_p, log_mem_d[log_mem_write_side].buf_index);
      if ( ret < 0) {
          fprintf(stderr,"{LOG} %s %d Couldn't write in %s \n",__FILE__,__LINE__,f_name);
          exit(EXIT_FAILURE);
      }
      close(fp);
      log_mem_file_cnt++;
      log_mem_d[log_mem_write_side].buf_index=0;
      log_mem_d[log_mem_write_side].enable_flag=1;
    }else{
      printf("If you'd like to write log, you should set enable flag to 0!!!\n");
      nanosleep(&slp_tm,NULL);
    }
  }
}

int logInit_log_mem (void)
{
  if(log_mem_flag==1){
    if(log_mem_multi==1){
      printf("log-mem multi!!!\n");
      log_mem_d[0].buf_p = malloc(LOG_MEM_SIZE);
      log_mem_d[0].buf_index=0;
      log_mem_d[0].enable_flag=1;
      log_mem_d[1].buf_p = malloc(LOG_MEM_SIZE);
      log_mem_d[1].buf_index=0;
      log_mem_d[1].enable_flag=1;
      log_mem_side=0;
      if ((pthread_mutex_init (&log_mem_lock, NULL) != 0)
          || (pthread_cond_init (&log_mem_notify, NULL) != 0)) {
        log_mem_d[1].enable_flag=0;
        return -1;
      }
      pthread_create(&log_mem_thread, NULL, (void *(*)(void *))log_mem_write, (void*)NULL);
    }else{
      printf("log-mem single!!!\n");
      log_mem_d[0].buf_p = malloc(LOG_MEM_SIZE);
      log_mem_d[0].buf_index=0;
      log_mem_d[0].enable_flag=1;
      log_mem_d[1].enable_flag=0;
      log_mem_side=0;
    }
  }else{
    log_mem_d[0].buf_p=NULL;
    log_mem_d[1].buf_p=NULL;
    log_mem_d[0].enable_flag=0;
    log_mem_d[1].enable_flag=0;
  }

  printf("log init done\n");
  
  return 0;
}

void nfapi_log(const char *file, const char *func, int line, int comp, int level, const char* format,va_list args)
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
  if ((g_log->onlinelog == 1) && (level != LOG_FILE)) {
    if(log_mem_flag==1){
      if(log_mem_d[log_mem_side].enable_flag==1){
        int temp_index;
        temp_index=log_mem_d[log_mem_side].buf_index;
        if(temp_index+len+1 < LOG_MEM_SIZE){
          log_mem_d[log_mem_side].buf_index+=len;
          memcpy(&log_mem_d[log_mem_side].buf_p[temp_index],log_buffer,len);
        }else{
          log_mem_d[log_mem_side].enable_flag=0;
          if(log_mem_d[1-log_mem_side].enable_flag==1){
            temp_index=log_mem_d[1-log_mem_side].buf_index;
            if(temp_index+len+1 < LOG_MEM_SIZE){
              log_mem_d[1-log_mem_side].buf_index+=len;
              log_mem_side=1-log_mem_side;
              memcpy(&log_mem_d[log_mem_side].buf_p[temp_index],log_buffer,len);
              /* write down !*/
              if (pthread_mutex_lock(&log_mem_lock) != 0) {
                return;
              }
              if(log_mem_write_flag==0){
                log_mem_write_side=1-log_mem_side;
                if(pthread_cond_signal(&log_mem_notify) != 0) {
                }
              }
              if(pthread_mutex_unlock(&log_mem_lock) != 0) {
                return;
              }
            }else{
              log_mem_d[1-log_mem_side].enable_flag=0;
            }
          }
        }
      }
    }else{
      fwrite(log_buffer, len, 1, stdout);
    }
  }

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
}

void logRecord_mt(const char *file, const char *func, int line, int comp, int level, const char* format, ... )
{
  va_list    args;
  va_start(args, format);
  nfapi_log(file,func,line,comp,level, format,args);
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



int set_log(int component, int level, int interval)
{
  /* Checking parameters */
  DevCheck((component >= MIN_LOG_COMPONENTS) && (component < MAX_LOG_COMPONENTS),
           component, MIN_LOG_COMPONENTS, MAX_LOG_COMPONENTS);
  DevCheck((level < NUM_LOG_LEVEL) && (level >= LOG_EMERG), level, NUM_LOG_LEVEL,
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
  DevCheck((level < NUM_LOG_LEVEL) && (level >= LOG_EMERG), level, NUM_LOG_LEVEL,
           LOG_EMERG);
  DevCheck((interval >= 0) && (interval <= 0xFF), interval, 0, 0xFF);

  g_log->log_component[component].flag = verbosity;

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
  
void output_log_mem(void){
  int fp;
  char f_name[1024];

  if(log_mem_flag==1){
    log_mem_d[0].enable_flag=0;
    log_mem_d[1].enable_flag=0;
    usleep(10); // wait for log writing
    while(log_mem_write_flag==1){
      usleep(100);
    }
    if(log_mem_multi==1){
      snprintf(f_name,1024, "%s_%d.log",log_mem_filename,log_mem_file_cnt);
      fp=open(f_name, O_WRONLY | O_CREAT, 0666);
      int ret = write(fp, log_mem_d[0].buf_p, log_mem_d[0].buf_index);
      if ( ret < 0) {
          fprintf(stderr,"{LOG} %s %d Couldn't write in %s \n",__FILE__,__LINE__,f_name);
          exit(EXIT_FAILURE);
      }
      close(fp);
      free(log_mem_d[0].buf_p);
      
      snprintf(f_name,1024, "%s_%d.log",log_mem_filename,log_mem_file_cnt);
      fp=open(f_name, O_WRONLY | O_CREAT, 0666);
      ret = write(fp, log_mem_d[1].buf_p, log_mem_d[1].buf_index);
      if ( ret < 0) {
          fprintf(stderr,"{LOG} %s %d Couldn't write in %s \n",__FILE__,__LINE__,f_name);
          exit(EXIT_FAILURE);
      }
      close(fp);
      free(log_mem_d[1].buf_p);
    }else{
      fp=open(log_mem_filename, O_WRONLY | O_CREAT, 0666);
      int ret = write(fp, log_mem_d[0].buf_p, log_mem_d[0].buf_index);
      if ( ret < 0) {
          fprintf(stderr,"{LOG} %s %d Couldn't write in %s \n",__FILE__,__LINE__,log_mem_filename);
          exit(EXIT_FAILURE);
       }
      close(fp);
      free(log_mem_d[0].buf_p);
    }
  }
}
  
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
  LOG_I(SIM, "1 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "1 debug  MAC \n");
  LOG_N(MAC, "1 notice MAC \n");
  LOG_W(MAC, "1 warning MAC \n");

  set_comp_log(SIM, LOG_INFO, FLAG_ONLINE);
  set_comp_log(MAC, LOG_WARNING, 0);

  LOG_I(SIM, "2 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_E(MAC, "2 emerge MAC\n");
  LOG_D(MAC, "2 debug  MAC \n");
  LOG_N(MAC, "2 notice MAC \n");
  LOG_W(MAC, "2 warning MAC \n");
  LOG_I(MAC, "2 info MAC \n");


  set_comp_log(MAC, LOG_NOTICE, 1);

  LOG_ENTER(MAC);
  LOG_I(SIM, "3 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "3 debug  MAC \n");
  LOG_N(MAC, "3 notice MAC \n");
  LOG_W(MAC, "3 warning MAC \n");
  LOG_I(MAC, "3 info MAC \n");

  set_comp_log(MAC, LOG_DEBUG,1);
  set_comp_log(SIM, LOG_DEBUG,1);

  LOG_ENTER(MAC);
  LOG_I(SIM, "4 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "4 debug  MAC \n");
  LOG_N(MAC, "4 notice MAC \n");
  LOG_W(MAC, "4 warning MAC \n");
  LOG_I(MAC, "4 info MAC \n");


  set_comp_log(MAC, LOG_DEBUG,0);
  set_comp_log(SIM, LOG_DEBUG,0);

  LOG_I(LOG, "5 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "5 debug  MAC \n");
  LOG_N(MAC, "5 notice MAC \n");
  LOG_W(MAC, "5 warning MAC \n");
  LOG_I(MAC, "5 info MAC \n");


  set_comp_log(MAC, LOG_TRACE,0X07F);
  set_comp_log(SIM, LOG_TRACE,0X07F);

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
