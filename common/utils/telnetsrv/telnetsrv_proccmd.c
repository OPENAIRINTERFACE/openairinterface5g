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

/*! \file common/utils/telnetsrv/telnetsrv_proccmd.c
 * \brief: implementation of telnet commands related to this linux process
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>

#define READCFG_DYNLOAD

#define TELNETSERVERCODE
#include "telnetsrv.h"
#define TELNETSRV_PROCCMD_MAIN
#include "common/utils/LOG/log.h"
#include "common/config/config_userapi.h"
#include "openair1/PHY/phy_extern.h"
#include "telnetsrv_proccmd.h"

void decode_procstat(char *record, int debug, telnet_printfunc_t prnt, webdatadef_t *tdata)
{
char prntline[160];
char *procfile_fields;
char *strtokptr;
char *lptr;
int fieldcnt;
char toksep[2];

fieldcnt = 0;
procfile_fields = strtok_r(record, " ", &strtokptr);
lptr = prntline;
/*http://man7.org/linux/man-pages/man5/proc.5.html gives the structure of the stat file */
int priority = 0;
int nice = 0;
while (procfile_fields != NULL && fieldcnt < 42) {
  long int policy;
  if (strlen(procfile_fields) == 0)
    continue;
  fieldcnt++;
  sprintf(toksep, " ");
  switch (fieldcnt) {
    case 1: /* id */
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[0] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, "%9.9s ", procfile_fields);
      sprintf(toksep, ")");
      break;
    case 2: /* name */
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[1] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, "%20.20s ", procfile_fields + 1);
      break;
    case 3: // thread state
      lptr += sprintf(lptr, "  %c   ", procfile_fields[0]);
      break;
    case 14: // time in user mode
    case 15: // time in kernel mode
      lptr += sprintf(lptr, "%9.9s ", procfile_fields);
      break;
    case 18: // priority column index 2 in tdata, -2 to -100 (1, min to 99, highest prio)
      priority = strtol(procfile_fields, NULL, 0);
    case 19: // nice	  column index 3 in tdata  0 to 39 (-20, highest prio, to 19)
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[fieldcnt - 16] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, "%3.3s ", procfile_fields);
      nice = strtol(procfile_fields, NULL, 0);
      break;
    case 23: // vsize
      lptr += sprintf(lptr, "%9.9s ", procfile_fields);
      break;
    case 39: // processor
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[4] = strdup(procfile_fields);
      }
      lptr += sprintf(lptr, " %2.2s  ", procfile_fields);
      break;
    case 41: // policy
      lptr += sprintf(lptr, "%3.3s ", procfile_fields);
      policy = strtol(procfile_fields, NULL, 0);
      char strschedp[64];
      switch (policy) {
        case SCHED_FIFO:
          snprintf(strschedp, sizeof(strschedp), "%s ", "rt:fifo");
          priority = priority + 1; // in /proc file system priority 1 to 99 mapped to -2 to -100
          break;
        case SCHED_OTHER:
          snprintf(strschedp, sizeof(strschedp), "%s ", "other");
          priority = nice - NICE_MIN; // linux nice is -20 to 19
          break;
        case SCHED_IDLE:
          snprintf(strschedp, sizeof(strschedp), "%s ", "idle");
          priority = 2 * (NICE_MAX - NICE_MIN + 1);
          break;
        case SCHED_BATCH:
          snprintf(strschedp, sizeof(strschedp), "%s ", "batch");
          priority = (NICE_MAX - NICE_MIN + 1) + nice - NICE_MIN;
          break;
        case SCHED_RR:
          snprintf(strschedp, sizeof(strschedp), "%s ", "rt:rr");
          priority = priority - 99;
          break;
#ifdef SCHED_DEADLINE
        case SCHED_DEADLINE:
          snprintf(strschedp, sizeof(strschedp), "%s ", "rt:deadline");
          break;
#endif
        default:
          snprintf(strschedp, sizeof(strschedp), "%s ", "????");
          break;
      }
      lptr += sprintf(lptr, "%s ", strschedp);
      if (tdata != NULL) {
        tdata->lines[tdata->numlines].val[5] = strdup(strschedp);
        tdata->lines[tdata->numlines].val[6] = malloc(10);
        snprintf(tdata->lines[tdata->numlines].val[6], 9, "%i", priority);
      }
      break;
    default:
      break;
  } /* switch on fieldcnr */
  procfile_fields = strtok_r(NULL, toksep, &strtokptr);
} /* while on proc_fields != NULL */
prnt("%s\n", prntline);
if (tdata != NULL) {
  tdata->numlines++;
}
} /*decode_procstat */

