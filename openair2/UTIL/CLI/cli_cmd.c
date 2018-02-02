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

/*! \file cli_cmd.c
* \brief oai cli commands
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \warning This component can be runned only in user-space
* @ingroup util

*/


#include "cli.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "log.h"
#include "log_extern.h"

extern cli_config *cli_cfg;

#define NEXT_OPT do {optv++;optc--; } while (0)
#define CHECK_OPTC do { if(!optc) {         \
      if((optc - 1) == 1){            \
  fprintf(stderr, "[CLI]Error parsing command line options!\n");  \
      } else {                \
  optv--;               \
  fprintf(stderr, "[CLI]You must provide a parameter when using the %s switch!\n", *optv); \
      }                 \
    } } while (0)


mapping log_comp[] = {
  {"phy", 2},
  {"mac", 3},
  {"emu", 4},
  {"ocg", 5},
  {"omg", 6},
  {NULL, -1}
};
mapping log_level[] = {
  {"error", 3},
  {"warn", 4},
  {"info", 6},
  {"debug", 7},
  {NULL, -1}
};

mapping log_flag[] = {
  {"none", 0x0},
  {"low", 0x04},
  {"med", 0x34},
  {"full", 0x74},
  {NULL, -1}
};

/* ************************ CLI COMMANDS ************************** */


/* The user wishes to quit using this program.  Just set DONE non-zero. */
int com_exit (char *arg)
{
  //set the xit value for the threads
  cli_cfg->exit = 1;
  cli_cfg->promptchar='\0';
  cli_cfg->prompt[0]='\0';
  snprintf(cli_cfg->prompt,200,"%s","oaicli");
  close(cli_cfg->cfd);
  return (0);
}

/* CLI error Function . */
void cli_error (char *caller)
{
  fprintf (stderr,"[CLI]an error running occured during the execution of %s function\n",
           caller);
}

int prompt (char *arg)
{
  // char buf[200];

  if (cli_help("prompt", arg) == OK)
    return 0;

  if (!arg || !*arg) { // no arg is provided, run get func
    snprintf(buffer,256,"%s\n", cli_cfg->prompt);
    send(cli_cfg->cfd, buffer, strlen(buffer), 0);
    // send(cli_cfg->cfd, cli_cfg->prompt, strlen(cli_cfg->prompt), 0);
  } else {// set func
    snprintf(cli_cfg->prompt,200,"%s",arg);
    sprintf( cli_cfg->prompt,"%s%c ", cli_prompt(),  cli_cfg->promptchar);

  }

  return (0);
}

int info (char *arg)
{

  sprintf(buffer, "CLI by user %s from host %s\n", username, host);
  send(cli_cfg->cfd,buffer, strlen(buffer),0);
  return 0;
}


int start (char *arg)
{
  // char buf[200];
  char* optv[20];
  int optc=0;


  if (cli_help("start", arg) == OK)
    return OK;

  if (valid_argument ("start", arg) == ERR) {
    start_usage("");
    return ERR;
  }

  optc = token_argument(arg, optv);
  process_argument(optc, optv);

  return (0);
}

int set(char *arg)
{
  char* optv[20];
  int  optc=0;

  if (cli_help("set", arg) == OK)
    return OK;

  if (valid_argument ("set", arg) == ERR) {
    set_usage( NULL );
    return ERR;
  }

  optc = token_argument(arg, optv);
  process_argument(optc, optv);
  return OK;

}
/* Return non-zero if ARG is a valid argument for CALLER, else print
   an error message and return zero. */
int valid_argument (char *caller, char *arg)
{

  if (!arg || !*arg)  {
    snprintf(buffer,200,"%s: argument required\n", caller);
    send(cli_cfg->cfd, buffer, strlen(buffer), 0);
    fprintf (stderr, "%s: argument required\n", caller);
    return (ERR);
  }

  return (OK);
}

int cli_help(char *caller, char * arg)
{

  command *command;

  if ((strcmp(arg, "-h") == 0 ) || (strcmp(arg, "?") == 0 ) || (strcmp(arg, "help") == 0 ) ||
      (strcmp(arg, "-help") == 0 ) ||(strcmp(arg, "--help") == 0 )) {

    command = find_command (caller); // we are sure that the command exists so no check is done
    return ((*(command->help))(caller)); // return zero after calling help func
  }

  return ERR; // no help is requested bu the user
}

int token_argument(char *arg, char* optv[])
{

  int tokc;

  tokc = 0;
  optv[tokc] = strtok(arg, " "); // get the first token

  while (optv[tokc]!=NULL) {
    tokc++;
    optv[tokc] = strtok(NULL, " ");
  }

  return tokc;

}

int process_argument(int optc, char* optv[])
{

  int index;
  int state;
  int comp=0, level=0, flag=0x34, interval=0;

  while (optc > 0) {

    /* add an RB */
    if ((strcmp(*optv, "ue") == 0) || (strcmp(*optv, "UE") == 0) ) {

      NEXT_OPT;
      CHECK_OPTC;
      index = atoi (*optv);

      if (optc > 0) {
        NEXT_OPT;
        CHECK_OPTC;
        state = atoi (*optv);
        printf("[CLI] UE %d state %d\n", index, state);
        oai_emulation.info.cli_start_ue[index]=state;
      } else
        return ERR;
    }

    if ((strcmp(*optv, "enb") == 0) || (strcmp(*optv, "eNB") == 0) ) {

      NEXT_OPT;
      CHECK_OPTC;
      index = atoi (*optv);

      if (optc > 0) {
        NEXT_OPT;
        CHECK_OPTC;
        state = atoi (*optv);
        printf("[CLI] eNB %d state %d\n", index, state);
        oai_emulation.info.cli_start_enb[index]= state;
      } else
        return ERR;
    }

    // fixme
    if ((strcmp(*optv, "log") == 0) || (strcmp(*optv, "LOG") == 0) ) {

      NEXT_OPT;
      CHECK_OPTC;
      comp = map_str_to_int(log_comp, *optv);
      NEXT_OPT;

      if (optc > 0) {
        CHECK_OPTC;
        level = map_str_to_int(log_level, *optv);
      } else
        return ERR;

      NEXT_OPT;

      if (optc > 0) {
        CHECK_OPTC;
        flag = map_str_to_int (log_flag, *optv);
      }

      NEXT_OPT;

      if (optc > 0) {
        CHECK_OPTC;
        interval = atoi (*optv);
      }

      printf("[CLI] OMG set log for comp %d level %d flag 0x%x interval %d\n",
             comp, level, flag, interval);
      set_comp_log(comp, level, flag, interval);
    }

    NEXT_OPT;
  }

  return OK;
}


int prompt_usage(char *arg)
{
  sprintf(buffer, "Usage: prompt [value]\n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);

  return 0;
}
int start_usage(char *arg)
{
  sprintf(buffer, "Usage: start [enb|UE] [enb_index][0: stop, 1: start] \n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Example to start enb 0: start enb 0 1 \n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Example to stop ue &: start ue 1 0 \n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);

  return 0;
}

int set_usage( char *arg )
{
  (void)(arg); // unused
  sprintf(buffer, "Usage: set log [comp] [level:debug,info,warn,error][flag:none,low,med,full][intervale: 1-100] \n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Example 1: set log omg debug\n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Example 1: set log omg info med 10\n");
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);

  return 0;
}
