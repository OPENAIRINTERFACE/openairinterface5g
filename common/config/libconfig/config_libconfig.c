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
/*! \file common/config/libconfig/config_libconfig.c
 * \brief: implementation libconfig configuration library
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE
#include <libconfig.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>


#include "config_libconfig.h"
#include "config_libconfig_private.h"
#include "../config_userapi.h"
#include "errno.h"

#if ( LIBCONFIG_VER_MAJOR == 1 && LIBCONFIG_VER_MINOR < 5)
  #define config_setting_lookup config_lookup_from
#endif

void config_libconfig_end(void );

int read_strlist(paramdef_t *cfgoptions,config_setting_t *setting, char *cfgpath) {
  const char *str;
  int st;
  int numelt;
  numelt=config_setting_length(setting);
  config_check_valptr(cfgoptions,(char **)&(cfgoptions->strlistptr), sizeof(char *) * numelt);
  st=0;

  for (int i=0; i< numelt ; i++) {
    str=config_setting_get_string_elem(setting,i);

    if (str==NULL) {
      printf("[LIBCONFIG] %s%i  not found in config file\n", cfgoptions->optname,i);
    } else {
      config_check_valptr(cfgoptions,&(cfgoptions->strlistptr[i]),strlen(str)+1);
      sprintf(cfgoptions->strlistptr[i],"%s",str);
      st++;
      printf_params("[LIBCONFIG] %s%i: %s\n", cfgpath,i,cfgoptions->strlistptr[i]);
    }
  }

  cfgoptions->numelt=numelt;
  return st;
}

int read_intarray(paramdef_t *cfgoptions,config_setting_t *setting, char *cfgpath) {
  cfgoptions->numelt=config_setting_length(setting);

  if (cfgoptions->numelt > 0) {
    cfgoptions->iptr=malloc(sizeof(int) * (cfgoptions->numelt));

    for (int i=0; i< cfgoptions->numelt && cfgoptions->iptr != NULL; i++) {
      cfgoptions->iptr[i]=config_setting_get_int_elem(setting,i);
      printf_params("[LIBCONFIG] %s[%i]: %i\n", cfgpath,i,cfgoptions->iptr[i]);
    } /* for loop on array element... */
  }

  return cfgoptions->numelt;
}
int config_libconfig_setparams(paramdef_t *cfgoptions, int numoptions, config_setting_t * asetting, char * prefix) {
 int status;
 int errors=0;
 int notused=0;
 #define LIBCONFIG_NOTUSED_PARAMS "Not used? (NULL value ptr)"
 
 for(int i=0; i<numoptions; i++) {
    if (cfgoptions[i].paramflags & PARAMFLAG_CMDLINEONLY) {
      continue;
      printf_params("[LIBCONFIG] setting %s.%s skipped (command line only) \n",(prefix==NULL)?"":prefix,cfgoptions[i].optname);
    }
	status=CONFIG_FALSE;
    config_setting_t * psetting;
    char *spath=malloc(((prefix==NULL)?0:strlen(prefix))+strlen(cfgoptions[i].optname)+10);
    sprintf(spath,"%s%s%s",(prefix==NULL)?"":prefix, (prefix==NULL)?"":".", cfgoptions[i].optname);
    psetting = config_lookup(&(libconfig_privdata.runtcfg),spath);
    free(spath);
    if (psetting != NULL) {
      printf_params("[LIBCONFIG] setting %s.%s already created \n",(prefix==NULL)?"":prefix,cfgoptions[i].optname);
      continue;
    }
    switch(cfgoptions[i].type) {
      case TYPE_STRING:
        psetting =config_setting_add(asetting,cfgoptions[i].optname,CONFIG_TYPE_STRING);
        if (psetting!= NULL) {
          char *value;
          if (cfgoptions[i].strptr == NULL) {
            notused++;
            value=LIBCONFIG_NOTUSED_PARAMS;
          } else {
            if (cfgoptions[i].numelt == 0 ) {
              value = *(cfgoptions[i].strptr);
            } else {
              value = (char *)(cfgoptions[i].strptr);
            }
          }
          status=config_setting_set_string(psetting, value);
        }  
        break;
      case TYPE_STRINGLIST:
        psetting =config_setting_add(asetting,cfgoptions[i].optname,CONFIG_TYPE_LIST);
        if (psetting!= NULL)
          for (int j=0; j<cfgoptions[i].numelt ; j++) {
            config_setting_t *elemsetting=config_setting_set_string_elem(psetting, -1,cfgoptions[i].strptr[j] );
            if (elemsetting == NULL) {
                          fprintf(stderr,"[LIBCONFIG] Error: Creating list %s element  %i value %s\n",cfgoptions[i].optname,j,cfgoptions[i].strptr[j]);
                          break;
                    }
                  }

        break;

      case TYPE_UINT8:
      case TYPE_INT8:
      case TYPE_UINT16:
      case TYPE_INT16:
      case TYPE_UINT32:
      case TYPE_INT32:
      case TYPE_MASK:
        psetting =config_setting_add(asetting,cfgoptions[i].optname,CONFIG_TYPE_INT);
        if (psetting!= NULL)
          status=config_setting_set_int(psetting, (int)*(cfgoptions[i].iptr) );
        break;

      case TYPE_UINT64:
      case TYPE_INT64:
        psetting =config_setting_add(asetting,cfgoptions[i].optname,CONFIG_TYPE_INT64);
        if (psetting!= NULL)
          status=config_setting_set_int64(psetting, (int64_t)*(cfgoptions[i].i64ptr) );
        break;

      case TYPE_UINTARRAY:
      case TYPE_INTARRAY:
        psetting =config_setting_add(asetting,cfgoptions[i].optname,CONFIG_TYPE_ARRAY);
        if (psetting!= NULL)
          for (int j=0; j<cfgoptions[i].numelt ; j++) {
            config_setting_t *elemsetting=config_setting_set_int_elem(psetting, -1,(int)(cfgoptions[i].iptr[j]) );
            if (elemsetting == NULL) {
			  fprintf(stderr,"[LIBCONFIG] Error: Creating array %s, at index %i value %i\n",cfgoptions[i].optname,j,(int)(cfgoptions[i].iptr[j]));
			  break;
		    }
		  }
        break;
        break;

      case TYPE_DOUBLE:
        psetting =config_setting_add(asetting,cfgoptions[i].optname,CONFIG_TYPE_FLOAT);
        if (psetting!= NULL)      
          status=config_setting_set_float(psetting, (double)*(cfgoptions[i].dblptr) );
        break;

      case TYPE_IPV4ADDR:
        break;

      case TYPE_LIST:
        break;

      default:
        fprintf(stderr,"[LIBCONFIG] %s type %i  not supported\n",cfgoptions[i].optname ,cfgoptions[i].type);
        status=CONFIG_FALSE;
        break;
    } /* switch on param type */
    if (status != CONFIG_TRUE) {
		errors++;
		fprintf(stderr,"[LIBCONFIG] Error creating setting %i: %s type %i\n",i,cfgoptions[i].optname ,cfgoptions[i].type);
	}	
  }
  if (notused > 0)
    printf_params("[LIBCONFIG], %i settings with NULL value pointer\n",notused);
    
  if (errors ==0) {
	  printf_params("[LIBCONFIG], %i settings set successfully \n",numoptions);
  } else {
	  fprintf(stderr,"[LIBCONFIG] ...%i/%i settings creation errors \n",errors,numoptions);
  }
  if ( cfgptr->status ) {
	 cfgptr->status->num_err_nullvalue += notused;
	 cfgptr->status->num_err_write += errors;
	 cfgptr->status->num_write += numoptions;
  }
  return errors;
}

