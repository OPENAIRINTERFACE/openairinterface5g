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

/*! \file common/utils/telnetsrv/telnetsrv.c
 * \brief: implementation of a telnet server
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
#include "telnetsrv.h"
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "common/utils/load_module_shlib.h"
#include "common/config/config_userapi.h"
#include "common/utils/threadPool/thread-pool.h"
#include "executables/softmodem-common.h"
#include <readline/history.h>


#include "telnetsrv_phycmd.h"
#include "telnetsrv_proccmd.h"
static char *telnet_defstatmod[] = {"softmodem","phy","loader","measur"};
static telnetsrv_params_t telnetparams;

#define TELNETSRV_OPTNAME_STATICMOD   "staticmod"
#define TELNETSRV_OPTNAME_SHRMOD      "shrmod"

// clang-format off
paramdef_t telnetoptions[] = {
    /*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    /*                                            configuration parameters for telnet utility                                                                                      */
    /*   optname                              helpstr                paramflags           XXXptr                               defXXXval               type                 numelt */
    /*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    {"listenaddr", "<listen ip address>\n", 0, .uptr = &telnetparams.listenaddr, .defstrval = "0.0.0.0", TYPE_IPV4ADDR, 0},
    {"listenport", "<local port>\n", 0, .uptr = &telnetparams.listenport, .defuintval = 9090, TYPE_UINT, 0},
    {"listenstdin", "enable input from stdin\n", PARAMFLAG_BOOL, .uptr = &telnetparams.listenstdin, .defuintval = 0, TYPE_UINT, 0},
    {"priority", "<scheduling policy (0-99)\n", 0, .iptr = &telnetparams.priority, .defuintval = 0, TYPE_INT, 0},
    {"debug", "<debug level>\n", 0, .uptr = NULL, .defuintval = 0, TYPE_UINT, 0},
    {"loopcount", "<loop command iterations>\n", 0, .uptr = &telnetparams.loopcount, .defuintval = 10, TYPE_UINT, 0},
    {"loopdelay", "<loop command delay (ms)>\n", 0, .uptr = &telnetparams.loopdelay, .defuintval = 5000, TYPE_UINT, 0},
    {"histfile", "<history file name>\n", PARAMFLAG_NOFREE, .strptr = &telnetparams.histfile, .defstrval = "oaitelnet.history", TYPE_STRING, 0},
    {"histsize", "<history sizes>\n", 0, .iptr = &telnetparams.histsize, .defuintval = 50, TYPE_INT, 0},
    {"logfile", "log file when redirecting", PARAMFLAG_NOFREE, .strptr = &telnetparams.logfile, .defstrval = "oaisoftmodem.log", TYPE_STRING, 0},
    {"phypbsize", "<phy dump buff size (bytes)>\n", 0, .uptr = &telnetparams.phyprntbuff_size, .defuintval = 65000, TYPE_UINT, 0},
    {TELNETSRV_OPTNAME_STATICMOD, "<static modules selection>\n", 0, .strlistptr = NULL, .defstrlistval = telnet_defstatmod, TYPE_STRINGLIST, (sizeof(telnet_defstatmod) / sizeof(char *))},
    {TELNETSRV_OPTNAME_SHRMOD, "<dynamic modules selection>\n", 0, .strlistptr = NULL, .defstrlistval = NULL, TYPE_STRINGLIST, 0}
};
// clang-format on

int get_phybsize(void) {
  return telnetparams.phyprntbuff_size;
};
int add_telnetcmd(char *modulename,telnetshell_vardef_t *var, telnetshell_cmddef_t *cmd );
int setoutput(char *buff, int debug, telnet_printfunc_t prnt);
int wsetoutput(char *buff, int debug, telnet_printfunc_t prnt, ...);
int setparam(char *buff, int debug, telnet_printfunc_t prnt);
int wsetparam(char *buff, int debug, telnet_printfunc_t prnt, ...);
int history_cmd(char *buff, int debug, telnet_printfunc_t prnt);

telnetshell_vardef_t telnet_vardef[] = {{"debug", TELNET_VARTYPE_INT32, 0, &telnetparams.telnetdbg},
                                        {"prio", TELNET_VARTYPE_INT32, 0, &telnetparams.priority},
                                        {"loopc", TELNET_VARTYPE_INT32, 0, &telnetparams.loopcount},
                                        {"loopd", TELNET_VARTYPE_INT32, 0, &telnetparams.loopdelay},
                                        {"phypb", TELNET_VARTYPE_INT32, 0, &telnetparams.phyprntbuff_size},
                                        {"hsize", TELNET_VARTYPE_INT32, 0, &telnetparams.histsize},
                                        {"hfile", TELNET_VARTYPE_STRING, TELNET_CHECKVAL_RDONLY, &telnetparams.histfile},
                                        {"logfile", TELNET_VARTYPE_STRING, 0, &telnetparams.logfile},
                                        {"", 0, 0, NULL}};

telnetshell_cmddef_t telnet_cmdarray[] = {
    {"redirlog", "[here,file,off]", setoutput, {NULL}, TELNETSRV_CMDFLAG_TELNETONLY, NULL},
    {"redirlog file", "", setoutput, {NULL}, TELNETSRV_CMDFLAG_WEBSRVONLY, NULL},
    {"redirlog off", "", setoutput, {NULL}, TELNETSRV_CMDFLAG_WEBSRVONLY, NULL},
    {"param", "[prio]", setparam, {wsetparam}, 0, NULL},
    {"history", "[list,reset]", history_cmd, {NULL}, TELNETSRV_CMDFLAG_TELNETONLY, NULL},
    {"", "", NULL, {NULL}, 0, NULL},
};

void client_printf(const char *message, ...) {
  va_list va_args;
  va_start(va_args, message);

  if (telnetparams.new_socket > 0) {
    vsnprintf(telnetparams.msgbuff,sizeof(telnetparams.msgbuff)-1,message, va_args);
    send(telnetparams.new_socket,telnetparams.msgbuff, strlen(telnetparams.msgbuff), MSG_NOSIGNAL);
  } else {
    vprintf(message, va_args);
  }

  va_end(va_args);
  return ;
}

void set_sched(pthread_t tid, int pid, int priority) {
  int rt;
  struct sched_param schedp;
  int policy;
  char strpolicy[10];
  int niceval = 0;

  if (priority < -100 && priority > -200) { // RR priority 1 to 99 (high) mapped to oai priority -101 to -199 (high)
    policy = SCHED_RR;
    sprintf(strpolicy, "%s", "RR");
    schedp.sched_priority = -(priority + 100);
  } else if (priority < 0 && priority > -100) {
    policy=SCHED_FIFO;
    sprintf(strpolicy,"%s","fifo");
    schedp.sched_priority = -priority; // fifo priority 1 to 99 (high) mapped to oai priority -1 to -99 (high)
  } else if (priority >= 0 && priority <= (NICE_MAX - NICE_MIN)) { // other (normal) nice value -20 to 19 mapped to oai priority 0 to 39
    policy = SCHED_OTHER;
    sprintf(strpolicy, "%s", "other");
    schedp.sched_priority = 0;
    niceval = priority + NICE_MIN;
  } else if (priority > NICE_MAX - NICE_MIN && priority <= (2 * (NICE_MAX - NICE_MIN) + 1)) { // batch (normal) nice value -20 to 19 mapped to oai priority 40 to 79
    policy = SCHED_BATCH;
    sprintf(strpolicy, "%s", "batch");
    niceval = priority + NICE_MIN - (NICE_MAX - NICE_MIN + 1);
    schedp.sched_priority = 0;
  } else if (priority > (2 * (NICE_MAX - NICE_MIN) + 1)) { // idle  mapped to oai priority >79
    policy=SCHED_IDLE;
    sprintf(strpolicy,"%s","idle");
    schedp.sched_priority=0;
  } else {
    client_printf("Error: %i invalid priority \n", priority);
    return;
  }
  if( tid != 0) {
    rt = pthread_setschedparam(tid, policy, &schedp);
  } else if(pid > 0)  {
    rt = sched_setscheduler( pid, policy,&schedp);
  } else {
    rt= -1;
    client_printf("Error: no pid or tid specified\n");
  }

  if (rt != 0) {
    client_printf("Error %i: %s modifying sched param to %s:%i, \n",
                  errno,strerror(errno),strpolicy,schedp.sched_priority);
  } else  {
    client_printf("policy set to %s, priority %i\n",strpolicy,schedp.sched_priority);

    if (policy == SCHED_OTHER || policy == SCHED_BATCH) {
      rt = getpriority(PRIO_PROCESS,tid);

      if (rt != -1) {
        rt = setpriority(PRIO_PROCESS, tid, niceval);

        if (rt < 0) {
          client_printf("Error %i: %s trying to set nice value of thread %u to %i\n", errno, strerror(errno), tid, niceval);
        }
      } else {
        client_printf("Error %i: %s trying to get nice value of thread %u \n",
                      errno,strerror(errno),tid);
      }
    }
  }

  if (policy == SCHED_OTHER || policy == SCHED_BATCH) {
    if ( tid > 0 && tid != pthread_self()) {
      client_printf("setting nice value using a thread id not implemented....\n");
    } else if (pid > 0) {
      errno=0;
      rt = setpriority(PRIO_PROCESS, pid, niceval);

      if (rt != 0) {
        client_printf("Error %i: %s calling setpriority, \n",errno,strerror(errno));
      } else {
        client_printf("nice value set to %i\n", niceval);
      }
    }
  }
}

void set_affinity(pthread_t tid, int pid, int coreid) {
  cpu_set_t cpuset;
  int rt;
  CPU_ZERO(&cpuset);
  CPU_SET(coreid, &cpuset);

  if (tid > 0) {
    rt = pthread_setaffinity_np((pthread_t)tid, sizeof(cpu_set_t), &cpuset);
  } else if (pid > 0) {
    rt = sched_setaffinity((pid_t)pid, sizeof(cpu_set_t), &cpuset);
  } else {
    rt= -1;
  }

  if (rt != 0) {
    client_printf("Error %i: %s calling , xxx_setaffinity...\n",errno,strerror(errno));
  } else {
    client_printf("thread %i affinity set to %i\n",(pid==0)?(int)tid:pid,coreid);
  }
}
/*------------------------------------------------------------------------------------*/
/*
function implementing telnet server specific commands, parameters of the
telnet_cmdarray table
*/

