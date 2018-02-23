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
#include "log.h"
#include "log_extern.h"
#include "common/config/config_userapi.h"
#include "openair1/PHY/extern.h"
#include "telnetsrv_proccmd.h"

void decode_procstat(char *record, int debug, telnet_printfunc_t prnt)
{
char prntline[160];
char *procfile_fiels;
char *strtokptr;
char *lptr;
int fieldcnt;
char toksep[2];

  fieldcnt=0;	  
  procfile_fiels =strtok_r(record," ",&strtokptr);
  lptr= prntline;
/*http://man7.org/linux/man-pages/man5/proc.5.html gives the structure of the stat file */
 
  while( 	procfile_fiels != NULL && fieldcnt < 42) {
    long int policy;
    if (strlen(procfile_fiels) == 0)
       continue;
    fieldcnt++;
    sprintf(toksep," ");
    switch(fieldcnt) {
       case 1: /* id */
           lptr+=sprintf(lptr,"%9.9s ",procfile_fiels);
           sprintf(toksep,")");
       break;  
       case 2: /* name */
	   lptr+=sprintf(lptr,"%20.20s ",procfile_fiels+1);
       break;              
       case 3:   //thread state
           lptr+=sprintf(lptr,"  %c   ",procfile_fiels[0]);
       break;
       case 14:   //time in user mode
       case 15:   //time in kernel mode
           lptr+=sprintf(lptr,"%9.9s ",procfile_fiels);
       break;
       case 18:   //priority
       case 19:   //nice	       
           lptr+=sprintf(lptr,"%3.3s ",procfile_fiels);
       break;
       case 23:   //vsize	       
           lptr+=sprintf(lptr,"%9.9s ",procfile_fiels);
       break;
       case 39:   //processor	       
           lptr+=sprintf(lptr," %2.2s  ",procfile_fiels);
       break;
       case 41:   //policy	       
           lptr+=sprintf(lptr,"%3.3s ",procfile_fiels);
           policy=strtol(procfile_fiels,NULL,0);
           switch(policy) {
              case SCHED_FIFO:
                   lptr+=sprintf(lptr,"%s ","rt: fifo");
              break;
              case SCHED_OTHER:
                   lptr+=sprintf(lptr,"%s ","other");
              break;
              case SCHED_IDLE:
                   lptr+=sprintf(lptr,"%s ","idle");
              break;
              case SCHED_BATCH:
                   lptr+=sprintf(lptr,"%s ","batch");
              break;
              case SCHED_RR:
                   lptr+=sprintf(lptr,"%s ","rt: rr");
              break;
              case SCHED_DEADLINE:
                   lptr+=sprintf(lptr,"%s ","rt: deadline");
              break;
              default:
                   lptr+=sprintf(lptr,"%s ","????");
              break;
           }
       break;
       default:
       break;	       	       	       	       	       
    }/* switch on fieldcnr */  
    procfile_fiels =strtok_r(NULL,toksep,&strtokptr); 
  } /* while on proc_fields != NULL */
  prnt("%s\n",prntline); 
} /*decode_procstat */

void read_statfile(char *fname,int debug, telnet_printfunc_t prnt)
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
    decode_procstat(arecord, debug, prnt);
}

void print_threads(char *buf, int debug, telnet_printfunc_t prnt)
{
char aname[256];

DIR *proc_dir;
struct dirent *entry;



    prnt("  id          name            state   USRmod    KRNmod  prio nice   vsize   proc pol \n\n");
    snprintf(aname, sizeof(aname), "/proc/%d/stat", getpid());
    read_statfile(aname,debug,prnt);
    prnt("\n");
    snprintf(aname, sizeof(aname), "/proc/%d/task", getpid());
    proc_dir = opendir(aname);
    if (proc_dir == NULL)
       {
       prnt("Error: Couldn't open %s %i %s\n",aname,errno,strerror(errno));
       return;
       }
    
    while ((entry = readdir(proc_dir)) != NULL)
        {
        if(entry->d_name[0] == '.')
            continue;
	snprintf(aname, sizeof(aname), "/proc/%d/task/%s/stat", getpid(),entry->d_name);    
        read_statfile(aname,debug,prnt);      
        } /* while entry != NULL */
	closedir(proc_dir);
} /* print_threads */


