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

/*! \file common/config/cmdline/config_libconfig.c
 * \brief configuration module, command line parsing implementation
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <platform_types.h>
#include "config_userapi.h"
#include "../utils/LOG/log.h"

int parse_stringlist(paramdef_t *cfgoptions, char *val) {
  char *atoken;
  char *tokctx = NULL;
  char *tmpval=strdup(val);
  int   numelt=0;
  cfgoptions->numelt=0;
  atoken=strtok_r(tmpval, ",",&tokctx);

  while(atoken != NULL) {
    numelt++ ;
    atoken=strtok_r(NULL, ",",&tokctx);
  }

  free(tmpval);
  config_check_valptr(cfgoptions,(char **)&(cfgoptions->strlistptr), sizeof(char *) * numelt);
  cfgoptions->numelt=numelt;
  atoken=strtok_r(val, ",",&tokctx);

  for( int i=0; i<cfgoptions->numelt && atoken != NULL ; i++) {
    config_check_valptr(cfgoptions,&(cfgoptions->strlistptr[i]),strlen(atoken)+1);
    sprintf(cfgoptions->strlistptr[i],"%s",atoken);
    printf_params("[LIBCONFIG] %s[%i]: %s\n", cfgoptions->optname,i,cfgoptions->strlistptr[i]);
    atoken=strtok_r(NULL, ",",&tokctx);
  }

  return (cfgoptions->numelt > 0);
}

int processoption(paramdef_t *cfgoptions, char *value) {
  char *tmpval = value;
  int optisset=0;
  char defbool[2]="1";

  if ( value == NULL) {
    if( (cfgoptions->paramflags &PARAMFLAG_BOOL) == 0 ) { /* not a boolean, argument required */
      CONFIG_PRINTF_ERROR("[CONFIG] command line, option %s requires an argument\n",cfgoptions->optname);
    } else {        /* boolean value option without argument, set value to true*/
      tmpval = defbool;
    }
  }

  switch(cfgoptions->type) {
      char *charptr;

    case TYPE_STRING:
      if (cfgoptions->numelt == 0 ) {
        config_check_valptr(cfgoptions, cfgoptions->strptr, strlen(tmpval)+1);
        sprintf(*(cfgoptions->strptr), "%s",tmpval);
      } else {
        sprintf( (char *)(cfgoptions->strptr), "%s",tmpval);
      }

      printf_cmdl("[CONFIG] %s set to  %s from command line\n", cfgoptions->optname, tmpval);
      optisset=1;
      break;

    case TYPE_STRINGLIST:
      optisset=parse_stringlist(cfgoptions,tmpval);
      break;

    case TYPE_UINT32:
    case TYPE_INT32:
    case TYPE_UINT16:
    case TYPE_INT16:
    case TYPE_UINT8:
    case TYPE_INT8:
      config_check_valptr(cfgoptions, (char **)&(cfgoptions->iptr),sizeof(int32_t));
      config_assign_int(cfgoptions,cfgoptions->optname,(int32_t)strtol(tmpval,&charptr,0));

      if( *charptr != 0) {
        CONFIG_PRINTF_ERROR("[CONFIG] command line, option %s requires an integer argument\n",cfgoptions->optname);
      }

      optisset=1;
      break;

    case TYPE_UINT64:
    case TYPE_INT64:
      config_check_valptr(cfgoptions, (char **)&(cfgoptions->i64ptr),sizeof(uint64_t));
      *(cfgoptions->i64ptr)=strtoll(tmpval,&charptr,0);

      if( *charptr != 0) {
        CONFIG_PRINTF_ERROR("[CONFIG] command line, option %s requires an integer argument\n",cfgoptions->optname);
      }

      printf_cmdl("[CONFIG] %s set to  %lli from command line\n", cfgoptions->optname, (long long)*(cfgoptions->i64ptr));
      optisset=1;
      break;

    case TYPE_UINTARRAY:
    case TYPE_INTARRAY:
      break;

    case TYPE_DOUBLE:
      config_check_valptr(cfgoptions, (char **)&(cfgoptions->dblptr),sizeof(double));
      *(cfgoptions->dblptr) = strtof(tmpval,&charptr);

      if( *charptr != 0) {
        CONFIG_PRINTF_ERROR("[CONFIG] command line, option %s requires a double argument\n",cfgoptions->optname);
      }

      printf_cmdl("[CONFIG] %s set to  %lf from command line\n", cfgoptions->optname, *(cfgoptions->dblptr));
      optisset=1;
      break;

    case TYPE_IPV4ADDR:
      break;

    default:
      CONFIG_PRINTF_ERROR("[CONFIG] command line, %s type %i  not supported\n",cfgoptions->optname, cfgoptions->type);
      break;
  } /* switch on param type */

  if (optisset == 1) {
    cfgoptions->paramflags = cfgoptions->paramflags |  PARAMFLAG_PARAMSET;
  }

  return optisset;
}

