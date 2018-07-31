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
#include  <linux/prctl.h>
#include "common/config/config_userapi.h"
// main log variables




mapping log_level_names[] = {
  {"error",  OAILOG_ERR},
  {"file",   OAILOG_FILE},
  {"warn",   OAILOG_WARNING},
  {"info",   OAILOG_INFO},
  {"debug",  OAILOG_DEBUG},
  {"trace",  OAILOG_TRACE},
  {NULL, -1}
};

mapping log_options[] = {
  {"nocolor", FLAG_NOCOLOR  },
  {"level",   FLAG_LEVEL  },
  {"thread",  FLAG_THREAD },
  {NULL,-1}
};


mapping log_maskmap[] = {
  {"prach",       DEBUG_PRACH},
  {"RU",          DEBUG_RU},
  {"LTEESTIM",    DEBUG_LTEESTIM},
  {"ctrlsocket",  DEBUG_CTRLSOCKET},
  {"UE_PHYPROC",  DEBUG_UE_PHYPROC},
  {"UE_TIMING",   UE_TIMING},
  {NULL,-1}
};

char *log_level_highlight_start[] = {LOG_RED, LOG_GREEN, LOG_ORANGE, "", LOG_BLUE, LOG_CYBL};  /*!< \brief Optional start-format strings for highlighting */
char *log_level_highlight_end[]   = {LOG_RESET,LOG_RESET,LOG_RESET,LOG_RESET, LOG_RESET,LOG_RESET};   /*!< \brief Optional end-format strings for highlighting */


int write_file_matlab(const char *fname,const char *vname,void *data,int length,int dec,char format)
{

  FILE *fp=NULL;
  int i;

  if (data == NULL)
     return -1;
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

    printf("\n eren: length :%d",length);
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
  int level;
  
  
  paramdef_t logparams_defaults[] = LOG_GLOBALPARAMS_DESC;
  paramdef_t logparams_level[MAX_LOG_PREDEF_COMPONENTS];
  paramdef_t logparams_logfile[MAX_LOG_PREDEF_COMPONENTS];
  paramdef_t logparams_debug[sizeof(log_maskmap)/sizeof(mapping)];
  paramdef_t logparams_matlab[sizeof(log_maskmap)/sizeof(mapping)];

  int ret = config_get( logparams_defaults,sizeof(logparams_defaults)/sizeof(paramdef_t),CONFIG_STRING_LOG_PREFIX);
  if (ret <0) {
       fprintf(stderr,"[LOG] init aborted, configuration couldn't be performed");
       return;
  } 

  for(int i=0; i<logparams_defaults[LOG_OPTIONS_IDX].numelt ; i++) {
     for(int j=0; log_options[j].name != NULL ; j++) {
        if (strcmp(logparams_defaults[LOG_OPTIONS_IDX].strlistptr[i],log_options[j].name) == 0) { 
            g_log->flag = g_log->flag |  log_options[j].value;
            break;
        } else if (log_options[j+1].name == NULL){
            fprintf(stderr,"Unknown log option: %s\n",logparams_defaults[LOG_OPTIONS_IDX].strlistptr[i]);
            exit(-1);
        }
     } 
  } 
  
/* build the parameter array for setting per component log level and infile options */  
  memset(logparams_level,    0, sizeof(paramdef_t)*MAX_LOG_PREDEF_COMPONENTS);
  memset(logparams_logfile,  0, sizeof(paramdef_t)*MAX_LOG_PREDEF_COMPONENTS);
  for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_PREDEF_COMPONENTS; i++) {
    if(g_log->log_component[i].name == NULL) {
       g_log->log_component[i].name = malloc(16);
       sprintf((char *)g_log->log_component[i].name,"comp%i?",i);
       logparams_logfile[i].paramflags = PARAMFLAG_DONOTREAD;
       logparams_level[i].paramflags = PARAMFLAG_DONOTREAD;
    }
    sprintf(logparams_level[i].optname,    LOG_CONFIG_LEVEL_FORMAT,       g_log->log_component[i].name);
    sprintf(logparams_logfile[i].optname,  LOG_CONFIG_LOGFILE_FORMAT,     g_log->log_component[i].name);
/* workaround: all log options in existing configuration files use lower case component names
   where component names include uppercase char in log.h....                                */ 
    for (int j=0 ; j<strlen(logparams_level[i].optname); j++) 
          logparams_level[i].optname[j] = tolower(logparams_level[i].optname[j]);
    for (int j=0 ; j<strlen(logparams_level[i].optname); j++) 
          logparams_logfile[i].optname[j] = tolower(logparams_logfile[i].optname[j]);
/* */
    logparams_level[i].defstrval     = gloglevel;
    logparams_logfile[i].defuintval  = 0;
    logparams_logfile[i].numelt      = 0;
    logparams_level[i].numelt        = 0;
    logparams_level[i].type          = TYPE_STRING;
    logparams_logfile[i].type        = TYPE_UINT;

    logparams_logfile[i].paramflags  = logparams_logfile[i].paramflags|PARAMFLAG_BOOL;
    }