void read_statfile(char *fname, int debug, telnet_printfunc_t prnt, webdatadef_t *tdata)
{
FILE *procfile;
char arecord[1024];

    procfile=fopen(fname,"r");
    if (procfile == NULL)
       {
       prnt("Error: Couldn't open %s %i %s\n",fname,errno,strerror(errno));
       return;
       }    
    if ( fgets(arecord,sizeof(arecord),procfile) == NULL)
       {
       prnt("Error: Nothing read from %s %i %s\n",fname,errno,strerror(errno));
       fclose(procfile);
       return;
       }    
    fclose(procfile);
    decode_procstat(arecord, debug, prnt, tdata);
}

int nullprnt(char *fmt, ...)
{
  return 0;
}

void proccmd_get_threaddata(char *buf, int debug, telnet_printfunc_t fprnt, webdatadef_t *tdata)
{
char aname[256];

  DIR *proc_dir;
  struct dirent *entry;
  telnet_printfunc_t prnt = (fprnt != NULL) ? fprnt : (telnet_printfunc_t)nullprnt;
  if (tdata != NULL) {
    tdata->numcols = 7;
    snprintf(tdata->columns[0].coltitle, sizeof(tdata->columns[0].coltitle), "thread id");
    tdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[1].coltitle, sizeof(tdata->columns[1].coltitle), "thread name");
    tdata->columns[1].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[2].coltitle, sizeof(tdata->columns[2].coltitle), "priority");
    tdata->columns[2].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[3].coltitle, sizeof(tdata->columns[3].coltitle), "nice");
    tdata->columns[3].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[4].coltitle, sizeof(tdata->columns[4].coltitle), "core");
    tdata->columns[4].coltype = TELNET_VARTYPE_STRING | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[5].coltitle, sizeof(tdata->columns[5].coltitle), "sched policy");
    tdata->columns[5].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[5].coltitle, sizeof(tdata->columns[5].coltitle), "sched policy");
    tdata->columns[6].coltype = TELNET_VARTYPE_STRING | TELNET_VAR_NEEDFREE;
    snprintf(tdata->columns[6].coltitle, sizeof(tdata->columns[6].coltitle), "oai priority");
    tdata->numlines = 0;
  }

  unsigned int eax = 11, ebx = 0, ecx = 1, edx = 0;

  asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "0"(eax), "2"(ecx) :);

  prnt("System has %d cores %d threads %d Actual threads", eax, ebx, edx);

  prnt("\n  id          name            state   USRmod    KRNmod  prio nice   vsize   proc pol \n\n");
  snprintf(aname, sizeof(aname), "/proc/%d/stat", getpid());
  read_statfile(aname, debug, prnt, NULL);
  prnt("\n");
  snprintf(aname, sizeof(aname), "/proc/%d/task", getpid());
  proc_dir = opendir(aname);
  if (proc_dir == NULL) {
    prnt("Error: Couldn't open %s %i %s\n", aname, errno, strerror(errno));
    return;
  }

  while ((entry = readdir(proc_dir)) != NULL) {
    if (entry->d_name[0] != '.') {
      snprintf(aname, sizeof(aname), "/proc/%d/task/%.*s/stat", getpid(), (int)(sizeof(aname) - 24), entry->d_name);
      read_statfile(aname, debug, prnt, tdata);
    }
  } /* while entry != NULL */
  closedir(proc_dir);
} /* proccmd_get_threaddata */