int  config_libconfig_set(paramdef_t *cfgoptions, int numoptions, char *prefix ) {
  char *tokctx1=NULL;
  int listidx=-1;
  char *prefixbck=NULL;
  char *prefix_elem1=NULL;
  config_setting_t * asetting = config_root_setting (&(libconfig_privdata.runtcfg));	 
  if (prefix != NULL) {
	 prefixbck=strdup(prefix);
	 prefix_elem1 = strtok_r(prefixbck, ".", &tokctx1);
  }
  printf_params("[LIBCONFIG], processing prefix %s\n",(prefix==NULL)?"NULL":prefix);
  
  /* parse the prefix , possibly creating groups, lists and list elements */
  while (prefix_elem1 != NULL) {
	int n1=strlen(prefix_elem1);
	char *prefix_elem2 =  prefix_elem1+n1+1 ;
	printf_params("[LIBCONFIG], processing elem1 %s elem2 %s\n",prefix_elem1,prefix_elem2);
/* prefix (the path to the parameter name) may contain groups and elements from a list, which are specified with [] */	
	if (prefix_elem2[0] != '[') {  // not a list
	  
	  config_setting_t *tmpset = config_setting_lookup(asetting,prefix_elem1);
	  if (tmpset == NULL)
	    asetting = config_setting_add (asetting, prefix_elem1, CONFIG_TYPE_GROUP);
	  else
	    asetting = tmpset;
	  printf_params("[LIBCONFIG], creating or looking for group %s: %s\n",prefix_elem1,(asetting==NULL)?"NOK":"OK");
    } else {                       // a list
	  listidx=(int)strtol(prefix_elem2+1,NULL,10);
	  if (errno == EINVAL || errno == ERANGE) {
		 printf_params("[LIBCONFIG], Error %s looking for list index in  %s \n",strerror(errno), prefix_elem2); 
		 break;
      }
	  config_setting_t *tmpset = config_setting_lookup(asetting,prefix_elem1);
	  if (tmpset == NULL)
	    asetting = config_setting_add (asetting, prefix_elem1, CONFIG_TYPE_LIST);
	  else
	    asetting = tmpset;
	  printf_params("[LIBCONFIG], creating or looking for list %s: %s\n",prefix_elem1,(asetting==NULL)?"NOK":"OK");
      if (asetting != NULL) {
	     tmpset = config_setting_get_elem (asetting, listidx);
         if (tmpset == NULL)
           asetting = config_setting_add (asetting, NULL, CONFIG_TYPE_GROUP);
         else
           asetting = tmpset;
      printf_params("[LIBCONFIG], creating or looking for list element %i: %s\n",listidx,(asetting==NULL)?"NOK":"OK");  
      }
      prefix_elem1 = strtok_r(NULL, ".", &tokctx1); //skip the [x] elements we already took care of it
      if (prefix_elem1 == NULL) {
          fprintf(stderr, "[LICONFIG] End of parsing %s \n",prefix);
          break;
      }
	}
	if (asetting == NULL) {
		fprintf(stderr,"[LIBCONFIG] Error creating setting %s %s %i\n",prefix_elem1,prefix_elem2,listidx);
	}
	prefix_elem1 = strtok_r(NULL, ".", &tokctx1);
  }
  free(prefixbck);
  if (asetting != NULL) {
    config_libconfig_setparams(cfgoptions, numoptions,asetting,prefix);
    printf_params("[LIBCONFIG] %i settings added in group %s\n",numoptions,(prefix==NULL)?"":prefix);
    return 0;
  } else {
    fprintf(stderr,"[LIBCONFIG] Error parsing %s params, skipped...\n",(prefix==NULL)?"":prefix);
    return -1;
  }
} 