int proccmd_show(char *buf, int debug, telnet_printfunc_t prnt)
{
extern log_t *g_log;   
   
   if (debug > 0)
       prnt(" proccmd_show received %s\n",buf);
   if (strcasestr(buf,"thread") != NULL) {
       print_threads(buf,debug,prnt);
   }
   if (strcasestr(buf,"loglvl") != NULL) {
       prnt("component                 verbosity  level  enabled\n");
       for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++) {
            prnt("%02i %17.17s:%10.10s%10.10s  %s\n",i ,g_log->log_component[i].name, 
                  map_int_to_str(log_verbosity_names,g_log->log_component[i].flag),
	          map_int_to_str(log_level_names,g_log->log_component[i].level),
                  ((g_log->log_component[i].interval>0)?"Y":"N") );
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
     prnt("softmodem thread needs 3 params, %i received\n",res);
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
extern void exit_fun(const char* s);   
   
   if (debug > 0)
       prnt("process module received %s\n",buf);

   exit_fun("telnet server received exit command\n");
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
          prnt("\nAvailable verbosity: \n   ");
          for (int i=0; log_verbosity_names[i].name != NULL; i++)
             prnt("%s ",log_verbosity_names[i].name);
          prnt("\n");
   	  proccmd_show("loglvl",debug,prnt);
      }
      else if (strcasestr(logsubcmd,"help") != NULL) {
          prnt(PROCCMD_LOG_HELP_STRING);
      } else {
          prnt("%s: wrong log command...\n",logsubcmd);
      }
   } else if ( s == 3 && logsubcmd != NULL) {
      int level, verbosity, interval;
      char *tmpstr=NULL;
      char *logparam=NULL;
      int l;

      level = verbosity = interval = -1;
      l=sscanf(logsubcmd,"%m[^'_']_%m[^'_']",&logparam,&tmpstr);
      if (debug > 0)
          prnt("l=%i, %s %s\n",l,((logparam==NULL)?"\"\"":logparam), ((tmpstr==NULL)?"\"\"":tmpstr));
      if (l ==2 ) {
         if (strcmp(logparam,"level") == 0) {
             level=map_str_to_int(log_level_names,tmpstr);
             if (level < 0)  prnt("level %s unknown\n",tmpstr);
         } else if (strcmp(logparam,"verbos") == 0) {
             verbosity=map_str_to_int(log_verbosity_names,tmpstr);
             if (verbosity < 0)  prnt("verbosity %s unknown\n",tmpstr);
         } else {
             prnt("%s%s unknown log sub command \n",logparam, tmpstr);
         }
      } else if (l ==1 ) {
         if (strcmp(logparam,"enable") == 0) {
              interval = 1;
         } else if (strcmp(logparam,"disable") == 0) {
              interval = 0;
         } else {
             prnt("%s%s unknown log sub command \n",logparam, tmpstr);
         }
      } else {
          prnt("%s unknown log sub command \n",logsubcmd); 
      }
      if (logparam != NULL) free(logparam);
      if (tmpstr != NULL)   free(tmpstr);
      for (int i=idx1; i<=idx2 ; i++) {
          set_comp_log(i, level, verbosity, interval);
          prnt("log level/verbosity  comp %i %s set to %s / %s (%s)\n",
                i,((g_log->log_component[i].name==NULL)?"":g_log->log_component[i].name),
                map_int_to_str(log_level_names,g_log->log_component[i].level),
                map_int_to_str(log_verbosity_names,g_log->log_component[i].flag),
                ((g_log->log_component[i].interval>0)?"enabled":"disabled"));

        
      }     
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