void print_threads(char *buf, int debug, telnet_printfunc_t prnt)
{
  proccmd_get_threaddata(buf, debug, prnt, NULL);
}

int proccmd_websrv_getdata(char *cmdbuff, int debug, void *data, telnet_printfunc_t prnt)
{
  webdatadef_t *logsdata = (webdatadef_t *)data;
  if (strncmp(cmdbuff, "set", 3) == 0) {
    telnet_printfunc_t printfunc = (prnt != NULL) ? prnt : (telnet_printfunc_t)printf;
    if (strcasestr(cmdbuff, "loglvl") != NULL) {
      int level = map_str_to_int(log_level_names, logsdata->lines[0].val[1]);
      int enabled = (strcmp(logsdata->lines[0].val[2], "true") == 0) ? 1 : 0;
      int loginfile = (strcmp(logsdata->lines[0].val[3], "true") == 0) ? 1 : 0;
      set_log(logsdata->numlines, level);
      if (enabled == 0)
        set_log(logsdata->numlines, OAILOG_DISABLE);
      if (loginfile == 1) {
        set_component_filelog(logsdata->numlines);
      } else {
        close_component_filelog(logsdata->numlines);
      }
      printfunc("%s log level %s is %s, output to %s\n",
                logsdata->lines[0].val[0],
                logsdata->lines[0].val[1],
                enabled ? "enabled" : "disabled",
                loginfile ? g_log->log_component[logsdata->numlines].filelog_name : "stdout");
    }
    if (strcasestr(cmdbuff, "logopt") != NULL) {
      int optbit = map_str_to_int(log_options, logsdata->lines[0].val[0]);
      if (optbit < 0) {
        printfunc("option %s unknown\n", logsdata->lines[0].val[0]);
      } else {
        if (strcmp(logsdata->lines[0].val[1], "true") == 0) {
          SET_LOG_OPTION(optbit);
        } else {
          CLEAR_LOG_OPTION(optbit);
        }
        printfunc("%s log option %s\n", logsdata->lines[0].val[0], (strcmp(logsdata->lines[0].val[1], "true") == 0) ? "enabled" : "disabled");
      }
    }
    if (strcasestr(cmdbuff, "dbgopt") != NULL) {
      int optbit = map_str_to_int(log_maskmap, logsdata->lines[0].val[0]);
      if (optbit < 0) {
        printfunc("debug option %s unknown\n", logsdata->lines[0].val[0]);
      } else {
        if (strcmp(logsdata->lines[0].val[1], "true") == 0)
          SET_LOG_DEBUG(optbit);
        else
          CLEAR_LOG_DEBUG(optbit);
        if (strcmp(logsdata->lines[0].val[2], "true") == 0)
          SET_LOG_DUMP(optbit);
        else
          CLEAR_LOG_DUMP(optbit);
        printfunc("%s debug %s dump %s\n",
                  logsdata->lines[0].val[0],
                  (strcmp(logsdata->lines[0].val[1], "true") == 0) ? "enabled" : "disabled",
                  (strcmp(logsdata->lines[0].val[2], "true") == 0) ? "enabled" : "disabled");
      }
    }
    if (strcasestr(cmdbuff, "threadsched") != NULL) {
      unsigned int tid = strtoll(logsdata->lines[0].val[0], NULL, 0);
      unsigned int core = strtoll(logsdata->lines[0].val[4], NULL, 0);
      int priority = strtoll(logsdata->lines[0].val[6], NULL, 0);
      printfunc("Thread %s id %u set affinity to core %u\n", logsdata->lines[0].val[0], tid, core);
      set_affinity(0, (pid_t)tid, core);
      set_sched(0, (pid_t)tid, priority);
    }
  } else { /* end of set, => show */
    if (strcasestr(cmdbuff, "loglvl") != NULL) {
      logsdata->numcols = 4;
      logsdata->numlines = 0;
      snprintf(logsdata->columns[0].coltitle, TELNET_CMD_MAXSIZE, "component");
      logsdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY;
      snprintf(logsdata->columns[1].coltitle, TELNET_CMD_MAXSIZE, "level");
      logsdata->columns[1].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_LOGLVL;
      snprintf(logsdata->columns[2].coltitle, TELNET_CMD_MAXSIZE, "enabled");
      logsdata->columns[2].coltype = TELNET_CHECKVAL_BOOL;
      snprintf(logsdata->columns[3].coltitle, TELNET_CMD_MAXSIZE, "in file");
      logsdata->columns[3].coltype = TELNET_CHECKVAL_BOOL;

      for (int i = MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
        if (g_log->log_component[i].name != NULL) {
          logsdata->numlines++;
          logsdata->lines[i].val[0] = (char *)(g_log->log_component[i].name);

          logsdata->lines[i].val[1] = map_int_to_str(log_level_names, (g_log->log_component[i].level >= 0) ? g_log->log_component[i].level : g_log->log_component[i].savedlevel);
          logsdata->lines[i].val[2] = (g_log->log_component[i].level >= 0) ? "true" : "false";
          logsdata->lines[i].val[3] = (g_log->log_component[i].filelog > 0) ? "true" : "false";
        }
      }
    }
    if (strcasestr(cmdbuff, "dbgopt") != NULL) {
      webdatadef_t *logsdata = (webdatadef_t *)data;
      logsdata->numcols = 3;
      logsdata->numlines = 0;
      snprintf(logsdata->columns[0].coltitle, TELNET_CMD_MAXSIZE, "module");
      logsdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY;
      snprintf(logsdata->columns[1].coltitle, TELNET_CMD_MAXSIZE, "debug");
      logsdata->columns[1].coltype = TELNET_CHECKVAL_BOOL;
      snprintf(logsdata->columns[2].coltitle, TELNET_CMD_MAXSIZE, "dump");
      logsdata->columns[2].coltype = TELNET_CHECKVAL_BOOL;

      for (int i = 0; log_maskmap[i].name != NULL; i++) {
        logsdata->numlines++;
        logsdata->lines[i].val[0] = log_maskmap[i].name;
        logsdata->lines[i].val[1] = (g_log->debug_mask & log_maskmap[i].value) ? "true" : "false";
        logsdata->lines[i].val[2] = (g_log->dump_mask & log_maskmap[i].value) ? "true" : "false";
      }
    }

    if (strcasestr(cmdbuff, "logopt") != NULL) {
      webdatadef_t *logsdata = (webdatadef_t *)data;
      logsdata->numcols = 2;
      logsdata->numlines = 0;
      snprintf(logsdata->columns[0].coltitle, TELNET_CMD_MAXSIZE, "option");
      logsdata->columns[0].coltype = TELNET_VARTYPE_STRING | TELNET_CHECKVAL_RDONLY;
      snprintf(logsdata->columns[1].coltitle, TELNET_CMD_MAXSIZE, "enabled");
      logsdata->columns[1].coltype = TELNET_CHECKVAL_BOOL;

      for (int i = 0; log_options[i].name != NULL; i++) {
        logsdata->numlines++;
        logsdata->lines[i].val[0] = log_options[i].name;
        logsdata->lines[i].val[1] = (g_log->flag & log_options[i].value) ? "true" : "false";
      }
    }
    if (strcasestr(cmdbuff, "threadsched") != NULL) {
      proccmd_get_threaddata(cmdbuff, debug, prnt, (webdatadef_t *)data);
    }
  } // show

  return 0;
}

