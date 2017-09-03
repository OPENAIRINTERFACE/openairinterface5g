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

/*! \file common/config/config_load_configmodule.c
 * \brief configuration module, load the shared library implementing the configuration module 
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
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#define CONFIG_LOADCONFIG_MAIN
#include "config_load_configmodule.h"
#include "config_userapi.h"
#define CONFIG_SHAREDLIBFORMAT "libparams_%s.so"

int load_config_sharedlib(char *cfgmode, char *cfgP[], int numP, configmodule_interface_t *cfgptr)
{
void *lib_handle;
char fname[128];
char libname[FILENAME_MAX]; 
int st;

   st=0;  
   sprintf(libname,CONFIG_SHAREDLIBFORMAT,cfgmode);

   lib_handle = dlopen(libname,RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
   if (!lib_handle) {
      fprintf(stderr,"[CONFIG] %s %d Error calling dlopen(%s): %s\n",__FILE__, __LINE__, libname,dlerror());
      st = -1;
   } else { 
      sprintf (fname,"config_%s_init",cfgmode);
      cfgptr->init = dlsym(lib_handle,fname);

      if (cfgptr->init == NULL ) {
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgmode);
      } else {
         st=cfgptr->init(cfgP,numP); 
         printf("[CONFIG] function %s returned %i\n",
               fname, st);	 
      }

      sprintf (fname,"config_%s_get",cfgmode);
      cfgptr->get = dlsym(lib_handle,fname);
      if (cfgptr->get == NULL ) { 
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgmode);
	 st = -1;
      }
      
      sprintf (fname,"config_%s_getlist",cfgmode);
      cfgptr->getlist = dlsym(lib_handle,fname);
      if (cfgptr->getlist == NULL ) { 
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgmode);
	 st = -1;
      }

      sprintf (fname,"config_%s_end",cfgmode);
      cfgptr->end = dlsym(lib_handle,fname);
      if (cfgptr->getlist == NULL ) { 
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgmode);
      }      
   } 
   
   return st;	       
}
/*-----------------------------------------------------------------------------------*/
/* from here: interface implementtion of the configuration module */


configmodule_interface_t *load_configmodule(int argc, char **argv)
{
char *cfgparam=NULL;
char *modeparams=NULL;
char *cfgmode=NULL;
char *strtokctx=NULL;
char *cfgP[CONFIG_MAX_OOPT_PARAMS];

int i; 
int p;  

 
  for(i=0; i<CONFIG_MAX_OOPT_PARAMS ; i++) {
      cfgP[i]=NULL;
  }
    
/* first parse the command line to look for the -O option */
  opterr=0;
  while ((i = getopt(argc, argv, "O:")) != -1) {
       if ( i == 'O' ) {
          cfgparam = optarg; 
       }      
   }
/* look for the OAI_CONFIGMODULE environement variable */
  if ( cfgparam == NULL ) {
     cfgparam = getenv("OAI_CONFIGMODULE");
     }

/* default */
  if (cfgparam == NULL) {
     cfgparam = "libconfig:oaisoftmodem.conf";
     }
/* parse the config parameters to set the config source */
   i = sscanf(cfgparam,"%m[^':']:%ms",&cfgmode,&modeparams);
   if (i< 0) {
       fprintf(stderr,"[CONFIG] %s, %d, sscanf error parsing config source  %s: %s\n", __FILE__, __LINE__,cfgparam, strerror(errno));
       return NULL;
   }
   else if ( i == 1 ) {
       modeparams=cfgmode;
       cfgmode=strdup("libconfig");
   }

   cfgptr = malloc(sizeof(configmodule_interface_t));

   p=0;
   cfgP[p]=strtok_r(modeparams,":",&strtokctx);     
   while ( p< CONFIG_MAX_OOPT_PARAMS && cfgP[p] != NULL) {
       char *aptr;
       aptr=strcasestr(cfgP[p],"dbgl");
       if (aptr != NULL) {
           cfgptr->rtflags = strtol(aptr+4,NULL,0);
           Config_Params[CONFIGPARAM_DEBUGFLAGS_IDX].paramflags = Config_Params[CONFIGPARAM_DEBUGFLAGS_IDX].paramflags | PARAMFLAG_DONOTREAD;
           for (int j=p; j<(CONFIG_MAX_OOPT_PARAMS-1); j++) cfgP[j] = cfgP[j+1];
           p--;
       }
       p++;
       cfgP[p] = strtok_r(NULL,":",&strtokctx);
   }

   
   printf("[CONFIG] get parameters from %s ",cfgmode);
   for (i=0;i<p; i++) {
        printf("%s ",cfgP[i]); 
   }
   printf("\n");

 
   i=load_config_sharedlib(cfgmode, cfgP,p,cfgptr);
   if (i< 0) {
      fprintf(stderr,"[CONFIG] %s %d config module %s couldn't be loaded\n", __FILE__, __LINE__,cfgmode);
      return NULL;
   } else {
      printf("[CONFIG] config module %s loaded\n",cfgmode);
   }

   Config_Params[CONFIGPARAM_DEBUGFLAGS_IDX].uptr=&(cfgptr->rtflags);
   config_get(Config_Params,CONFIG_PARAMLENGTH(Config_Params), CONFIG_SECTIONNAME );   

   if (modeparams != NULL) free(modeparams);
   if (cfgmode != NULL) free(cfgmode);
   optind=1;
   cfgptr->argc = argc;
   cfgptr->argv  = argv;    
   return cfgptr;
}

void end_configmodule()
{ 
  if (cfgptr != NULL) {
      printf ("[CONFIG] free %u pointers\n",cfgptr->numptrs);  
      for(int i=0; i<cfgptr->numptrs ; i++) {
          if (cfgptr->ptrs[i] != NULL) {
             free(cfgptr->ptrs[i]);
          }
          cfgptr->ptrs[i]=NULL;
      }
      if (cfgptr->end != NULL) {
      printf ("[CONFIG] calling config module end function...\n"); 
      cfgptr->end();
      }
  free(cfgptr);
  cfgptr=NULL;
  }
}