int config_libconfig_get(paramdef_t *cfgoptions,int numoptions, char *prefix ) {
  config_setting_t *setting;
  char *str;
  int i,u;
  long long int llu;
  double dbl;
  int rst;
  int status=0;
  int notfound;
  int defval;
  int fatalerror=0;
  int numdefvals=0;
  char cfgpath[512];  /* 512 should be enough for the sprintf below */

  for(i=0; i<numoptions; i++) {
    if (prefix != NULL) {
      sprintf(cfgpath,"%s.%s",prefix,cfgoptions[i].optname);
    } else {
      sprintf(cfgpath,"%s",cfgoptions[i].optname);
    }

    if( (cfgoptions->paramflags & PARAMFLAG_DONOTREAD) != 0) {
      printf_params("[LIBCONFIG] %s.%s ignored\n", cfgpath,cfgoptions[i].optname );
      continue;
    }

    notfound=0;
    defval=0;

    switch(cfgoptions[i].type) {
      case TYPE_STRING:
        if ( config_lookup_string(&(libconfig_privdata.cfg), cfgpath, (const char**)&str)) {
          if ( cfgoptions[i].numelt > 0  && str != NULL && strlen(str) >= cfgoptions[i].numelt ) {
            fprintf(stderr,"[LIBCONFIG] %s:  %s exceeds maximum length of %i bytes, value truncated\n",
                    cfgpath,str,cfgoptions[i].numelt);
            str[strlen(str)-1] = 0;
          }

          if (cfgoptions[i].numelt == 0 ) {
            //          config_check_valptr(&(cfgoptions[i]), (char **)(&(cfgoptions[i].strptr)), sizeof(char *));
            config_check_valptr(&(cfgoptions[i]), cfgoptions[i].strptr, strlen(str)+1);
            sprintf( *(cfgoptions[i].strptr) , "%s", str);
            printf_params("[LIBCONFIG] %s: \"%s\"\n", cfgpath,*(cfgoptions[i].strptr) );
          } else {
            sprintf( (char *)(cfgoptions[i].strptr) , "%s", str);
            printf_params("[LIBCONFIG] %s: \"%s\"\n", cfgpath,(char *)cfgoptions[i].strptr );
          }
        } else {
          defval=config_setdefault_string(&(cfgoptions[i]),prefix);
        }

        break;

      case TYPE_STRINGLIST:
        setting = config_setting_lookup (config_root_setting(&(libconfig_privdata.cfg)),cfgpath );

        if ( setting != NULL) {
          read_strlist(&cfgoptions[i],setting,cfgpath);
        } else {
          defval=config_setdefault_stringlist(&(cfgoptions[i]),prefix);
        }

        break;

      case TYPE_UINT8:
      case TYPE_INT8:
      case TYPE_UINT16:
      case TYPE_INT16:
      case TYPE_UINT32:
      case TYPE_INT32:
      case TYPE_MASK:
        if ( config_lookup_int(&(libconfig_privdata.cfg),cfgpath, &u)) {
          config_check_valptr(&(cfgoptions[i]), (char **)(&(cfgoptions[i].iptr)),sizeof(int32_t));
          config_assign_int(&(cfgoptions[i]),cfgpath,u);
        } else {
          defval=config_setdefault_int(&(cfgoptions[i]),prefix);
        }

        break;

      case TYPE_UINT64:
      case TYPE_INT64:
        if ( config_lookup_int64(&(libconfig_privdata.cfg),cfgpath, &llu)) {
          config_check_valptr(&(cfgoptions[i]), (char **)&(cfgoptions[i].i64ptr),sizeof(long long));

          if(cfgoptions[i].type==TYPE_UINT64) {
            *(cfgoptions[i].u64ptr) = (uint64_t)llu;
            printf_params("[LIBCONFIG] %s: %llu\n", cfgpath,(long long unsigned)(*(cfgoptions[i].u64ptr)) );
          } else {
            *(cfgoptions[i].i64ptr) = llu;
            printf_params("[LIBCONFIG] %s: %lld\n", cfgpath,(long long)(*(cfgoptions[i].i64ptr)) );
          }
        } else {
          defval=config_setdefault_int64(&(cfgoptions[i]),prefix);
        }

        break;

      case TYPE_UINTARRAY:
      case TYPE_INTARRAY:
        setting = config_setting_lookup (config_root_setting(&(libconfig_privdata.cfg)),cfgpath );

        if ( setting != NULL) {
          read_intarray(&cfgoptions[i],setting,cfgpath);
        } else {
          defval=config_setdefault_intlist(&(cfgoptions[i]),prefix);
        }

        break;

      case TYPE_DOUBLE:
        if ( config_lookup_float(&(libconfig_privdata.cfg),cfgpath, &dbl)) {
          config_check_valptr(&(cfgoptions[i]), (char **)&(cfgoptions[i].dblptr),sizeof(double));
          *(cfgoptions[i].dblptr) = dbl;
          printf_params("[LIBCONFIG] %s: %lf\n", cfgpath,*(cfgoptions[i].dblptr) );
        } else {
          defval=config_setdefault_double(&(cfgoptions[i]),prefix);
        }

        break;

      case TYPE_IPV4ADDR:
        if ( !config_lookup_string(&(libconfig_privdata.cfg),cfgpath, (const char **)&str)) {
          defval=config_setdefault_ipv4addr(&(cfgoptions[i]),prefix);
        } else {
          rst=config_assign_ipv4addr(cfgoptions, str);

          if (rst < 0) {
            fprintf(stderr,"[LIBCONFIG] %s not valid for %s \n", str, cfgpath);
            fatalerror=1;
          }
        }

        break;

      case TYPE_LIST:
        setting = config_setting_lookup (config_root_setting(&(libconfig_privdata.cfg)),cfgpath );

        if ( setting) {
          cfgoptions[i].numelt=config_setting_length(setting);
        } else {
          notfound=1;
        }

        break;

      default:
        fprintf(stderr,"[LIBCONFIG] %s type %i  not supported\n", cfgpath,cfgoptions[i].type);
        fatalerror=1;
        break;
    } /* switch on param type */

    if( notfound == 1) {
      printf("[LIBCONFIG] %s not found in %s ", cfgpath,libconfig_privdata.configfile );

      if ( (cfgoptions[i].paramflags & PARAMFLAG_MANDATORY) != 0) {
        fatalerror=1;
        printf("  mandatory parameter missing\n");
      } else {
        printf("\n");
      }
    } else {
      if (defval == 1) {
        numdefvals++;
        cfgoptions[i].paramflags = cfgoptions[i].paramflags |  PARAMFLAG_PARAMSETDEF;
      } else {
        cfgoptions[i].paramflags = cfgoptions[i].paramflags |  PARAMFLAG_PARAMSET;
      }

      status++;
    }
  } /* for loop on options */

  printf("[LIBCONFIG] %s: %i/%i parameters successfully set, (%i to default value)\n",
         ((prefix == NULL)?"(root)":prefix),
         status,numoptions,numdefvals );

  if (fatalerror == 1) {
    fprintf(stderr,"[LIBCONFIG] fatal errors found when processing  %s \n", libconfig_privdata.configfile );
    config_libconfig_end();
    end_configmodule();
  }

  return status;
}