int proccmd_show(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (buf == NULL) {
    prnt("ERROR wrong softmodem SHOW command...\n");
    return 0;
  }
   if (debug > 0)
       prnt(" proccmd_show received %s\n",buf);
   if (strcasestr(buf,"thread") != NULL) {
       print_threads(buf,debug,prnt);
   }
   if (strcasestr(buf,"loglvl") != NULL) {
       prnt("\n               component level  enabled   output\n");
       for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
            if (g_log->log_component[i].name != NULL) {
               prnt("%02i %17.17s:%10.10s    %s      %s\n",i ,g_log->log_component[i].name, 
	             map_int_to_str(log_level_names,(g_log->log_component[i].level>=0)?g_log->log_component[i].level:g_log->log_component[i].savedlevel),
                     ((g_log->log_component[i].level>=0)?"Y":"N"),
                     ((g_log->log_component[i].filelog>0)?g_log->log_component[i].filelog_name:"stdout"));
           }
       }
   }
   if (strcasestr(buf,"logopt") != NULL) {
       prnt("\n               option      enabled\n");
       for (int i=0; log_options[i].name != NULL; i++) {
               prnt("%02i %17.17s %10.10s \n",i ,log_options[i].name, 
                     ((g_log->flag & log_options[i].value)?"Y":"N") );
       }
   }
   if (strcasestr(buf,"dbgopt") != NULL) {
       prnt("\n               module  debug dumpfile\n");
       for (int i=0; log_maskmap[i].name != NULL ; i++) {
               prnt("%02i %17.17s %5.5s   %5.5s\n",i ,log_maskmap[i].name, 
	             ((g_log->debug_mask &  log_maskmap[i].value)?"Y":"N"),
                     ((g_log->dump_mask & log_maskmap[i].value)?"Y":"N") );
       }
   }
   if (strcasestr(buf,"config") != NULL) {
       prnt("Command line arguments:\n");
       for (int i=0; i < config_get_if()->argc; i++) {
            prnt("    %02i %s\n",i ,config_get_if()->argv[i]);
       }
       prnt("Config module flags ( -O <cfg source>:<xxx>:dbgl<flags>): 0x%08x\n", config_get_if()->rtflags); 

       prnt("    Print config debug msg, params values (flag %u): %s\n",CONFIG_PRINTPARAMS,
            ((config_get_if()->rtflags & CONFIG_PRINTPARAMS) ? "Y" : "N") ); 
       prnt("    Print config debug msg, memory management(flag %u): %s\n",CONFIG_DEBUGPTR,
            ((config_get_if()->rtflags & CONFIG_DEBUGPTR) ? "Y" : "N") ); 
       prnt("    Print config debug msg, command line processing (flag %u): %s\n",CONFIG_DEBUGCMDLINE,
            ((config_get_if()->rtflags & CONFIG_DEBUGCMDLINE) ? "Y" : "N") );        
       prnt("    Don't exit if param check fails (flag %u): %s\n",CONFIG_NOABORTONCHKF,
            ((config_get_if()->rtflags & CONFIG_NOABORTONCHKF) ? "Y" : "N") );      
       prnt("Config source: %s,  parameters:\n",CONFIG_GETSOURCE );
       for (int i=0; i < config_get_if()->num_cfgP; i++) {
            prnt("    %02i %s\n",i ,config_get_if()->cfgP[i]);
       }
       prnt("Softmodem components:\n");
       prnt("   %02i Ru(s)\n", RC.nb_RU);
       prnt("   %02i lte RRc(s),     %02i NbIoT RRC(s)\n",    RC.nb_inst, RC.nb_nb_iot_rrc_inst);
       prnt("   %02i lte MACRLC(s),  %02i NbIoT MACRLC(s)\n", RC.nb_macrlc_inst, RC.nb_nb_iot_macrlc_inst);
       prnt("   %02i lte L1,	    %02i NbIoT L1\n",	     RC.nb_L1_inst, RC.nb_nb_iot_L1_inst);

       for(int i=0; i<RC.nb_inst; i++) {
           prnt("    lte RRC %i:     %02i CC(s) \n",i,((RC.nb_CC == NULL)?0:RC.nb_CC[i]));
       }
       for(int i=0; i<RC.nb_L1_inst; i++) {
           prnt("    lte L1 %i:      %02i CC(s)\n",i,((RC.nb_L1_CC == NULL)?0:RC.nb_L1_CC[i]));
       }
       for(int i=0; i<RC.nb_macrlc_inst; i++) {
           prnt("    lte macrlc %i:  %02i CC(s)\n",i,((RC.nb_mac_CC == NULL)?0:RC.nb_mac_CC[i]));
       }
   }
   return 0;
} 