void redirstd(char *newfname,telnet_printfunc_t prnt ) {
  FILE *fd;
  fd=freopen(newfname, "w", stdout);

  if (fd == NULL) {
    prnt("ERROR: stdout redir to %s error %s\n", strerror(errno));
  } else {
    prnt("stdout redirected to %s\n", newfname);
  }

  fd=freopen(newfname, "w", stderr);

  if (fd == NULL) {
    prnt("ERROR: stderr redir to %s error %s\n", strerror(errno));
  } else {
    prnt("stderr redirected to %s\n", newfname);
  }
}

int wsetoutput(char *buffer, int debug, telnet_printfunc_t prnt, ...)
{
  return 0;
}

int setoutput(char *buff, int debug, telnet_printfunc_t prnt) {
  char cmds[TELNET_MAX_MSGLENGTH/TELNET_CMD_MAXSIZE][TELNET_CMD_MAXSIZE];
  char *logfname;
  char stdout_str[64];

  memset(cmds,0,sizeof(cmds));
  sscanf(buff,"%9s %32s %9s %9s %9s", cmds[0],cmds[1],cmds[2],cmds[3],cmds[4]  );

  if (strncasecmp(cmds[0],"here",4) == 0) {
    fflush(stdout);
    sprintf(stdout_str,"/proc/%i/fd/%i",getpid(),telnetparams.new_socket);
    dup2(telnetparams.new_socket,fileno(stdout));
    //   freopen(stdout_str, "w", stdout);
    //   freopen(stdout_str, "w", stderr);
    dup2(telnetparams.new_socket,fileno(stderr));
    prnt("Log output redirected to this terminal (%s)\n",stdout_str);
  }

  if (strncasecmp(cmds[0],"file",4) == 0) {
    if (cmds[1][0] == 0)
      logfname = telnetparams.logfile;
    else
      logfname=cmds[1];
    prnt("Log output redirected to (%s)\n", logfname);
    fflush(stdout);
    redirstd(logfname,prnt);
  }

  if (strncasecmp(cmds[0],"off",3) == 0) {
    fflush(stdout);
    redirstd("/dev/tty",prnt);
  }

  return CMDSTATUS_FOUND;
} /* setoutput */