int config_libconfig_getlist(paramlist_def_t *ParamList,
                             paramdef_t *params, int numparams, char *prefix) {
  config_setting_t *setting;
  int i,j,status;
  char *listpath=NULL;
  char cfgpath[MAX_OPTNAME_SIZE*2 + 6]; /* prefix.listname.[listindex] */

  if (prefix != NULL) {
    i=asprintf(&listpath ,"%s.%s",prefix,ParamList->listname);
  } else {
    i=asprintf(&listpath ,"%s",ParamList->listname);
  }

  setting = config_lookup(&(libconfig_privdata.cfg), listpath);

  if ( setting) {
    status = ParamList->numelt = config_setting_length(setting);
    printf_params("[LIBCONFIG] %i %s in config file %s \n",
                  ParamList->numelt,listpath,libconfig_privdata.configfile );
  } else {
    printf("[LIBCONFIG] list %s not found in config file %s \n",
           listpath,libconfig_privdata.configfile );
    ParamList->numelt= 0;
    status = -1;
  }

  if (ParamList->numelt > 0 && params != NULL) {
    ParamList->paramarray = malloc(ParamList->numelt * sizeof(paramdef_t *));

    if ( ParamList->paramarray != NULL) {
      config_get_if()->ptrs[config_get_if()->numptrs] = (char *)(ParamList->paramarray);
      config_get_if()->numptrs++;
    } else {
      fprintf (stderr,"[LIBCONFIG] %s %d malloc error\n",__FILE__, __LINE__);
      exit(-1);
    }

    for (i=0 ; i < ParamList->numelt ; i++) {
      ParamList->paramarray[i] = malloc(numparams * sizeof(paramdef_t));

      if ( ParamList->paramarray[i] != NULL) {
        config_get_if()->ptrs[config_get_if()->numptrs] = (char *)(ParamList->paramarray[i]);
        config_get_if()->numptrs++;
      } else {
        fprintf (stderr,"[LIBCONFIG] %s %d malloc error\n",__FILE__, __LINE__);
        exit(-1);
      }

      memcpy(ParamList->paramarray[i], params, sizeof(paramdef_t)*numparams);

      for (j=0; j<numparams; j++) {
        ParamList->paramarray[i][j].strptr = NULL ;
      }

      sprintf(cfgpath,"%s.[%i]",listpath,i);
      config_libconfig_get(ParamList->paramarray[i], numparams, cfgpath );
    } /* for i... */
  } /* ParamList->numelt > 0 && params != NULL */

  if (listpath != NULL)
    free(listpath);

  return status;
}

