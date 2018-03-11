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

/*! \file common/utils/load_module_shlib.c
 * \brief shared library loader implementation
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include "openair1/PHY/defs.h"
#define LOAD_MODULE_SHLIB_MAIN

#include "common/config/config_userapi.h"
#include "load_module_shlib.h"
void loader_init(void) {
  paramdef_t LoaderParams[] = LOADER_PARAMS_DESC;

  loader_data.mainexec_buildversion =  PACKAGE_VERSION;
  int ret = config_get( LoaderParams,sizeof(LoaderParams)/sizeof(paramdef_t),LOADER_CONFIG_PREFIX);
  if (ret <0) {
       printf("[LOADER]  configuration couldn't be performed via config module, parameters set to default values\n");
       if (loader_data.shlibpath == NULL) {
         loader_data.shlibpath=DEFAULT_PATH;
        }
       loader_data.maxshlibs = DEFAULT_MAXSHLIBS;
  }
  loader_data.shlibs = malloc(loader_data.maxshlibs * sizeof(loader_shlibdesc_t));
  if(loader_data.shlibs == NULL) {
     fprintf(stderr,"[LOADER]  %s %d memory allocation error %s\n",__FILE__, __LINE__,strerror(errno));
     exit_fun("[LOADER] unrecoverable error");
  }
  memset(loader_data.shlibs,0,loader_data.maxshlibs * sizeof(loader_shlibdesc_t));
}

/* build the full shared lib name from the module name */
char *loader_format_shlibpath(char *modname)
{

char *tmpstr;
char *shlibpath   =NULL;
char *shlibversion=NULL;
char *cfgprefix;
paramdef_t LoaderParams[] ={{"shlibpath", NULL, 0, strptr:&shlibpath, defstrval:NULL, TYPE_STRING, 0},
                            {"shlibversion", NULL, 0, strptr:&shlibversion, defstrval:"", TYPE_STRING, 0}};

int ret;




/* looks for specific path for this module in the config file */
/* specific value for a module path and version is located in a modname subsection of the loader section */
/* shared lib name is formatted as lib<module name><module version>.so */
  cfgprefix = malloc(sizeof(LOADER_CONFIG_PREFIX)+strlen(modname)+16);
  if (cfgprefix == NULL) {
      fprintf(stderr,"[LOADER] %s %d malloc error loading module %s, %s\n",__FILE__, __LINE__, modname, strerror(errno));
      exit_fun("[LOADER] unrecoverable error");
  } else {
      sprintf(cfgprefix,LOADER_CONFIG_PREFIX ".%s",modname);
      int ret = config_get( LoaderParams,sizeof(LoaderParams)/sizeof(paramdef_t),cfgprefix);
      if (ret <0) {
          fprintf(stderr,"[LOADER]  %s %d couldn't retrieve config from section %s\n",__FILE__, __LINE__,cfgprefix);
      }
   }
/* no specific path, use loader default shared lib path */
   if (shlibpath == NULL) {
       shlibpath =  loader_data.shlibpath ;
   } 
/* no specific shared lib version */
   if (shlibversion == NULL) {
       shlibversion = "" ;
   } 
/* alloc memory for full module shared lib file name */
   tmpstr = malloc(strlen(shlibpath)+strlen(modname)+strlen(shlibversion)+16);
   if (tmpstr == NULL) {
      fprintf(stderr,"[LOADER] %s %d malloc error loading module %s, %s\n",__FILE__, __LINE__, modname, strerror(errno));
      exit_fun("[LOADER] unrecoverable error");
   }
   if(shlibpath[0] != 0) {
       ret=sprintf(tmpstr,"%s/",shlibpath);
   } else {
       ret = 0;
   }

   sprintf(tmpstr+ret,"lib%s%s.so",modname,shlibversion);
   
  
   return tmpstr; 
}