int wsetparam(char *buff, int debug, telnet_printfunc_t prnt, ...)
{
  return 0;
}

int setparam(char *buff, int debug, telnet_printfunc_t prnt) {
  char cmds[TELNET_MAX_MSGLENGTH/TELNET_CMD_MAXSIZE][TELNET_CMD_MAXSIZE];
  memset(cmds,0,sizeof(cmds));
  sscanf(buff,"%9s %9s %9s %9s %9s", cmds[0],cmds[1],cmds[2],cmds[3],cmds[4]  );

  if (strncasecmp(cmds[0],"prio",4) == 0) {
    int prio;
    prio=(int)strtol(cmds[1],NULL,0);

    if (errno == ERANGE)
      return CMDSTATUS_VARNOTFOUND;

    telnetparams.priority = prio;
    set_sched(pthread_self(),0,prio);
    return CMDSTATUS_FOUND;
  }

  if (strncasecmp(cmds[0],"aff",3) == 0) {
    int aff;
    aff=(int)strtol(cmds[1],NULL,0);

    if (errno == ERANGE)
      return CMDSTATUS_VARNOTFOUND;

    set_affinity(pthread_self(),0,aff);
    return CMDSTATUS_FOUND;
  }

  return CMDSTATUS_NOTFOUND;
} /* setparam */