int config_libconfig_init(char *cfgP[], int numP) {
  config_init(&(libconfig_privdata.cfg));
  libconfig_privdata.configfile = strdup((char *)cfgP[0]);
  config_get_if()->numptrs=0;
  memset(config_get_if()->ptrs,0,sizeof(void *) * CONFIG_MAX_ALLOCATEDPTRS);
  memset(config_get_if()->ptrsAllocated, 0, sizeof(config_get_if()->ptrsAllocated));
  /* search for include path parameter and set config file include path accordingly */
  for (int i=0; i<numP; i++) {
  	  if (strncmp(cfgP[i],"incp",4) == 0) {
  	  	  config_set_include_dir (&(libconfig_privdata.cfg),cfgP[i]+4);
  	  break;
  	  }
  }
  /* dirname may modify the input path and returned ptr may points to part of input path */
  char *tmppath = strdup(libconfig_privdata.configfile);
  if ( config_get_include_dir (&(libconfig_privdata.cfg)) == NULL) {
  	 config_set_include_dir (&(libconfig_privdata.cfg),dirname( tmppath )); 	 
  }
  
  const char *incp = config_get_include_dir (&(libconfig_privdata.cfg)) ;
 
  printf("[LIBCONFIG] Path for include directive set to: %s\n", (incp!=NULL)?incp:"libconfig defaults");
  /* set convertion option to allow integer to float conversion*/
   config_set_auto_convert (&(libconfig_privdata.cfg), CONFIG_TRUE);
  /* Read the file. If there is an error, report it and exit. */
  if( config_read_file(&(libconfig_privdata.cfg), libconfig_privdata.configfile) == CONFIG_FALSE) {
    fprintf(stderr,"[LIBCONFIG] %s %d file %s - line %d: %s\n",__FILE__, __LINE__,
            libconfig_privdata.configfile, config_error_line(&(libconfig_privdata.cfg)),
            config_error_text(&(libconfig_privdata.cfg)));
    config_destroy(&(libconfig_privdata.cfg));
    printf( "\n");
    free(tmppath);
    return -1;
  }
  
  /* possibly init a libconfig struct for saving really used params */
  if( cfgptr->rtflags & CONFIG_SAVERUNCFG ) {
	  config_init ( &(libconfig_privdata.runtcfg)); 
//	  config_set_options (&(libconfig_privdata.runtcfg), CONFIG_OPTION_ALLOW_OVERRIDES, CONFIG_TRUE);
  }
  free(tmppath);
  return 0;
}


