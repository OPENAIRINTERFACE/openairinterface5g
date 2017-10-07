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
#include <sys/ioctl.h>
#include <dlfcn.h>
#include "openair1/PHY/defs.h"
#define LOAD_MODULE_SHLIB_MAIN

#include "common/config/config_userapi.h"
#include "load_module_shlib.h"
void loader_init(void) {
  paramdef_t LoaderParams[] = LOADER_PARAMS_DESC;


  int ret = config_get( LoaderParams,sizeof(LoaderParams)/sizeof(paramdef_t),LOADER_CONFIG_PREFIX);
  if (ret <0) {
       fprintf(stderr,"[LOADER] configuration couldn't be performed");
       if (loader_data.shlibpath == NULL) {
         loader_data.shlibpath=DEFAULT_PATH;
        }
       return;
  }   
}

int load_module_shlib(char *modname,loader_shlibfunc_t *farray, int numf)
{
   void *lib_handle;
   initfunc_t fpi;
   char *tmpstr;
   int ret=0;

   if (loader_data.shlibpath  == NULL) {
      loader_init();
   }
   tmpstr = malloc(strlen(loader_data.shlibpath)+strlen(modname)+16);
   if (tmpstr == NULL) {
      fprintf(stderr,"[LOADER] %s %d malloc error loading module %s, %s\n",__FILE__, __LINE__, modname, strerror(errno));
      return -1; 
   }

   if(loader_data.shlibpath[0] != 0) {
       ret=sprintf(tmpstr,"%s/",loader_data.shlibpath);
   }
   if(strstr(modname,".so") == NULL) {
      sprintf(tmpstr+ret,"lib%s.so",modname);
   } else {
      sprintf(tmpstr+ret,"%s",modname);   
   } 
   ret = 0;
   lib_handle = dlopen(tmpstr, RTLD_LAZY|RTLD_NODELETE|RTLD_GLOBAL);
   if (!lib_handle) {
      fprintf(stderr,"[LOADER] library %s is not loaded: %s\n", tmpstr,dlerror());
      ret = -1;
   } else {
      printf("[LOADER] library %s uccessfully loaded loaded\n", tmpstr);
      sprintf(tmpstr,"%s_autoinit",modname);
      fpi = dlsym(lib_handle,tmpstr);

      if (fpi != NULL )
         {
	 fpi();
	 }

      if (farray != NULL) {
          for (int i=0; i<numf; i++) {
	      farray[i].fptr = dlsym(lib_handle,farray[i].fname);
	      if (farray[i].fptr == NULL ) {
	          fprintf(stderr,"[LOADER] %s %d %s function not found %s\n",__FILE__, __LINE__, dlerror(),farray[i].fname);
               ret= -1;
	      }
	  } /* for int i... */
      }	 /* farray ! NULL */
    } 
	  	 
   if (tmpstr != NULL) free(tmpstr);
   if (lib_handle != NULL) dlclose(lib_handle); 
   return ret;	       
}
