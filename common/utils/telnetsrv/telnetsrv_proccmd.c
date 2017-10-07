/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
 
  while( 	procfile_fiels != NULL && fieldcnt < 42)
    {
    if (strlen(procfile_fiels) == 0)
       continue;
    fieldcnt++;
    sprintf(toksep," ");
    switch(fieldcnt)
       {
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

int rt;

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
   if (strcasestr(buf,"thread") != NULL)
       {
       print_threads(buf,debug,prnt);
       }
   if (strcasestr(buf,"loglvl") != NULL) {
       for (int i=MIN_LOG_COMPONENTS; i < MAX_LOG_COMPONENTS; i++){
            prnt("\t%s:\t%s\t%s\n",g_log->log_component[i].name, map_int_to_str(log_verbosity_names,g_log->log_component[i].flag),
	        map_int_to_str(log_level_names,g_log->log_component[i].level));
       }
   }
   return 0;
} 

int proccmd_thread(char *buf, int debug, telnet_printfunc_t prnt)
{
int bv1,bv2;   
int res;
char sv1[64]; 
char tname[32];  
   bv1=0;
   bv2=0;
   sv1[0]=0;
   if (debug > 0)
       prnt("proccmd_thread received %s\n",buf);
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
int s = sscanf(buf,"%*s %i-%i",&idx1,&idx2);   
   
   if (debug > 0)
       prnt("process module received %s\n",buf);

   if (strcasestr(buf,"enable") != NULL)
       {
       set_glog_onlinelog(1);
       }
   if (strcasestr(buf,"disable") != NULL)
       {
       set_glog_onlinelog(0);
       }
    if (strcasestr(buf,"show") != NULL)
       {
       proccmd_show("loglvl",debug,prnt);
       }      
   return 0;
} 
/*-------------------------------------------------------------------------------------*/

void add_softmodem_cmds()
{
   add_telnetcmd("softmodem",proc_vardef,proc_cmdarray);
}
