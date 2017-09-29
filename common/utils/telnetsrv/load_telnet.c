
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
#include "telnetsrv.h"
#include "openair1/PHY/defs.h"

int load_telnet(void)
{
   void *lib_handle;
   initfunc_t fpi;

 
   lib_handle = dlopen(TELNETSRV_SHAREDLIB, RTLD_LAZY|RTLD_NODELETE|RTLD_GLOBAL);
   if (!lib_handle) 
   {
      printf("[TELNETSRV] telnet server is not loaded: %s\n", dlerror());
      return -1;
   } 
   
   fpi = dlsym(lib_handle,"init_telnetsrv");

     if (fpi != NULL )
         {
	 fpi(cfgfile);
	 }
      else
         {
         fprintf(stderr,"[TELNETSRV] %s %d Telnet server init function not found %s\n",__FILE__, __LINE__, dlerror());
         return -1;
         } 
	  	 

   dlclose(lib_handle); 
   return 0;	       
}
