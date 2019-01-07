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

/*! \file cli.c
* \brief oai main cli core
* \author Navid Nikaein
* \date 2011 - 2014
* \version 0.1
* \warning This component can be runned only in user-space
* @ingroup util

*/


#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <ctype.h>

#include "cli.h"

extern cli_config *cli_cfg;



command commands[] = {
  {0, "help",NULL,com_help, NULL,"Display this text" },
  {0, "?",NULL,com_help, NULL, "help at any point" },
  {2, "prompt",NULL,prompt, prompt_usage,"Set/get cli prompt" },
  {2, "info",NULL,info, NULL,"get software information" },
  {1, "start",NULL,start, start_usage,"Start eNB/UE instance" },
  {1, "set",NULL,set, set_usage,"set log for a give component" },
  {0, "exit",NULL,com_exit, NULL,"Exit using OpenAir CLI Interface" },
  {0, (char *)NULL,NULL, NULL,NULL, (char *)NULL }
};


static uid_t real_uid;
void priv_init(void)
{
  real_uid = getuid();
}

void priv_up(void)
{
  if (seteuid(0) != 0) {
    printf("[CLI] Error setting uid to %d\n", real_uid);
  }
}

void priv_down(void)
{
  if (seteuid(real_uid) != 0) {
    printf("[CLI] Error setting uid to %d\n", 0);
  }
}

int cli_start(void)
{

  priv_init();

  priv_down();
  putenv("REMADDR=");

  // get username
  if ((username = getlogin()) == NULL) {
    username = getenv("USER");
  }

  set_permissions_map();

  // set login info
  cli_login(username, CLI_MAX_NODES, CLI_MAX_CMDS);
  // send welcome msg to the client
  sprintf(buffer, "%s", WELCOME_MSG);
  send(cli_cfg->cfd, buffer, strlen(buffer), 0);

  printf(WELCOME_MSG);
  printf("[CLI] started...\n");
  sprintf(cli_cfg->prompt,"%s%c ", cli_prompt(),  cli_cfg->promptchar);
  send(cli_cfg->cfd,cli_cfg->prompt, strlen(cli_cfg->prompt), 0);
  return 1;
}


int cli_loop(char * msg)
{

  char *s;

  /* Remove leading and trailing whitespace from the line.
     Then, if there is anything left, add it to the history list
     and execute it. */
  s = stripwhite(msg);

  if (*s) {
    //add_history(s);
    execute_line(s);
  }

  send(cli_cfg->cfd, cli_cfg->prompt, strlen(cli_cfg->prompt), 0);

  return 0;
}


/**
 * The permissions map is a bit map which defines which modes are available
 * to a user. The permissions groups map to linux groups in the following
 * way: subtract 1000. The remainder is the bit number.
 * Thus unix groups 1000, 1001, 1002 etc represent the permissions groups
 * default, config and admin respectively.
 *
 */
int set_permissions_map(void)
{
  gid_t group[10];
  int n, i;
  int perm_num;
  // get the number of supplementary group IDs
  n = getgroups(10, group);

  // give show permissions even if user has none
  map_permissions = ADMIN_PERMISSION;

  for (i=0; i<n; i++) {
    perm_num = group[i] - 1000;

    if (perm_num < 0 || perm_num > 31) {
      continue;
    }

    //map_permissions |= (1 << perm_num);
    map_permissions |= ((1 << (perm_num+1)) - 1);
  }

  printf("[CLI] Permissions %04x for this session\n", map_permissions);
  return 0;
}


int cli_login(const char *username, int maxnodes, int maxcmds)
{

  cli_set_prompt_char();

  printf("[CLI] Client initialised\n");

  // log the connection
  // telnets use REMADDR
  host = getenv("REMADDR");

  if (host) {
    snprintf(g_sid, MAX_SID, "%s#%s", username, host);
    printf("[CLI] login via telnet by user %s from address %s\n", username, host);
  } else {
    host = getenv("SSH_CLIENT");

    if (host) {
      char *cp;
      // format is <rem_addr> <rem_port> <local_port>
      // replace first space with ':', second terminates the string
      cp = strchr(host, ' ');

      if (cp) {
        *cp = ':';
      }

      cp = strchr(host, ' ');

      if (cp) {
        *cp = '\0';
      }

      snprintf(g_sid, MAX_SID, "%s$%s", username, host);
      printf("[CLI] login via SSH by user %s from address %s", username, host);
    } else {
      host = getenv("HOST");

      if (host) {
        snprintf(g_sid, MAX_SID, "%s$%s", username, host);
        printf("[CLI] by user %s from host %s", username, host);
      } else {
        printf("[CLI]Login by user %s from unknown destination\n", username);
      }
    }

  }

  //register an exit function
  atexit(cli_finish);
  return 0;
}

char *cli_prompt(void)
{
  static char promptstr[256];
  promptstr[0]='\0';
  snprintf(promptstr, 200,"%s@%s",username, cli_cfg->prompt);
  return promptstr;
}

int cli_set_prompt_char(void)
{
  if (map_permissions & ADMIN_PERMISSION) {
    cli_cfg->promptchar = '#';
  } else if (map_permissions & CONFIG_PERMISSION) {
    cli_cfg->promptchar = '%';
  } else if (map_permissions & SHOW_PERMISSION) {
    cli_cfg->promptchar = '$';
  }

  return 0;
}

void cli_finish(void)
{
  char *cp;

  if ((cp = strchr(g_sid, '#'))) {
    *cp = '\0';
    printf("[CLI] logout via telnet by user %s from address %s\n", g_sid, cp+1);
  } else if ((cp = strchr(g_sid, '$'))) {
    *cp = '\0';
    printf("[CLI] logout via SSH by user %s from address %s\n", g_sid, cp+1);
  } else {
    printf("[CLI] logout from %s\n", g_sid);
  }

  return;
}

/* Execute a command line. */
int execute_line (char *line)
{
  register int i;
  command *command;
  char *word;

  /* Isolate the command word. */
  i = 0;

  while (line[i] && whitespace (line[i]))
    i++;

  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command(word);

  if (!command) {
    sprintf(buffer, "%s: %s \n", word, NOCOMMAND_MSG);
    send(cli_cfg->cfd, buffer, strlen(buffer), 0);
    printf("[CLI] commnad %s does not exist !\n", word);
    return (-1);
  }

  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
command * find_command (char *name)
{
  register int i;

  for (i = 0; commands[i].name; i++) {
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);
  }

  return ((command *)NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char * stripwhite (char *string)
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;

  while (t > s && whitespace (*t))
    t--;

  *++t = '\0';

  return s;
}


/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */


/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int com_help (char *arg)
{
  register int i;
  int printed = 0;

  for (i = 0; commands[i].name != NULL; i++) {
    //  if (!*arg || (strcmp (arg, commands[i].name) == 0)){
    sprintf (buffer,"%s\t\t<%s>\n", commands[i].name, commands[i].doc);
    send(cli_cfg->cfd, buffer, strlen(buffer), 0);
    printed++;
    // }
  }

  if (!printed) {
    printf("No commands match `%s'.  Possibilties are:\n", arg);

    for (i = 0; commands[i].name; i++) {
      /* Print in six columns. */
      if (printed == 6)            {
        printed = 0;
        printf("\n");
      }

      printf("%s\t", commands[i].name);
      printed++;
    }

    if (printed)
      printf("\n");
  }

  return (0);
}

int whitespace (char c)
{

  return (isspace((int)c));
}