int history_cmd(char *buff, int debug, telnet_printfunc_t prnt) {
  char cmds[TELNET_MAX_MSGLENGTH/TELNET_CMD_MAXSIZE][TELNET_CMD_MAXSIZE];
  memset(cmds,0,sizeof(cmds));
  sscanf(buff,"%9s %9s %9s %9s %9s", cmds[0],cmds[1],cmds[2],cmds[3],cmds[4]  );

  if (strncasecmp(cmds[0],"list",4) == 0) {
    HIST_ENTRY **hist = history_list();

    if (hist) {
      for (int i = 0; hist[i]; i++) {
        prnt ("%d: %s\n", i + history_base, hist[i]->line);
      }
    }

    return CMDSTATUS_FOUND;
  }

  if (strncasecmp(cmds[0],"reset",5) == 0) {
    clear_history();
    write_history(telnetparams.histfile);
    return CMDSTATUS_FOUND;
  }

  return CMDSTATUS_NOTFOUND;
} /* history_cmd */
/*-------------------------------------------------------------------------------------------------------*/
/*
generic commands available for all modules loaded by the server
*/
char *telnet_getvarvalue(telnetshell_vardef_t *var, int varindex)
{
  char *val;
  switch (var[varindex].vartype) {
    case TELNET_VARTYPE_INT32:
      val = malloc(64);
      snprintf(val, 64, "%i", *(int32_t *)(var[varindex].varvalptr));
      break;

    case TELNET_VARTYPE_INT64:
      val = malloc(128);
      snprintf(val, 128, "%lli", (long long)*(int64_t *)(var[varindex].varvalptr));
      break;

    case TELNET_VARTYPE_INT16:
      val = malloc(16);
      snprintf(val, 16, "%hi", *(short *)(var[varindex].varvalptr));
      break;

    case TELNET_VARTYPE_INT8:
      val = malloc(16);
      snprintf(val, 16, "%i", (int)(*(int8_t *)(var[varindex].varvalptr)));
      break;

    case TELNET_VARTYPE_UINT:
      val = malloc(64);
      snprintf(val, 64, "%u", *(unsigned int *)(var[varindex].varvalptr));
      break;

    case TELNET_VARTYPE_DOUBLE:
      val = malloc(32);
      snprintf(val, 32, "%g\n", *(double *)(var[varindex].varvalptr));
      break;

    case TELNET_VARTYPE_STRING:
      val = strdup(*(char **)(var[varindex].varvalptr));
      break;

    default:
      val = malloc(64);
      snprintf(val, 64, "ERR:var %i unknown type", varindex);
      break;
  }
  return val;
}

int telnet_setvarvalue(telnetshell_vardef_t *var, char *strval, telnet_printfunc_t prnt)
{
  int st = 0;
  switch (var->vartype) {
    case TELNET_VARTYPE_INT32:
      *(int *)(var->varvalptr) = (int)strtol(strval, NULL, 0);
      if (prnt != NULL)
        prnt("%i\n", *(int *)(var->varvalptr));
      break;

    case TELNET_VARTYPE_INT16:
      *(short *)(var->varvalptr) = (short)strtol(strval, NULL, 0);
      if (prnt != NULL)
        prnt("%hi\n", *(short *)(var->varvalptr));
      break;

    case TELNET_VARTYPE_INT8:
      *(char *)(var->varvalptr) = (char)strtol(strval, NULL, 0);
      if (prnt != NULL)
        prnt("%i\n", *(int *)(var->varvalptr));
      break;

    case TELNET_VARTYPE_UINT:
      *(unsigned int *)(var->varvalptr) = (unsigned int)strtol(strval, NULL, 0);
      if (prnt != NULL)
        prnt("%u\n", *(unsigned int *)(var->varvalptr));
      break;

    case TELNET_VARTYPE_DOUBLE:
      *(double *)(var->varvalptr) = strtod(strval, NULL);
      if (prnt != NULL)
        prnt("%g\n", *(double *)(var->varvalptr));
      break;

    case TELNET_VARTYPE_STRING:
      sprintf(*(char **)(var->varvalptr), "%s", strval);
      if (prnt != NULL)
        prnt("\"%s\"\n", *(char **)(var->varvalptr));
      break;

    default:
      if (prnt != NULL)
        prnt("unknown type\n");
      st = -1;
      break;
  }
  return st;
}

int setgetvar(int moduleindex, char getorset, char *params)
{
  int n, i;
  char varname[TELNET_CMD_MAXSIZE];
  char *varval = NULL;
  memset(varname, 0, sizeof(varname));
  n = sscanf(params, "%9s %ms", varname, &varval);

  for (i = 0; telnetparams.CmdParsers[moduleindex].var[i].varvalptr != NULL; i++) {
    if (strncasecmp(telnetparams.CmdParsers[moduleindex].var[i].varname, varname, strlen(telnetparams.CmdParsers[moduleindex].var[i].varname)) == 0) {
      if (n > 0 && (getorset == 'g' || getorset == 'G')) {
        client_printf("%s, %s = ", telnetparams.CmdParsers[moduleindex].module, telnetparams.CmdParsers[moduleindex].var[i].varname);
        char *strval = telnet_getvarvalue(telnetparams.CmdParsers[moduleindex].var, i);
        client_printf("%s\n", strval);
        free(strval);
      }
      if (n > 1 && (getorset == 's' || getorset == 'S')) {
        client_printf("%s, %s set to \n", telnetparams.CmdParsers[moduleindex].module, telnetparams.CmdParsers[moduleindex].var[i].varname);
        telnet_setvarvalue(&(telnetparams.CmdParsers[moduleindex].var[i]), varval, client_printf);
      }
    }
  }

  if (n>1 && varval != NULL) {
    free(varval);
  }

  return CMDSTATUS_VARNOTFOUND;
}