int load_module_shlib(char *modname,loader_shlibfunc_t *farray, int numf)
{
   void *lib_handle;
   initfunc_t fpi;
   checkverfunc_t fpc;
   getfarrayfunc_t fpg;
   char *shlib_path;
   char *afname=NULL;
   int ret=0;

   if (loader_data.shlibpath  == NULL) {
      loader_init();
   }

   shlib_path = loader_format_shlibpath(modname);

   ret = 0;
   lib_handle = dlopen(shlib_path, RTLD_LAZY|RTLD_NODELETE|RTLD_GLOBAL);
   if (!lib_handle) {
      fprintf(stderr,"[LOADER] library %s is not loaded: %s\n", shlib_path,dlerror());
      ret = -1;
   } else {
      printf("[LOADER] library %s successfully loaded\n", shlib_path);
      afname=malloc(strlen(modname)+15);
      sprintf(afname,"%s_checkbuildver",modname);
      fpc = dlsym(lib_handle,afname);
      if (fpc != NULL ){
	 int chkver_ret = fpc(loader_data.mainexec_buildversion, &(loader_data.shlibs[loader_data.numshlibs].shlib_buildversion));
         if (chkver_ret < 0) {
              fprintf(stderr,"[LOADER]  %s %d lib %s, version mismatch",__FILE__, __LINE__, modname);
              exit_fun("[LOADER] unrecoverable error");
         }
      }
      sprintf(afname,"%s_autoinit",modname);
      fpi = dlsym(lib_handle,afname);

      if (fpi != NULL ) {
	 fpi();
      }

      if (farray != NULL) {
          loader_data.shlibs[loader_data.numshlibs].funcarray=malloc(numf*sizeof(loader_shlibfunc_t));
          loader_data.shlibs[loader_data.numshlibs].numfunc=0;
          for (int i=0; i<numf; i++) {
	      farray[i].fptr = dlsym(lib_handle,farray[i].fname);
	      if (farray[i].fptr == NULL ) {
	          fprintf(stderr,"[LOADER] %s %d %s function not found %s\n",__FILE__, __LINE__, dlerror(),farray[i].fname);
                  ret= -1;
	      } else { /* farray[i].fptr == NULL */
                  loader_data.shlibs[loader_data.numshlibs].funcarray[i].fname=strdup(farray[i].fname); 
                  loader_data.shlibs[loader_data.numshlibs].funcarray[i].fptr = farray[i].fptr;
                  loader_data.shlibs[loader_data.numshlibs].numfunc++;                 
              }/* farray[i].fptr != NULL */
	  } /* for int i... */
      }	else {  /* farray ! NULL */
          sprintf(afname,"%s_getfarray",modname);
          fpg = dlsym(lib_handle,afname);
          if (fpg != NULL ) {
	      loader_data.shlibs[loader_data.numshlibs].numfunc = fpg(&(loader_data.shlibs[loader_data.numshlibs].funcarray));
          }            
      } /* farray ! NULL */
    loader_data.shlibs[loader_data.numshlibs].name=strdup(modname);
    loader_data.shlibs[loader_data.numshlibs].thisshlib_path=strdup(shlib_path); 

    (loader_data.numshlibs)++;
    } /* lib_handle != NULL */ 
	  	 
   if ( shlib_path!= NULL) free(shlib_path);
   if ( afname!= NULL) free(afname);
   if (lib_handle != NULL) dlclose(lib_handle); 
   return ret;	       
}

void * get_shlibmodule_fptr(char *modname, char *fname)
{
    for (int i=0; i<loader_data.numshlibs && loader_data.shlibs[i].name != NULL; i++) {
        if ( strcmp(loader_data.shlibs[i].name, modname) == 0) {
            for (int j =0; j<loader_data.shlibs[i].numfunc ; j++) {
                 if (strcmp(loader_data.shlibs[i].funcarray[j].fname, fname) == 0) {
                     return loader_data.shlibs[i].funcarray[j].fptr;
                 }
            } /* for j loop on module functions*/
        }
    } /* for i loop on modules */
    return NULL;
}