/* read the per component parameters */
  config_get( logparams_level,    MAX_LOG_PREDEF_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
  config_get( logparams_logfile,  MAX_LOG_PREDEF_COMPONENTS,CONFIG_STRING_LOG_PREFIX); 
/* now set the log levels and infile option, according to what we read */
  for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_PREDEF_COMPONENTS; i++) {
    level     = map_str_to_int(log_level_names,    *(logparams_level[i].strptr));
    set_log(i, level,1);
    if (*(logparams_logfile[i].uptr) == 1)
        set_component_filelog(i);
  }

/* build then read the debug and matlab parameter array */
  for (int i=0;log_maskmap[i].name != NULL ; i++) {
      sprintf(logparams_debug[i].optname,    LOG_CONFIG_DEBUG_FORMAT, log_maskmap[i].name);
      sprintf(logparams_matlab[i].optname,   LOG_CONFIG_MATLAB_FORMAT, log_maskmap[i].name);
      logparams_debug[i].defuintval  = 0;
      logparams_debug[i].type        = TYPE_UINT;
      logparams_debug[i].paramflags  = PARAMFLAG_BOOL;
      logparams_debug[i].uptr        = NULL;
      logparams_debug[i].chkPptr     = NULL;
      logparams_debug[i].numelt      = 0;
      logparams_matlab[i].defuintval  = 0;
      logparams_matlab[i].type        = TYPE_UINT;
      logparams_matlab[i].paramflags  = PARAMFLAG_BOOL;
      logparams_matlab[i].uptr        = NULL;
      logparams_matlab[i].chkPptr     = NULL;
      logparams_matlab[i].numelt      = 0;
  }
  config_get( logparams_debug,(sizeof(log_maskmap)/sizeof(mapping)) - 1 ,CONFIG_STRING_LOG_PREFIX);
  config_get( logparams_matlab,(sizeof(log_maskmap)/sizeof(mapping)) - 1 ,CONFIG_STRING_LOG_PREFIX);
/* set the debug mask according to the debug parameters values */
  for (int i=0; log_maskmap[i].name != NULL ; i++) {
    if (*(logparams_debug[i].uptr) )
        g_log->debug_mask = g_log->debug_mask | log_maskmap[i].value;
    if (*(logparams_matlab[i].uptr) )
        g_log->matlab_mask = g_log->matlab_mask | log_maskmap[i].value;
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
      g_log->log_component[computed_compidx].level = LOG_ERR;
      g_log->log_component[computed_compidx].interval =  1;
      g_log->log_component[computed_compidx].stream = NULL;
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
  memset(g_log,0,sizeof(log_t));



  register_log_component("PHY","log",PHY);
  register_log_component("MAC","log",MAC);
  register_log_component("OPT","log",OPT);
  register_log_component("RLC","log",RLC);
  register_log_component("PDCP","log",PDCP);
  register_log_component("RRC","log",RRC);
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
  register_log_component("FLEXRAN_AGENT","log",FLEXRAN_AGENT); 
  register_log_component("TMR","",TMR); 
  register_log_component("USIM","txt",USIM);   
  register_log_component("SIM","txt",SIM);  

  /* following log component are used for the localization*/
  register_log_component("LOCALIZE","log",LOCALIZE);
  register_log_component("NAS","log",NAS);
  register_log_component("UDP","",UDP_);


  register_log_component("GTPV1U","",GTPU);


  register_log_component("S1AP","",S1AP);


  register_log_component("SCTP","",SCTP);
  register_log_component("RRH","",RRH);
 

  



  g_log->level2string[OAILOG_ERR]           = "E"; // ERROR
  g_log->level2string[OAILOG_WARNING]       = "W"; // WARNING
  g_log->level2string[OAILOG_INFO]          = "I"; //INFO
  g_log->level2string[OAILOG_DEBUG]         = "D"; // DEBUG
  g_log->level2string[OAILOG_FILE]          = "F"; // file
  g_log->level2string[OAILOG_TRACE]         = "T"; // TRACE
 

  g_log->onlinelog = 1; //online log file
  g_log->filelog   = 0;
 


  g_log->filelog_name = "/tmp/openair.log";

  log_getconfig(g_log);


  // could put a loop here to check for all comps
  for (i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
    if (g_log->log_component[i].filelog == 1 ) {
      g_log->log_component[i].stream = fopen(g_log->log_component[i].filelog_name,"w");
      g_log->log_component[i].fwrite = vfprintf;
    } else if (g_log->log_component[i].filelog == 1 ) {
        g_log->log_component[i].stream = fopen(g_log->filelog_name,"w");
        g_log->log_component[i].fwrite = vfprintf;
    } else if (g_log->onlinelog == 1 ) {
        g_log->log_component[i].stream = stdout;
        g_log->log_component[i].fwrite = vfprintf;
    }
  }

  // set all unused component items to 0, they are for non predefined components
  for (i=MAX_LOG_PREDEF_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
        memset(&(g_log->log_component[i]),0,sizeof(log_component_t));
  }
  printf("log init done\n");

  return 0;
}




