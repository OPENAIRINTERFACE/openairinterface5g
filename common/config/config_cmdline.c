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
#include "config_userapi.h"


void parse_stringlist(paramdef_t *cfgoptions, char *val)
{
char *atoken;
char *tokctx;
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
   cfgoptions->numelt=numelt; 
}
 
int processoption(paramdef_t *cfgoptions, char *value)
{
char *tmpval = value;
int optisset=0;
char defbool[2]="1";

     
     if ( value == NULL) {
        if( (cfgoptions->paramflags &PARAMFLAG_BOOL) == 0 ) { /* not a boolean, argument required */
	    fprintf(stderr,"[CONFIG] command line, option %s requires an argument\n",cfgoptions->optname);
	    return 0;
        } else {        /* boolean value option without argument, set value to true*/
            tmpval = defbool;
        }
     }
     switch(cfgoptions->type)
       {
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
           parse_stringlist(cfgoptions,tmpval); 
        break;
        case TYPE_UINT32:
       	case TYPE_INT32:
        case TYPE_UINT16:
       	case TYPE_INT16:
	case TYPE_UINT8:
       	case TYPE_INT8:	
           config_check_valptr(cfgoptions, (char **)&(cfgoptions->iptr),sizeof(int32_t));
	   config_assign_int(cfgoptions,cfgoptions->optname,(int32_t)strtol(tmpval,NULL,0));  
	   optisset=1;
        break;  	
       	case TYPE_UINT64:
       	case TYPE_INT64:
           config_check_valptr(cfgoptions, (char **)&(cfgoptions->i64ptr),sizeof(uint64_t));
	   *(cfgoptions->i64ptr)=strtoll(tmpval,NULL,0);  
           printf_cmdl("[CONFIG] %s set to  %lli from command line\n", cfgoptions->optname, (long long)*(cfgoptions->i64ptr));
	   optisset=1;
        break;        
       	case TYPE_UINTARRAY:
       	case TYPE_INTARRAY:

        break;
        case TYPE_DOUBLE:
           config_check_valptr(cfgoptions, (char **)&(cfgoptions->dblptr),sizeof(double)); 
           *(cfgoptions->dblptr) = strtof(tmpval,NULL);  
           printf_cmdl("[CONFIG] %s set to  %lf from command line\n", cfgoptions->optname, *(cfgoptions->dblptr));
	   optisset=1; 
        break; 

       	case TYPE_IPV4ADDR:

        break;

       default:
            fprintf(stderr,"[CONFIG] command line, %s type %i  not supported\n",cfgoptions->optname, cfgoptions->type);
       break;
       } /* switch on param type */
       if (optisset == 1) {
          cfgoptions->paramflags = cfgoptions->paramflags |  PARAMFLAG_PARAMSET;
       }
    return optisset;
}

int config_process_cmdline(paramdef_t *cfgoptions,int numoptions, char *prefix)
{


int c = config_get_if()->argc;
int i,j;
char *pp;
char *cfgpath; 
 
  j = (prefix ==NULL) ? 0 : strlen(prefix); 
  cfgpath = malloc( j + MAX_OPTNAME_SIZE +1);
  if (cfgpath == NULL) {
     fprintf(stderr,"[CONFIG] %s %i malloc error,  %s\n", __FILE__, __LINE__,strerror(errno));
     return -1;
  }

  j = 0;
  i = 0;
    while (c > 0 ) {
        char *oneargv = strdup(config_get_if()->argv[i]);          /* we use strtok_r which modifies its string paramater, and we don't want argv to be modified */
/* first check help options, either --help, -h or --help_<section> */
        if (strncmp(oneargv, "-h",2) == 0 || strncmp(oneargv, "--help",6) == 0 ) {
            char *tokctx;
            pp=strtok_r(oneargv, "_",&tokctx);
            if (pp == NULL || strcasecmp(pp,config_get_if()->argv[i] ) == 0 ) {
                if( prefix == NULL) {
                  config_printhelp(cfgoptions,numoptions);
                  if ( ! ( CONFIG_ISFLAGSET(CONFIG_NOEXITONHELP)))
                     exit_fun("[CONFIG] Exiting after displaying help\n");
                }
            } else {
                pp=strtok_r(NULL, " ",&tokctx);
                if ( prefix != NULL && pp != NULL && strncasecmp(prefix,pp,strlen(pp)) == 0 ) { 
                   printf ("Help for %s section:\n",prefix);               
                   config_printhelp(cfgoptions,numoptions);
                   if ( ! (CONFIG_ISFLAGSET(CONFIG_NOEXITONHELP))) {
                      fprintf(stderr,"[CONFIG] %s %i section %s:", __FILE__, __LINE__, prefix);
                      exit_fun(" Exiting after displaying help\n");
                   }
                }
            } 
        }

/* now, check for non help options */
        if (oneargv[0] == '-') {        
    	    for(int n=0;n<numoptions;n++) {
    		if ( ( cfgoptions[n].paramflags & PARAMFLAG_DISABLECMDLINE) != 0) {
    		  continue;
    		 }
    		if (prefix != NULL) {
    		   sprintf(cfgpath,"%s.%s",prefix,cfgoptions[n].optname);
    		} else {
    		   sprintf(cfgpath,"%s",cfgoptions[n].optname);
    		}
    		if ( ((strlen(oneargv) == 2) && (strcmp(oneargv + 1,cfgpath) == 0))  || /* short option, one "-" */
    		     ((strlen(oneargv) > 2) && (strcmp(oneargv + 2,cfgpath ) == 0 )) ) {
                   char *valptr=NULL;
                   int ret;
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
  free(cfgpath);
  return j;            
}  /* parse_cmdline*/



