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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "config_userapi.h"

int processoption(paramdef_t *cfgoptions, char *value)
{
int ret = 0;

     switch(cfgoptions->type)
       {
       	case TYPE_STRING:
           check_valptr(cfgoptions, (char **)(cfgoptions->strptr), sizeof(char *));
           check_valptr(cfgoptions, cfgoptions->strptr, strlen(value+1));
           sprintf(*(cfgoptions->strptr), "%s",value);
           printf_cmdl("[LIBCONFIG] %s set to  %s from command line\n", cfgoptions->optname, value);
           ret++;
       break;
       case TYPE_STRINGLIST:

       break;
	
       	case TYPE_UINT:
       	case TYPE_INT:
       	case TYPE_UINT64:
       	case TYPE_INT64:
           check_valptr(cfgoptions, (char **)&(cfgoptions->i64ptr),sizeof(uint64_t)); 
           *(cfgoptions->uptr) =strtol(value,NULL,0);  
           printf_cmdl("[LIBCONFIG] %s set to  %lli from command line\n", cfgoptions->optname, (long long)*(cfgoptions->i64ptr));
           ret++; 
        break;        
       	case TYPE_UINTARRAY:
       	case TYPE_INTARRAY:

        break;
       	case TYPE_IPV4ADDR:

        break;

       default:
            fprintf(stderr,"[LIBCONFIG] command line, %s type %i  not supported\n",cfgoptions->optname, cfgoptions->type);
       break;
       } /* switch on param type */
    return ret;
}

int config_process_cmdline(paramdef_t *cfgoptions,int numoptions, char *prefix)
{
char **p = config_get_if()->argv;
int c = config_get_if()->argc;
int j;
char *cfgpath; 
 
  j = (prefix ==NULL) ? 0 : strlen(prefix); 
  cfgpath = malloc( j + MAX_OPTNAME_SIZE +1);
  if (cfgpath == NULL) {
     fprintf(stderr,"[CONFIG] %s %i malloc error,  %s\n", __FILE__, __LINE__,strerror(errno));
     return -1;
  }

  j=0;
  p++;
  c--;
    while (c >= 0 && *p != NULL) {
        if (strcmp(*p, "-h") == 0 || strcmp(*p, "--help") == 0 ) {
            config_printhelp(cfgoptions,numoptions);
        }
        for(int i=0;i<numoptions;i++) {
            if ( ( cfgoptions[i].paramflags & PARAMFLAG_DISABLECMDLINE) != 0) {
              continue;
             }
            sprintf(cfgpath,"%s.%s",prefix,cfgoptions[i].optname);
            if ( strcmp(*p + 1,cfgoptions[i].shortopt) == 0  || 
                 strcmp(*p + 2,cfgpath ) == 0 ) {
               p++;
               j =+ processoption(&(cfgoptions[i]), *p);
               c--;
            }
         }   	     
   	 p++;
         c--;  
    }   /* fin du while */
  printf_cmdl("[CONFIG] %s %i options set from command line\n",((prefix == NULL) ? "":prefix),j);
  free(cfgpath);
  return j;            
}  /* parse_cmdline*/