int proccmd_thread(char *buf, int debug, telnet_printfunc_t prnt)
{
int bv1,bv2;   
int res;
char sv1[64];

if (buf == NULL) {
  prnt("ERROR wrong thread command...\n");
  return 0;
}
   bv1=0;
   bv2=0;
   sv1[0]=0;
   if (debug > 0)
       prnt("proccmd_thread received %s\n",buf);
   if (strcasestr(buf,"help") != NULL) {
          prnt(PROCCMD_THREAD_HELP_STRING);
          return 0;
   } 
   res=sscanf(buf,"%i %9s %i",&bv1,sv1,&bv2);
   if (debug > 0)
       prnt(" proccmd_thread: %i params = %i,%s,%i\n",res,bv1,sv1,bv2);   
   if(res != 3)
     {
     print_threads(buf, debug, prnt);
     return 0;
     }

  
   if (strcasestr(sv1,"prio") != NULL)
       {
       set_sched(0,bv1, bv2);
       }
   else if (strcasestr(sv1,"aff") != NULL)
       {
       set_affinity(0,bv1, bv2);
       }
   else
       {
       prnt("%s is not a valid thread command\n",sv1);
       }
   return 0;
} 
int proccmd_exit(char *buf, int debug, telnet_printfunc_t prnt)
{
   if (debug > 0)
       prnt("process module received %s\n",buf);
   exit_fun("telnet server received exit command\n");
   return 0;
}