void config_libconfig_end(void ) {
  config_destroy(&(libconfig_privdata.cfg));
  if( cfgptr->rtflags & CONFIG_SAVERUNCFG ) {
      char *fname=strdup(libconfig_privdata.configfile);
      int newcfgflen = strlen( libconfig_privdata.configfile) + strlen(cfgptr->tmpdir) + 20;
	  char *newcfgf = malloc( newcfgflen);
	  time_t t = time(NULL);
      struct tm tm = *localtime(&t);
	  snprintf (newcfgf,newcfgflen, "%s/%s-run%d_%02d_%02d_%02d%02d",cfgptr->tmpdir,basename(fname),
	           tm.tm_year + 1900,tm.tm_mon + 1,tm.tm_mday, tm.tm_hour, tm.tm_min);
	  if ( config_write_file (&(libconfig_privdata.runtcfg) , newcfgf) != CONFIG_TRUE) {
		 fprintf(stderr,"[LIBCONFIG] %s %d file %s - line %d: %s\n",__FILE__, __LINE__,
            newcfgf, config_error_line(&(libconfig_privdata.runtcfg)),
            config_error_text(&(libconfig_privdata.runtcfg)));
      } else {
		 printf("[LIBCONFIG] file %s created successfully\n", newcfgf);  
	  }
	  config_destroy(&(libconfig_privdata.runtcfg));
      free(fname);
	  free(newcfgf);
  }
  if ( libconfig_privdata.configfile != NULL ) {
    free(libconfig_privdata.configfile);
    libconfig_privdata.configfile=NULL;
  }
}