void telnetsrv_freetbldata(webdatadef_t *wdata)
{
  for (int i = 0; i < wdata->numlines; i++)
    for (int j = 0; j < wdata->numcols; j++)
      if (wdata->columns[j].coltype & TELNET_VAR_NEEDFREE)
        free(wdata->lines[i].val[j]);
}
/*----------------------------------------------------------------------------------------------------*/
char *get_time(char *buff,int bufflen) {
  struct tm  tmstruct;
  time_t now = time (0);
  strftime (buff, bufflen, "%Y-%m-%d %H:%M:%S.000", localtime_r(&now,&tmstruct));
  return buff;
}
void telnet_pushcmd(telnetshell_cmddef_t *cmd, char *cmdbuff, telnet_printfunc_t prnt)
{
  notifiedFIFO_elt_t *msg = newNotifiedFIFO_elt(sizeof(telnetsrv_qmsg_t), 0, NULL, NULL);
  telnetsrv_qmsg_t *cmddata = NotifiedFifoData(msg);
  cmddata->cmdfunc = (qcmdfunc_t)cmd->cmdfunc;
  cmddata->prnt = prnt;
  cmddata->debug = telnetparams.telnetdbg;
  if (cmdbuff != NULL)
    cmddata->cmdbuff = strdup(cmdbuff);
  pushNotifiedFIFO(cmd->qptr, msg);
}

int process_command(char *buf, int iteration)
{
  int i,j,k;
  char modulename[TELNET_CMD_MAXSIZE];
  char cmd[TELNET_CMD_MAXSIZE];
  char *cmdb=NULL;
  int rt;
  memset(modulename,0,sizeof(modulename));
  memset(cmd,0,sizeof(cmd));


  if (strncasecmp(buf,"ex",2) == 0)
    return CMDSTATUS_EXIT;

  if (strncasecmp(buf,"help",4) == 0) {
    for (i=0; telnetparams.CmdParsers[i].var != NULL && telnetparams.CmdParsers[i].cmd != NULL; i++) {
      client_printf("   module %i = %s:\n",i,telnetparams.CmdParsers[i].module);

      for(j=0; telnetparams.CmdParsers[i].var[j].varvalptr != NULL ; j++) {
        client_printf("      %s [get set] %s <value>\n",
                      telnetparams.CmdParsers[i].module, telnetparams.CmdParsers[i].var[j].varname);
      }

      for(j=0; telnetparams.CmdParsers[i].cmd[j].cmdfunc != NULL ; j++) {
        if (telnetparams.CmdParsers[i].cmd[j].cmdflags & TELNETSRV_CMDFLAG_WEBSRVONLY)
          continue;
        client_printf("      %s %s %s\n",
                      telnetparams.CmdParsers[i].module,telnetparams.CmdParsers[i].cmd[j].cmdname,
                      telnetparams.CmdParsers[i].cmd[j].helpstr);
      }
    }

    return CMDSTATUS_FOUND;
  }

  rt=CMDSTATUS_NOTFOUND;
  j = sscanf(buf,"%19s %19s %m[^\t\n]",modulename,cmd,&cmdb);

  if (telnetparams.telnetdbg > 0)
    printf("process_command: %i words, module=%s cmd=%s, parameters= %s\n", j, modulename, cmd, (cmdb == NULL) ? "" : cmdb);

  for (i=0; j>=2 && telnetparams.CmdParsers[i].var != NULL && telnetparams.CmdParsers[i].cmd != NULL; i++) {
    if ( (strncasecmp(telnetparams.CmdParsers[i].module,modulename,strlen(telnetparams.CmdParsers[i].module)) == 0)) {
      if (strncasecmp(cmd,"getall",7) == 0 ) {
        for(j=0; telnetparams.CmdParsers[i].var[j].varvalptr != NULL ; j++) {
          setgetvar(i,'g',telnetparams.CmdParsers[i].var[j].varname);
        }

        rt= CMDSTATUS_FOUND;
      } else if (strcasecmp(cmd,"get") == 0 || strcasecmp(cmd,"set") == 0) {
        rt= setgetvar(i,cmd[0],cmdb);
      } else {
        for (k=0 ; telnetparams.CmdParsers[i].cmd[k].cmdfunc != NULL ; k++) {
          if (strncasecmp(cmd, telnetparams.CmdParsers[i].cmd[k].cmdname,sizeof(telnetparams.CmdParsers[i].cmd[k].cmdname)) == 0) {
            if (telnetparams.CmdParsers[i].cmd[k].cmdflags & TELNETSRV_CMDFLAG_WEBSRVONLY)
              continue;
            if (telnetparams.CmdParsers[i].cmd[k].qptr != NULL) {
              telnet_pushcmd(&(telnetparams.CmdParsers[i].cmd[k]), (cmdb != NULL) ? strdup(cmdb) : NULL, client_printf);
            } else {
              telnetparams.CmdParsers[i].cmd[k].cmdfunc(cmdb, telnetparams.telnetdbg, client_printf);
            }
            rt= CMDSTATUS_FOUND;
          }
        } /* for k */
      }/* else */
    }/* strncmp: module name test */
    else if (strncasecmp(modulename,"loop",4) == 0 ) {
      int f = fcntl(telnetparams.new_socket,F_GETFL);
      int f1=fcntl (telnetparams.new_socket, F_SETFL, O_NONBLOCK | f);

      if (f<0 || f1 <0) {
        client_printf( " Loop won't be cancelable: %s\n",strerror(errno) );
      }

      for(int lc=0; lc<telnetparams.loopcount; lc++) {
        char dummybuff[20];
        char tbuff[64];
        client_printf(CSI "1J" CSI "1;10H         " STDFMT "%s %i/%i\n",
                      get_time(tbuff,sizeof(tbuff)),lc,telnetparams.loopcount );
        process_command(buf + strlen("loop") + 1, lc);
        errno=0;
        int rs = read(telnetparams.new_socket,dummybuff,sizeof(dummybuff));

        if (telnetparams.telnetdbg > 0)
          client_printf("Received \"%s\" status %d, errno %s while running loop\n",dummybuff,rs,strerror(errno));

        if ( errno != EAGAIN && errno != EWOULDBLOCK) {
          client_printf( STDFMT " Loop canceled, iteration %i/%i\n",lc,telnetparams.loopcount );
          lc=telnetparams.loopcount;
          break;
        }

        usleep(telnetparams.loopdelay * 1000);
      }

      fcntl (telnetparams.new_socket, F_SETFL, f);
      rt= CMDSTATUS_FOUND;
    } /* loop */
  } /* for i */
  free(cmdb);
  return rt;
}

