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

int load_config_sharedlib(configmodule_interface_t *cfgptr)
{
void *lib_handle;
char fname[128];
char libname[FILENAME_MAX]; 
int st;

   st=0;  
   sprintf(libname,CONFIG_SHAREDLIBFORMAT,cfgptr->cfgmode);

   lib_handle = dlopen(libname,RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
   if (!lib_handle) {
      fprintf(stderr,"[CONFIG] %s %d Error calling dlopen(%s): %s\n",__FILE__, __LINE__, libname,dlerror());
      st = -1;
   } else { 
      sprintf (fname,"config_%s_init",cfgptr->cfgmode);
      cfgptr->init = dlsym(lib_handle,fname);

      if (cfgptr->init == NULL ) {
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgptr->cfgmode);
      } else {
         st=cfgptr->init(cfgptr->cfgP,cfgptr->num_cfgP); 
         printf("[CONFIG] function %s returned %i\n",
               fname, st);	 
      }

      sprintf (fname,"config_%s_get",cfgptr->cfgmode);
      cfgptr->get = dlsym(lib_handle,fname);
      if (cfgptr->get == NULL ) { 
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgptr->cfgmode);
	 st = -1;
      }
      
      sprintf (fname,"config_%s_getlist",cfgptr->cfgmode);
      cfgptr->getlist = dlsym(lib_handle,fname);
      if (cfgptr->getlist == NULL ) { 
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgptr->cfgmode);
	 st = -1;
      }

      sprintf (fname,"config_%s_end",cfgptr->cfgmode);
      cfgptr->end = dlsym(lib_handle,fname);
      if (cfgptr->getlist == NULL ) { 
         printf("[CONFIG] %s %d no function %s for config mode %s\n",
               __FILE__, __LINE__,fname, cfgptr->cfgmode);
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
char *atoken;
uint32_t tmpflags=0;
int i;
 
/* first parse the command line to look for the -O option */
  opterr=0;
  for (i = 0;i<argc;i++) {
       if (strlen(argv[i]) < 2) continue;
       if ( argv[i][1] == 'O' && i < (argc -1)) {
          cfgparam = argv[i+1]; 
       } 
        if ( argv[i][1] == 'h' ) {
          tmpflags = CONFIG_HELP; 
       }            
   }
   optind=1;

/* look for the OAI_CONFIGMODULE environement variable */
  if ( cfgparam == NULL ) {
     cfgparam = getenv("OAI_CONFIGMODULE");
     }

/* default */
  if (cfgparam == NULL) {
     tmpflags = tmpflags | CONFIG_NOOOPT;
     cfgparam = DEFAULT_CFGMODE ":" DEFAULT_CFGFILENAME;
     }
/* parse the config parameters to set the config source */
   i = sscanf(cfgparam,"%m[^':']:%ms",&cfgmode,&modeparams);
   if (i< 0) {
       fprintf(stderr,"[CONFIG] %s, %d, sscanf error parsing config source  %s: %s\n", __FILE__, __LINE__,cfgparam, strerror(errno));
       cfgmode=strdup(DEFAULT_CFGMODE);
       modeparams = strdup(DEFAULT_CFGFILENAME);
   }
   else if ( i == 1 ) {
  /* -O argument doesn't contain ":" separator, assume -O <conf file> option, default cfgmode to libconfig
     with one parameter, the path to the configuration file */
       modeparams=cfgmode;
       cfgmode=strdup(DEFAULT_CFGMODE);
   }

   cfgptr = malloc(sizeof(configmodule_interface_t));
   memset(cfgptr,0,sizeof(configmodule_interface_t));

   cfgptr->rtflags = cfgptr->rtflags | tmpflags;
   cfgptr->argc   = argc;
   cfgptr->argv   = argv; 
   cfgptr->cfgmode=strdup(cfgmode);

   cfgptr->num_cfgP=0;
   atoken=strtok_r(modeparams,":",&strtokctx);     
   while ( cfgptr->num_cfgP< CONFIG_MAX_OOPT_PARAMS && atoken != NULL) {
       /* look for debug level in the config parameters, it is commom to all config mode 
          and will be removed frome the parameter array passed to the shared module */
       char *aptr;
       aptr=strcasestr(atoken,"dbgl");
       if (aptr != NULL) {
           cfgptr->rtflags = cfgptr->rtflags | strtol(aptr+4,NULL,0);

       } else {
           cfgptr->cfgP[cfgptr->num_cfgP] = strdup(atoken);
           cfgptr->num_cfgP++;
       }
       atoken = strtok_r(NULL,":",&strtokctx);
   }

   
   printf("[CONFIG] get parameters from %s ",cfgmode);
   for (i=0;i<cfgptr->num_cfgP; i++) {
        printf("%s ",cfgptr->cfgP[i]); 
   }
   printf("\n");

 
   i=load_config_sharedlib(cfgptr);
   if (i< 0) {
      fprintf(stderr,"[CONFIG] %s %d config module %s couldn't be loaded\n", __FILE__, __LINE__,cfgmode);
      cfgptr->rtflags = cfgptr->rtflags | CONFIG_HELP | CONFIG_ABORT;
   } else {
      printf("[CONFIG] config module %s loaded\n",cfgmode);
      Config_Params[CONFIGPARAM_DEBUGFLAGS_IDX].uptr=&(cfgptr->rtflags);
      config_get(Config_Params,CONFIG_PARAMLENGTH(Config_Params), CONFIG_SECTIONNAME ); 
   }

  

   if (modeparams != NULL) free(modeparams);
   if (cfgmode != NULL) free(cfgmode);
   if (CONFIG_ISFLAGSET(CONFIG_ABORT)) config_printhelp(Config_Params,CONFIG_PARAMLENGTH(Config_Params));
   return cfgptr;
}


/* free memory allocated when reading parameters */
/* config module could be initialized again after this call */
void end_configmodule(void)
{ 
  if (cfgptr != NULL) {
      if (cfgptr->end != NULL) {
         printf ("[CONFIG] calling config module end function...\n"); 
         cfgptr->end();
      }

      printf ("[CONFIG] free %u config value pointers\n",cfgptr->numptrs);  
      for(int i=0; i<cfgptr->numptrs ; i++) {
          if (cfgptr->ptrs[i] != NULL) {
             free(cfgptr->ptrs[i]);
             cfgptr->ptrs[i]=NULL;
          }
      }
      cfgptr->numptrs=0;
  }
}

/* free all memory used by config module */
/* should be called only at program exit */
void free_configmodule(void)
{ 
  if (cfgptr != NULL) {
      end_configmodule();
      if( cfgptr->cfgmode != NULL) free(cfgptr->cfgmode);
      printf ("[CONFIG] free %u config parameter pointers\n",cfgptr->num_cfgP);  
      for (int i=0; i<cfgptr->num_cfgP; i++) {
          if ( cfgptr->cfgP[i] != NULL) free(cfgptr->cfgP[i]);
          }


  free(cfgptr);
  cfgptr=NULL;
  }
}