int proccmd_restart(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (debug > 0)
       prnt("process module received %s\n", buf);
  configmodule_interface_t *cfg = config_get_if();
  end_configmodule(cfg);
  execvpe(cfg->argv[0], cfg->argv, environ);
  return 0;
}

int proccmd_log(char *buf, int debug, telnet_printfunc_t prnt)
{
int idx1=0;
int idx2=NUM_LOG_LEVEL-1;
char *logsubcmd=NULL;

int s = sscanf(buf,"%ms %i-%i\n",&logsubcmd, &idx1,&idx2);   
   
   if (debug > 0)
       prnt( "proccmd_log received %s\n   s=%i sub command %s\n",buf,s,((logsubcmd==NULL)?"":logsubcmd));

   if (s == 1 && logsubcmd != NULL) {
      if (strcasestr(logsubcmd,"online") != NULL) {
          if (strcasestr(buf,"noonline") != NULL) {
   	      set_glog_onlinelog(0);
              prnt("online logging disabled\n",buf);
          } else {
   	      set_glog_onlinelog(1);
              prnt("online logging enabled\n",buf);
          }
      }
      else if (strcasestr(logsubcmd,"show") != NULL) {
          prnt("Available log levels: \n   ");
          for (int i=0; log_level_names[i].name != NULL; i++)
             prnt("%s ",log_level_names[i].name);
          prnt("\n\n");
          prnt("Available display options: \n   ");
          for (int i=0; log_options[i].name != NULL; i++)
             prnt("%s ",log_options[i].name);
          prnt("\n\n");
          prnt("Available debug and dump options: \n   ");
          for (int i=0; log_maskmap[i].name != NULL; i++)
             prnt("%s ",log_maskmap[i].name);
          prnt("\n\n");
   	  proccmd_show("loglvl",debug,prnt);
   	  proccmd_show("logopt",debug,prnt);
   	  proccmd_show("dbgopt",debug,prnt);
      }
      else if (strcasestr(logsubcmd,"help") != NULL) {
          prnt(PROCCMD_LOG_HELP_STRING);
      } else {
          prnt("%s: wrong log command...\n",logsubcmd);
      }
   } else if ( s == 2 && logsubcmd != NULL) {
      char *opt=NULL;
      char *logparam=NULL;
      int  l;
      int optbit;

      l=sscanf(logsubcmd,"%m[^'_']_%ms",&logparam,&opt);
      if (l == 2 && strcmp(logparam,"print") == 0){
         optbit=map_str_to_int(log_options,opt);
         if (optbit < 0) {
            prnt("option %s unknown\n",opt);
         } else {
            if (idx1 > 0)    
                SET_LOG_OPTION(optbit);
            else
                CLEAR_LOG_OPTION(optbit);
            proccmd_show("logopt",debug,prnt);
         }
      }
      else if (l == 2 && strcmp(logparam,"debug") == 0){
         optbit=map_str_to_int(log_maskmap,opt);
         if (optbit < 0) {
            prnt("module %s unknown\n",opt);
         } else {
            if (idx1 > 0)    
                SET_LOG_DEBUG(optbit);
            else
                CLEAR_LOG_DEBUG(optbit);
            proccmd_show("dbgopt",debug,prnt);
         }
      }  
       else if (l == 2 && strcmp(logparam,"dump") == 0){
         optbit=map_str_to_int(log_maskmap,opt);
         if (optbit < 0) {
            prnt("module %s unknown\n",opt);
         } else {
            if (idx1 > 0)    
                SET_LOG_DUMP(optbit);
            else
                CLEAR_LOG_DUMP(optbit);
            proccmd_show("dump", debug, prnt);
         }
      }       
      if (logparam != NULL) free(logparam);
      if (opt != NULL)      free(opt); 
   } else if ( s == 3 && logsubcmd != NULL) {
      int level, enable,filelog;
      char *tmpstr=NULL;
      char *logparam=NULL;
      int l;

      level = OAILOG_DISABLE - 1;
      filelog = -1;
      enable=-1; 
      l=sscanf(logsubcmd,"%m[^'_']_%m[^'_']",&logparam,&tmpstr);
      if (debug > 0)
          prnt("l=%i, %s %s\n",l,((logparam==NULL)?"\"\"":logparam), ((tmpstr==NULL)?"\"\"":tmpstr));
      if (l ==2 ) {
         if (strcmp(logparam,"level") == 0) {
             level=map_str_to_int(log_level_names,tmpstr);
             if (level < 0) {
                 prnt("level %s unknown\n",tmpstr);
                 level=OAILOG_DISABLE - 1;
              }
         } else {
             prnt("%s%s unknown log sub command \n",logparam, tmpstr);
         }
      } else if (l ==1 ) {
         if (strcmp(logparam,"enable") == 0) {
            enable=1;
         } else if (strcmp(logparam,"disable") == 0) {
             level=OAILOG_DISABLE;
         } else if (strcmp(logparam,"file") == 0) {
             filelog = 1 ;
         } else if (strcmp(logparam,"nofile") == 0) {
             filelog = 0 ;
         } else {
             prnt("%s%s unknown log sub command \n",logparam, tmpstr);
         }
      } else {
        level = map_str_to_int(log_level_names, tmpstr);
        prnt("%s unknown log sub command \n", logsubcmd);
      }
      if (logparam != NULL) free(logparam);
      if (tmpstr != NULL)   free(tmpstr);
      for (int i=idx1; i<=idx2 ; i++) {
        if (level >= OAILOG_DISABLE)
           set_log(i, level);
        else if ( enable == 1)
           set_log(i,g_log->log_component[i].savedlevel);
        else if ( filelog == 1 ) {
           set_component_filelog(i);
        } else if ( filelog == 0 ) {
           close_component_filelog(i);
        } 
          
      }
     proccmd_show("loglvl",debug,prnt);
   } else {
       prnt("%s: wrong log command...\n",buf);
   }

   return 0;
} 
/*-------------------------------------------------------------------------------------*/

void add_softmodem_cmds(void)
{
   add_telnetcmd("softmodem",proc_vardef,proc_cmdarray);
}
