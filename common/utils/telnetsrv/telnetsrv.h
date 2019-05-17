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
/*! \file common/utils/telnetsrv/telnetsrv.h
 * \brief: include file for telnet server implementation
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#ifndef TELNETSRV_H
#define TELNETSRV_H

#define TELNETSRV_MODNAME  "telnetsrv"

#define TELNET_PORT               9090
#define TELNET_MAX_MSGLENGTH      2048
#define TELNET_PROMPT             "softmodem> "
#define TELNET_MAXCMD             20
#define TELNET_CMD_MAXSIZE        20
#define TELNET_HELPSTR_SIZE       80

/* status return by the command parser after it analysed user input */
#define CMDSTATUS_NOCMD       0
#define CMDSTATUS_EXIT        1
#define CMDSTATUS_FOUND       2
#define CMDSTATUS_VARNOTFOUND 3
#define CMDSTATUS_NOTFOUND    4

/*----------------------------------------------------------------------------*/
/* structure to be used when adding a module to the telnet server */
/* This is the second parameter of the add_telnetcmd function, which can be used   */
/* to add a set of new command to the telnet server shell */
typedef void(*telnet_printfunc_t)(const char* format, ...);
typedef int(*cmdfunc_t)(char*, int, telnet_printfunc_t prnt);

typedef struct cmddef {
    char cmdname[TELNET_CMD_MAXSIZE];
    char helpstr[TELNET_HELPSTR_SIZE];
    cmdfunc_t cmdfunc; 
} telnetshell_cmddef_t;

/*----------------------------------------------------------------------------*/
/*structure to be used when adding a module to the telnet server */
/* This is the first parameter of the add_telnetcmd function, which can be used   */
/* to add a set of new variables which can be got/set from the telnet server shell */
#define TELNET_VARTYPE_INT32  1
#define TELNET_VARTYPE_INT16  2
#define TELNET_VARTYPE_INT64  3
#define TELNET_VARTYPE_STRING 4
#define TELNET_VARTYPE_DOUBLE 5
#define TELNET_VARTYPE_INT8   6
#define TELNET_VARTYPE_UINT   7
typedef struct variabledef {
    char varname[TELNET_CMD_MAXSIZE];
    char vartype;
    void *varvalptr;
} telnetshell_vardef_t;



/*----------------------------------------------------------------------------*/
/* internal structure definitions                                             */
/* cmdparser_t is used to store all modules which have been added to the telnet server.  */
/* Each time the add_telnetcmd function is used, the internal array cmdparser_t[] of the */
/* telnet server is populated with the new commands and variables                   */
typedef struct cmdparser {
    char module[TELNET_CMD_MAXSIZE];   // module name = first token of the telnet shell command
    telnetshell_cmddef_t   *cmd;       // array of functions added to the shell
    telnetshell_vardef_t   *var;       // array of variables added to the shell
} cmdparser_t;

/* telnetsrv_params_t is an internal structure storing all the current parameters and */
/* global variables used by the telnet server                                        */
typedef struct {
     pthread_t telnet_pthread;       // thread id of the telnet server
     int telnetdbg;                  // debug level of the server
     int priority;                   // server running priority
     char *histfile;                 // command history
     int histsize;                   // command history length
     int new_socket;                 // socket of the client connection
     int logfilefd;                  // file id of the log file when log output is redirected to a file
     int  saved_stdout;              // file id of the previous stdout, used to be able to restore original stdout 
     cmdparser_t CmdParsers[TELNET_MAXCMD];   // array of registered modules.
     char msgbuff[TELNET_MAX_MSGLENGTH];      // internal buffer of the client_printf function which is used to print to the client terminal */
     unsigned int   listenport;           // ip port the telnet server is listening on
     unsigned int   listenaddr;           // ip address the telnet server is listening on
     unsigned int   loopcount;            // loop command param: number of loop iteration
     unsigned int   loopdelay;            // loop command param: delay in ms between 2 iterations
     unsigned int   phyprntbuff_size;     // for phy module,  dump_eNB_stats function buffer size
} telnetsrv_params_t;



typedef int(*addcmdfunc_t)(char*, telnetshell_vardef_t*, telnetshell_cmddef_t*);

typedef void(*settelnetmodule_t)(char *name, void *ptr); 

/*-------------------------------------------------------------------------------------------*/
/* 
VT escape sequence definition, for smarter display....
*/

#define ESC      "\x1b"
#define CSI      "\x1b["
#define BOLD     "\x1b[1m"
#define RED      "\x1b[31m"
#define GREEN    "\x1b[32m"
#define BLUE     "\x1b[34m"
#define MAGENTA  "\x1b[35m"
#define CYAN     "\x1b[36m"
#define STDFMT   "\x1b[0m"

/*---------------------------------------------------------------------------------------------*/
#define TELNET_ADDCMD_FNAME "add_telnetcmd"
typedef int(*add_telnetcmd_func_t)(char *, telnetshell_vardef_t *, telnetshell_cmddef_t *);
#ifdef TELNETSERVERCODE
int add_telnetcmd(char *modulename, telnetshell_vardef_t *var, telnetshell_cmddef_t *cmd);
void set_sched(pthread_t tid, int pid,int priority);
void set_affinity(pthread_t tid, int pid, int coreid);
extern int get_phybsize(void); 
#endif
#endif