void run_telnetsrv(void) {
  int sock;
  struct sockaddr_in name;
  char buf[TELNET_MAX_MSGLENGTH];
  struct sockaddr cli_addr;
  unsigned int cli_len = sizeof(cli_addr);
  int readc, filled;
  int status;
  int optval = 1;
  char prompt[sizeof(TELNET_PROMPT_PREFIX)+10];
  pthread_setname_np(pthread_self(), "telnet");
  set_sched(pthread_self(),0,telnetparams.priority);
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0)
    fprintf(stderr,"[TELNETSRV] Error %s on socket call\n",strerror(errno));

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
  name.sin_family = AF_INET;

  if (telnetparams.listenaddr == 0)
    name.sin_addr.s_addr = INADDR_ANY;
  else
    name.sin_addr.s_addr = telnetparams.listenaddr;

  name.sin_port = htons((unsigned short)(telnetparams.listenport));

  if(bind(sock, (void *) &name, sizeof(name)))
    fprintf(stderr,"[TELNETSRV] Error %s on bind call\n",strerror(errno));

  if(listen(sock, 1) == -1)
    fprintf(stderr,"[TELNETSRV] Error %s on listen call\n",strerror(errno));

  using_history();
  int plen=sprintf(prompt,"%s_%s> ",TELNET_PROMPT_PREFIX,get_softmodem_function(NULL));
  printf("\nInitializing telnet server...\n");

  while( (telnetparams.new_socket = accept(sock, &cli_addr, &cli_len)) ) {
    printf("[TELNETSRV] Telnet client connected....\n");
    read_history(telnetparams.histfile);
    stifle_history(telnetparams.histsize);

    if(telnetparams.new_socket < 0)
      fprintf(stderr,"[TELNETSRV] Error %s on accept call\n",strerror(errno));

    while(telnetparams.new_socket>0) {
      filled = 0;
      memset(buf,0,sizeof(buf));

      while(filled < ( TELNET_MAX_MSGLENGTH-1)) {
        readc = recv(telnetparams.new_socket, buf+filled, TELNET_MAX_MSGLENGTH-filled-1, 0);

        if(!readc)
          break;

        filled += readc;

        if(buf[filled-1] == '\n') {
          buf[filled-1] = 0;
          break;
        }
      }

      if(!readc) {
        printf ("[TELNETSRV] Telnet Client disconnected.\n");
        break;
      }

      if (telnetparams.telnetdbg > 0)
        printf("[TELNETSRV] Command received: readc %i filled %i \"%s\"\n", readc, filled,buf);

      if (buf[0] == '!') {
        if (buf[1] == '!') {
          sprintf(buf,"%s","telnet history list");
        } else {
          HIST_ENTRY *hisentry = history_get(strtol(buf+1,NULL,0));

          if (hisentry) {
            char msg[TELNET_MAX_MSGLENGTH + plen +10];
            sprintf(buf,"%s",hisentry->line);
            sprintf(msg,"%s %s\n",prompt, hisentry->line);
            send(telnetparams.new_socket, msg, strlen(msg), MSG_NOSIGNAL);
          }
        }
      }

      if (strlen(buf) > 2 ) {
        status = process_command(buf, 0);
      } else
        status=CMDSTATUS_NOCMD;

      if (status != CMDSTATUS_EXIT) {
        if (status == CMDSTATUS_NOTFOUND) {
          char msg[TELNET_MAX_MSGLENGTH + 50];
          sprintf(msg,"Error: \n      %s\n is not a softmodem command\n",buf);
          send(telnetparams.new_socket, msg, strlen(msg), MSG_NOSIGNAL);
        } else if (status == CMDSTATUS_FOUND) {
          add_history(buf);
        }

        send(telnetparams.new_socket, prompt, strlen(prompt), MSG_NOSIGNAL);
      } else {
        printf ("[TELNETSRV] Closing telnet connection...\n");
        break;
      }
    }

    write_history(telnetparams.histfile);
    clear_history();
    close(telnetparams.new_socket);
    printf ("[TELNETSRV] Telnet server waitting for connection...\n");
  }

  close(sock);
  return;
}

