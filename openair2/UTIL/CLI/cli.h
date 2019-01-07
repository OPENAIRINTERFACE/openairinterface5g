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

/*! \file cli.h
* \brief cli interface header file
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \warning This component can be runned only in the user-space
* @ingroup util
*/


#ifndef __CLI_H__
#define __CLI_H__

#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

//#include <readline/readline.h>
//#include <readline/history.h>

typedef struct {

  int port;
  int sfd; //server fd
  int cfd; // client fd
  int enabled;
  int debugfd; // debug state
  int debug_paused;
  char promptchar;
  char prompt[200];
  //  char user_name[200];
  int exit;
  /*end*/

} cli_config;


typedef struct {
  int group;
  char *name;             /* User printable name of the function. */
  void *data;
  int (*func)(char*);
  int (*help)(char*);
  char *doc;      /* Documentation for this function.  */
} command;


typedef void (* cli_handler_t)(const void * data, socklen_t len);


#define MAX_SOCK_BUFFER_SIZE 1500
char buffer[MAX_SOCK_BUFFER_SIZE];

/* The name of this program, as taken from argv[0]. */
char *progname;
/*username*/
const char *username;
const char *host;
/* When non-zero, this global means the user is done using this program. */
int done;



/// global permissions table
unsigned int map_permissions;

sigjmp_buf openair_jmp_buf;

/// maximum session ID length
#define MAX_SID 100
/// session id - sent in every message
char g_sid[MAX_SID];

#define WELCOME_MSG "\n\nWelcome to the Eurecom OpenAirInterface CLI \n\n"

#define NOCOMMAND_MSG "No such command for openair CLI Interface"


#define SHOW_PERMISSION     (1 << 0)

#define CONFIG_PERMISSION   (1 << 1)

#define ADMIN_PERMISSION    (1 << 2)

#define OK 0
#define ERR -1

#define CLI_MAX_CMDS 200
#define CLI_MAX_NODES 10

/* The names of functions that actually do the manipulation. */
int com_help (char * arg);
int com_exit (char *arg);

int prompt(char *arg);
int prompt_usage(char *);
int info(char *arg);

int start(char *arg), set(char *arg);
int start_usage(char *), set_usage(char *);

/* Forward declarations. */
char *stripwhite (char *string);
command *find_command (char* name);
void abandon_input(int);
//char *command_generator (void);
//char **fileman_completion (void);
int cli_login(const char *, int, int );
int cli_loop(char* msg);
int cli_set_prompt_char(void);
char *cli_prompt(void);
int openair_cli(void);
int valid_argument (char *caller, char *arg);
int cli_help(char *caller, char * arg);
void cli_init(void);
int cli_start(void);
void set_comp_debug(int);
int set_permissions_map(void);
int is_debugging(void);
int execute_line(char* line);
int whitespace (char  c);
void cli_finish(void);
int token_argument(char *arg, char* optv[]);
int process_argument(int optc, char* optv[]);
#include "cli_if.h"

#endif
