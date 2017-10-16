
/* FT NOKBLF:
 *  this source is to be linked with the program using the telnet server, it looks for
 *  the telnet server dynamic library, possibly loads it and calls the telnet server
 *  init functions
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
#include "load_module_shlib.h"
int load_module_shlib(char *modname)
{
   void *lib_handle;
   initfunc_t fpi;
   char *tmpstr;
   int ret=0;
 
   tmpstr = malloc(strlen(modname)+16);
   if (tmpstr == NULL) {
      fprintf(stderr,"[LOADER] %s %d malloc error loading module %s, %s\n",__FILE__, __LINE__, modname, strerror(errno));
      return -1; 
   }
   sprintf(tmpstr,"lib%s.so",modname);
   lib_handle = dlopen(tmpstr, RTLD_LAZY|RTLD_NODELETE|RTLD_GLOBAL);
   if (!lib_handle) {
      printf("[LOADER] library %s is not loaded: %s\n", tmpstr,dlerror());
      ret = -1;
   } else {
      sprintf(tmpstr,"init_%s",modname);
      fpi = dlsym(lib_handle,tmpstr);

      if (fpi != NULL )
         {
	 fpi();
	 }
      else
         {
         fprintf(stderr,"[LOADER] %s %d %s function not found %s\n",__FILE__, __LINE__, dlerror(),tmpstr);
         ret =  -1;
         }
    } 
	  	 
   if (tmpstr != NULL) free(tmpstr);
   if (lib_handle != NULL) dlclose(lib_handle); 
   return ret;	       
}