void run_telnetclt(void) {
  int sock;
  struct sockaddr_in name;
  pthread_setname_np(pthread_self(), "telnetclt");
  set_sched(pthread_self(),0,telnetparams.priority);
  char prompt[sizeof(TELNET_PROMPT_PREFIX)+10];
  sprintf(prompt,"%s_%s> ",TELNET_PROMPT_PREFIX,get_softmodem_function(NULL));
  name.sin_family = AF_INET;
  struct in_addr addr;
  inet_aton("127.0.0.1", &addr) ;
  name.sin_addr.s_addr = addr.s_addr;   
  name.sin_port = htons((unsigned short)(telnetparams.listenport));
  while (1) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
      fprintf(stderr,"[TELNETSRV] Error %s on socket call\n",strerror(errno));

    if(connect(sock, (void *) &name, sizeof(name)))
      fprintf(stderr,"[TELNETSRV] Error %s on connect call\n",strerror(errno));
 
    struct timeval ts;
    ts.tv_sec = 1; // 1 second
    ts.tv_usec = 0;
    while (1) {
      fd_set fds;   
      FD_ZERO(&fds);
      FD_SET(sock, &fds);
      FD_SET(STDIN_FILENO , &fds);     
      // wait for data
      int nready = select(sock + 1, &fds, (fd_set *) 0, (fd_set *) 0, &ts);
      if (nready < 0) {
          perror("select. Error");
          break;                                                                                                                                   
      }
      else if (nready == 0) {
          ts.tv_sec = 1; // 1 second
          ts.tv_usec = 0;
      }
      else if ( FD_ISSET(sock, &fds)) {
          int rv; 
          char inbuf[TELNET_MAX_MSGLENGTH*2];
          memset(inbuf,0,sizeof(inbuf)); 
          rv = recv(sock , inbuf , sizeof(inbuf)-1 , 0);
          if (rv  > 0) {
				 printf("%s",inbuf);
			  }
          else if (rv == 0) {
              printf("Connection closed by the remote end\n\r");
              break;
          }
          else {
              perror("recv error");
              break;            
          }
      }       
      else if (FD_ISSET(STDIN_FILENO , &fds)) {
		char *inbuf=NULL;  
      	size_t inlen=0; 
        inlen = getline( &inbuf,&inlen, stdin);
        if ( inlen > 0 ) {
      	  if ( send(sock, inbuf,inlen, 0) < 0) 
              break;
          }
        free(inbuf); 
      }
    }
    close(sock);
    }
  return;
} /* run_telnetclt */

void poll_telnetcmdq(void *qid, void *arg) {
	notifiedFIFO_elt_t *msg = pollNotifiedFIFO((notifiedFIFO_t *)qid);
	
	if (msg != NULL) {
	  telnetsrv_qmsg_t *msgdata=NotifiedFifoData(msg);
	  msgdata->cmdfunc(msgdata->cmdbuff,msgdata->debug,msgdata->prnt,arg);
	  free(msgdata->cmdbuff);
	  delNotifiedFIFO_elt(msg);
	}
}
/*------------------------------------------------------------------------------------------------*/
/* load the commands delivered with the telnet server
 *
 *
 *
*/
static bool exec_moduleinit(char *modname) {
  void (*fptr)(void);
  char initfunc[TELNET_CMD_MAXSIZE+10];

  if (strlen(modname) > TELNET_CMD_MAXSIZE) {
    fprintf(stderr,"[TELNETSRV] module %s not loaded, name exceeds the %i size limit\n",
            modname, TELNET_CMD_MAXSIZE);
    return false;
  }

  sprintf(initfunc,"add_%s_cmds",modname);
  fptr = dlsym(RTLD_DEFAULT,initfunc);

  if ( fptr != NULL) {
    fptr();
    return true;
  }
  fprintf(stderr, "[TELNETSRV] couldn't find %s for module %s \n", initfunc, modname);
  return false;
}

int add_embeddedmodules(void) {
  int ret=0;
  int pindex = config_paramidx_fromname(telnetoptions, sizeofArray(telnetoptions), TELNETSRV_OPTNAME_STATICMOD);
  for(int i=0; i<telnetoptions[pindex].numelt; i++) {
    bool success = exec_moduleinit(telnetoptions[pindex].strlistptr[i]);
    if (success)
      ret++;
  }

  return ret;
}