/*--------------------------------------------------------------------*/
/*  check unknown options in the command line
*/
int config_check_unknown_cmdlineopt(char *prefix) {
  int unknowndetected=0;
  char testprefix[CONFIG_MAXOPTLENGTH];
  int finalcheck = 0;
  memset(testprefix,0,sizeof(testprefix));
  memset(testprefix,0,sizeof(testprefix));

  if (prefix != NULL) {
    if (strcmp(prefix,CONFIG_CHECKALLSECTIONS) == 0)
      finalcheck = 1;
    else if (strlen(prefix) > 0) {
      sprintf(testprefix,"--%.*s.",CONFIG_MAXOPTLENGTH-4,prefix);
    }
  }

  for (int i=1; i<config_get_if()->argc ; i++) {
    if ( !finalcheck && strstr(config_get_if()->argv[i],testprefix) == NULL ) continue;

    if ( !finalcheck && testprefix[0]==0 && index(config_get_if()->argv[i],'.') != NULL) continue;

    if ( !finalcheck && isdigit(config_get_if()->argv[i][0])) continue;

    if ( !finalcheck && config_get_if()->argv[i][0] == '-' && isdigit(config_get_if()->argv[i][1])) continue;

    if ( (config_get_if()->argv_info[i] & CONFIG_CMDLINEOPT_PROCESSED) == 0 ) {
      fprintf(stderr,"[CONFIG] unknown option: %s\n",
              config_get_if()->argv[i] );
      unknowndetected++;
    }
  }

  if (unknowndetected > 0) {
    CONFIG_PRINTF_ERROR("[CONFIG] %i unknown option(s) in command line starting with %s (section %s)\n",
                        unknowndetected,testprefix,((prefix==NULL)?"":prefix));
  }

  return unknowndetected;
}  /* config_check_unknown_cmdlineopt */

int config_process_cmdline(paramdef_t *cfgoptions,int numoptions, char *prefix) {
  int c = config_get_if()->argc;
  int i,j;
  char *pp;
  char cfgpath[CONFIG_MAXOPTLENGTH];
  j = 0;
  i = 0;

  while (c > 0 ) {
    char *oneargv = strdup(config_get_if()->argv[i]);          /* we use strtok_r which modifies its string paramater, and we don't want argv to be modified */

    /* first check help options, either --help, -h or --help_<section> */
    if (strncmp(oneargv, "-h",2) == 0 || strncmp(oneargv, "--help",6) == 0 ) {
      char *tokctx = NULL;
      pp=strtok_r(oneargv, "_",&tokctx);
      config_get_if()->argv_info[i] |= CONFIG_CMDLINEOPT_PROCESSED;

      if (pp == NULL || strcasecmp(pp,config_get_if()->argv[i] ) == 0 ) {
        if( prefix == NULL) {
          config_printhelp(cfgoptions,numoptions,"(root section)");

          if ( ! ( CONFIG_ISFLAGSET(CONFIG_NOEXITONHELP)))
            exit_fun("[CONFIG] Exiting after displaying help\n");
        }
      } else {
        pp=strtok_r(NULL, " ",&tokctx);

        if ( prefix != NULL && pp != NULL && strncasecmp(prefix,pp,strlen(pp)) == 0 ) {
          config_printhelp(cfgoptions,numoptions,prefix);

          if ( ! (CONFIG_ISFLAGSET(CONFIG_NOEXITONHELP))) {
            fprintf(stderr,"[CONFIG] %s %i section %s:", __FILE__, __LINE__, prefix);
            exit_fun(" Exiting after displaying help\n");
          }
        }
      }
    }

    /* now, check for non help options */
    if (oneargv[0] == '-') {
      for(int n=0; n<numoptions; n++) {
        if ( ( cfgoptions[n].paramflags & PARAMFLAG_DISABLECMDLINE) != 0) {
          continue;
        }

        if (prefix != NULL) {
          sprintf(cfgpath,"%s.%s",prefix,cfgoptions[n].optname);
        } else {
          sprintf(cfgpath,"%s",cfgoptions[n].optname);
        }

        if ( ((strlen(oneargv) == 2) && (strcmp(oneargv + 1,cfgpath) == 0))  || /* short option, one "-" */
             ((strlen(oneargv) > 2) && (strcmp(oneargv + 2,cfgpath ) == 0 )) || /* long option beginning with "--" */
             ((strlen(oneargv) == 2) && (strcmp(oneargv + 1,cfgoptions[n].optname) == 0) && (cfgoptions[n].paramflags & PARAMFLAG_CMDLINE_NOPREFIXENABLED )) ||
             ((strlen(oneargv) > 2) && (strcmp(oneargv + 2,cfgpath ) == 0 ) && (cfgoptions[n].paramflags & PARAMFLAG_CMDLINE_NOPREFIXENABLED )) ) {
          char *valptr=NULL;
          int ret;
          config_get_if()->argv_info[i] |= CONFIG_CMDLINEOPT_PROCESSED;

          if (c > 0) {
            pp = config_get_if()->argv[i+1];

            if (pp != NULL ) {
              ret = strlen(pp);

              if (ret > 0 ) {
                if (pp[0] != '-')
                  valptr=pp;
                else if ( ret > 1 && pp[0] == '-' && isdigit(pp[1]) )
                  valptr=pp;
              }
            }
          }

          j += processoption(&(cfgoptions[n]), valptr);

          if (  valptr != NULL ) {
            i++;
            config_get_if()->argv_info[i] |= CONFIG_CMDLINEOPT_PROCESSED;
            c--;
          }

          break;
        }
      } /* for n... */
    } /* if (oneargv[0] == '-') */

    free(oneargv);
    i++;
    c--;
  }   /* fin du while */

  printf_cmdl("[CONFIG] %s %i options set from command line\n",((prefix == NULL) ? "(root)":prefix),j);
  return j;
}  /* parse_cmdline*/