char *log_getthreadname(char *threadname, int bufsize) {

int rt =   pthread_getname_np(pthread_self(), threadname,bufsize) ;  
   if (rt == 0)
   {
     return threadname;
   } else {
     return "thread?";
   }
}


void logRecord_mt(const char *file, const char *func, int line, int comp, int level, const char* format, ... )
  {

  char threadname[PR_SET_NAME];
  char log_buffer[MAX_LOG_TOTAL];
  va_list args;

  va_start(args, format);






  snprintf(log_buffer, MAX_LOG_TOTAL , "%s%s[%s]%s %s %s",
  	   log_level_highlight_end[level],
  	   ( (g_log->flag & FLAG_NOCOLOR)?"":log_level_highlight_start[level]),
  	   g_log->log_component[comp].name,
  	   ( (g_log->flag & FLAG_LEVEL)?g_log->level2string[level]:""),
  	   ( (g_log->flag & FLAG_THREAD)?log_getthreadname(threadname,PR_SET_NAME+1):""),
  	   format);

  g_log->log_component[comp].fwrite(g_log->log_component[comp].stream,log_buffer, args);
  va_end(args);


}



int set_log(int component, int level, int interval)
{
  /* Checking parameters */
  DevCheck((component >= MIN_LOG_COMPONENTS) && (component < MAX_LOG_COMPONENTS),
           component, MIN_LOG_COMPONENTS, MAX_LOG_COMPONENTS);
  DevCheck((level < NUM_LOG_LEVEL) && (level >= OAILOG_ERR), level, NUM_LOG_LEVEL,
           OAILOG_ERR);
  DevCheck((interval >= 0) && (interval <= 0xFF), interval, 0, 0xFF);

  g_log->log_component[component].level = level;


  g_log->log_component[component].interval = interval;

  return 0;
}



void set_glog(int level)
{
  for (int c=0; c< MAX_LOG_COMPONENTS; c++ ) {
     g_log->log_component[c].level = level;
  }
  
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

    if (g_log->log_component[comp].stream == NULL) {
      g_log->log_component[comp].stream = fopen(g_log->log_component[comp].filelog_name,"w");
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
  LOG_I(PHY,"\n");




  for (i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
    if (g_log->log_component[i].stream != NULL) {
      fclose(g_log->log_component[i].stream);
    }
  }
}

  
#ifdef LOG_TEST

int main(int argc, char *argv[])
{

  logInit();


  test_log();

  return 1;
}

int test_log(void)
{
  LOG_ENTER(MAC); // because the default level is DEBUG
  LOG_I(EMU, "1 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "1 debug  MAC \n");
  LOG_W(MAC, "1 warning MAC \n");

  set_log(EMU, OAILOG_INFO, FLAG_ONLINE);
  set_log(MAC, OAILOG_WARNING, 0);

  LOG_I(EMU, "2 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_E(MAC, "2 error MAC\n");
  LOG_D(MAC, "2 debug  MAC \n");
  LOG_W(MAC, "2 warning MAC \n");
  LOG_I(MAC, "2 info MAC \n");


  set_log(MAC, OAILOG_NOTICE, 1);

  LOG_ENTER(MAC);
  LOG_I(EMU, "3 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "3 debug  MAC \n");
  LOG_W(MAC, "3 warning MAC \n");
  LOG_I(MAC, "3 info MAC \n");

  set_log(MAC, LOG_DEBUG,1);
  set_log(EMU, LOG_DEBUG,1);

  LOG_ENTER(MAC);
  LOG_I(EMU, "4 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "4 debug  MAC \n");
  LOG_W(MAC, "4 warning MAC \n");
  LOG_I(MAC, "4 info MAC \n");


  set_log(MAC, LOG_DEBUG,0);
  set_log(EMU, LOG_DEBUG,0);

  LOG_I(LOG, "5 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "5 debug  MAC \n");
  LOG_W(MAC, "5 warning MAC \n");
  LOG_I(MAC, "5 info MAC \n");


  set_log(MAC, LOG_TRACE,0X07F);
  set_log(EMU, LOG_TRACE,0X07F);

  LOG_ENTER(MAC);
  LOG_I(LOG, "6 Starting OAI logs version %s Build date: %s on %s\n",
        BUILD_VERSION, BUILD_DATE, BUILD_HOST);
  LOG_D(MAC, "6 debug  MAC \n");
  LOG_W(MAC, "6 warning MAC \n");
  LOG_I(MAC, "6 info MAC \n");
  LOG_EXIT(MAC);

  return 0;
}
#endif