int add_sharedmodules(void) {
  int ret=0;
  int pindex = config_paramidx_fromname(telnetoptions, sizeofArray(telnetoptions), TELNETSRV_OPTNAME_SHRMOD);
  for(int i=0; i<telnetoptions[pindex].numelt; i++) {
    char *name = telnetoptions[pindex].strlistptr[i];
    char libname[256];
    snprintf(libname, sizeof(libname), "telnetsrv_%s", name);
    load_module_shlib(libname, NULL, 0, NULL);
    bool success = exec_moduleinit(name);
    if (success)
      ret++;
  }

  return ret;
}

/* autoinit functions is called by the loader when the telnet shared library is
   dynamically loaded
*/
int telnetsrv_autoinit(void) {
  memset(&telnetparams,0,sizeof(telnetparams));
  config_get(config_get_if(), telnetoptions, sizeofArray(telnetoptions), "telnetsrv");
  /* possibly load a exec specific shared lib */
  char *execfunc=get_softmodem_function(NULL);
  char libname[64];
  sprintf(libname,"telnetsrv_%s",execfunc);
  load_module_shlib(libname,NULL,0,NULL);
  if(pthread_create(&telnetparams.telnet_pthread,NULL, (void *(*)(void *))run_telnetsrv, NULL) != 0) {
    fprintf(stderr,"[TELNETSRV] Error %s on pthread_create call\n",strerror(errno));
    return -1;
  }

  add_telnetcmd("telnet", telnet_vardef, telnet_cmdarray);
  add_embeddedmodules();
  add_sharedmodules();
  if ( telnetparams.listenstdin ) {
    if(pthread_create(&telnetparams.telnetclt_pthread,NULL, (void *(*)(void *))run_telnetclt, NULL) != 0) {
      fprintf(stderr,"[TELNETSRV] Error %s on pthread_create f() run_telnetclt \n",strerror(errno));
    return -1;
    }
  }  
  return 0;
}

/*---------------------------------------------------------------------------------------------*/
/* add_telnetcmd is used to add a set of commands to the telnet server. A module calls this
 * function at init time. the telnet server is delivered with a set of commands which
 * will be loaded or not depending on the telnet section of the config file
*/
int add_telnetcmd(char *modulename, telnetshell_vardef_t *var, telnetshell_cmddef_t *cmd)
{
  notifiedFIFO_t *afifo = NULL;

  if( modulename == NULL || var == NULL || cmd == NULL) {
    fprintf(stderr,"[TELNETSRV] Telnet server, add_telnetcmd: invalid parameters\n");
    return -1;
  }

  for (int i = 0; i < TELNET_MAXCMD; i++) {
    if (telnetparams.CmdParsers[i].var == NULL) {
      strncpy(telnetparams.CmdParsers[i].module, modulename, sizeof(telnetparams.CmdParsers[i].module) - 1);
      telnetparams.CmdParsers[i].cmd = cmd;
      telnetparams.CmdParsers[i].var = var;
      for (int j = 0; cmd[j].cmdfunc != NULL; j++) {
        if (cmd[j].cmdflags & TELNETSRV_CMDFLAG_PUSHINTPOOLQ) {
          if (afifo == NULL) {
            afifo = malloc(sizeof(notifiedFIFO_t));
            initNotifiedFIFO(afifo);
          }
          cmd[j].qptr = afifo;
        }
      }
      printf("[TELNETSRV] Telnet server: module %i = %s added to shell\n", i, telnetparams.CmdParsers[i].module);
      break;
    }
  }

  return 0;
}

/* function which will be called by the shared lib loader, to check shared lib version
   against main exec version. version mismatch not considered as fatal (interfaces not supposed to change)
*/
int  telnetsrv_checkbuildver(char *mainexec_buildversion, char **shlib_buildversion) {
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "standalone built: " __DATE__ __TIME__
#endif
  *shlib_buildversion = PACKAGE_VERSION;

  if (strcmp(mainexec_buildversion, *shlib_buildversion) != 0) {
    fprintf(stderr,"[TELNETSRV] shared lib version %s, doesn't match main version %s, compatibility should be checked\n",
            mainexec_buildversion,*shlib_buildversion);
  }

  return 0;
}

int telnetsrv_getfarray(loader_shlibfunc_t  **farray) {
  int const num_func_tln_srv = 3;	
  *farray = malloc(sizeof(loader_shlibfunc_t) * num_func_tln_srv);
  (*farray)[0].fname=TELNET_ADDCMD_FNAME;
  (*farray)[0].fptr=(int (*)(void) )add_telnetcmd;
  (*farray)[1].fname=TELNET_POLLCMDQ_FNAME;
  (*farray)[1].fptr = (int (*)(void))poll_telnetcmdq;
  (*farray)[2].fname = TELNET_PUSHCMD_FNAME;
  (*farray)[2].fptr = (int (*)(void))telnet_pushcmd;
  return (3);
}
/* for webserver interface, needs access to some telnet server paramaters */
telnetsrv_params_t *get_telnetsrv_params(void)
{
  return &telnetparams;
}
