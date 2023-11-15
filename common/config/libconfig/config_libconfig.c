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
#include <pthread.h>
#include <libconfig.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "config_libconfig.h"
#include "config_libconfig_private.h"
#define NOT_uniqCfg
#include "../config_common.h"
#include "../config_userapi.h"
#include "errno.h"
#include "assertions.h"

#if ( LIBCONFIG_VER_MAJOR == 1 && LIBCONFIG_VER_MINOR < 5)
  #define config_setting_lookup config_lookup_from
#endif

void config_libconfig_end(configmodule_interface_t *cfg);

int read_strlist(configmodule_interface_t *cfg, paramdef_t *cfgoptions, config_setting_t *setting, char *cfgpath)
{
  const char *str;
  int st;
  int numelt;
  numelt=config_setting_length(setting);
  config_check_valptr(cfg, cfgoptions, sizeof(char *), numelt);
  st=0;
  AssertFatal(MAX_LIST_SIZE > numelt, 
              "This piece of code use fixed size arry of constant #define MAX_LIST_SIZE %d\n", 
              MAX_LIST_SIZE );
  for (int i=0; i< numelt ; i++) {
    str=config_setting_get_string_elem(setting,i);

    if (str==NULL) {
      printf("[LIBCONFIG] %s%i  not found in config file\n", cfgoptions->optname,i);
    } else {
      snprintf(cfgoptions->strlistptr[i], DEFAULT_EXTRA_SZ, "%s",str);
      st++;
      printf_params(cfg, "[LIBCONFIG] %s%i: %s\n", cfgpath, i, cfgoptions->strlistptr[i]);
    }
  }

  cfgoptions->numelt=numelt;
  return st;
}

static int read_intarray(configmodule_interface_t *cfg, paramdef_t *cfgoptions, config_setting_t *setting, char *cfgpath)
{
  cfgoptions->numelt=config_setting_length(setting);

  if (cfgoptions->numelt > 0) {
    cfgoptions->iptr =
        config_allocate_new(cfg, cfgoptions->numelt * sizeof(*cfgoptions->iptr), !(cfgoptions->paramflags & PARAMFLAG_NOFREE));

    for (int i=0; i< cfgoptions->numelt && cfgoptions->iptr != NULL; i++) {
      cfgoptions->iptr[i]=config_setting_get_int_elem(setting,i);
      printf_params(cfg, "[LIBCONFIG] %s[%i]: %i\n", cfgpath, i, cfgoptions->iptr[i]);
    } /* for loop on array element... */
  }

  return cfgoptions->numelt;
}
int config_libconfig_setparams(configmodule_interface_t *cfg,
                               paramdef_t *cfgoptions,
                               int numoptions,
                               config_setting_t *asetting,
                               char *prefix)
{
  int status;
  int errors = 0;
  int notused = 0;
  int emptyla = 0;
#define LIBCONFIG_NOTUSED_PARAMS "Not used? (NULL value ptr)"
  char *secprefix = (prefix == NULL) ? "" : prefix;

  for (int i = 0; i < numoptions; i++) {
    if (cfgoptions[i].paramflags & PARAMFLAG_CMDLINEONLY) {
      continue;
      printf_params(cfg, "[LIBCONFIG] setting %s.%s skipped (command line only) \n", secprefix, cfgoptions[i].optname);
    }
    status = CONFIG_FALSE;
    config_setting_t *psetting;
    char *spath = malloc(((prefix == NULL) ? 0 : strlen(prefix)) + strlen(cfgoptions[i].optname) + 10);
    sprintf(spath, "%s%s%s", secprefix, (prefix == NULL) ? "" : ".", cfgoptions[i].optname);
    psetting = config_lookup(&(libconfig_privdata.runtcfg), spath);
    free(spath);
    if (psetting != NULL) {
      printf_params(cfg, "[LIBCONFIG] setting %s.%s already created \n", secprefix, cfgoptions[i].optname);
      continue;
    }
    switch (cfgoptions[i].type) {
      case TYPE_STRING:
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_STRING);
        if (psetting != NULL) {
          char *value;
          if (cfgoptions[i].strptr == NULL) {
            notused++;
            value = LIBCONFIG_NOTUSED_PARAMS;
          } else {
            value = *(cfgoptions[i].strptr);
          }
          status = config_setting_set_string(psetting, value);
        }
        break;
      case TYPE_STRINGLIST:
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_LIST);
        if (psetting != NULL) {
          if (cfgoptions[i].numelt <= 0) {
            emptyla++;
            printf_params(cfg, "[LIBCONFIG], no element in list %s\n", cfgoptions[i].optname);
            status = CONFIG_TRUE;
          }
          for (int j = 0; j < cfgoptions[i].numelt; j++) {
            config_setting_t *elemsetting = config_setting_set_string_elem(psetting, -1, cfgoptions[i].strptr[j]);
            if (elemsetting == NULL) {
              fprintf(stderr, "[LIBCONFIG] Error: Creating list %s element  %i value %s\n", cfgoptions[i].optname, j, cfgoptions[i].strptr[j]);
              break;
            } else
              status = CONFIG_TRUE;
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
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_INT);
        if (psetting != NULL)
          status = config_setting_set_int(psetting, (int)*(cfgoptions[i].iptr));
        break;

      case TYPE_UINT64:
      case TYPE_INT64:
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_INT64);
        if (psetting != NULL)
          status = config_setting_set_int64(psetting, (int64_t) * (cfgoptions[i].i64ptr));
        break;

      case TYPE_UINTARRAY:
      case TYPE_INTARRAY:
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_ARRAY);
        if (psetting != NULL) {
          if (cfgoptions[i].numelt <= 0) {
            emptyla++;
            printf_params(cfg, "[LIBCONFIG], no element in array %s.%s\n", secprefix, cfgoptions[i].optname);
            status = CONFIG_TRUE;
          }
          for (int j = 0; j < cfgoptions[i].numelt; j++) {
            config_setting_t *elemsetting = config_setting_set_int_elem(psetting, -1, (int)(cfgoptions[i].iptr[j]));
            if (elemsetting == NULL) {
              fprintf(stderr, "[LIBCONFIG] Error: Creating array %s.%s, at index %i value %i\n", secprefix, cfgoptions[i].optname, j, (int)(cfgoptions[i].iptr[j]));
              break;
            } else
              status = CONFIG_TRUE;
          }
        }
        break;

      case TYPE_DOUBLE:
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_FLOAT);
        if (psetting != NULL)
          status = config_setting_set_float(psetting, (double)*(cfgoptions[i].dblptr));
        break;

      case TYPE_IPV4ADDR:
        psetting = config_setting_add(asetting, cfgoptions[i].optname, CONFIG_TYPE_STRING);
        if (psetting != NULL) {
          char ipstr[INET_ADDRSTRLEN];
          if (inet_ntop(AF_INET, cfgoptions[i].uptr, ipstr, INET_ADDRSTRLEN) == NULL) {
            notused++;
            sprintf(ipstr, "undef");
          } else {
            status = config_setting_set_string(psetting, ipstr);
          }
        }
        break;

      case TYPE_LIST:
        break;

      default:
        fprintf(stderr, "[LIBCONFIG] %s.%s type %i  not supported\n", secprefix, cfgoptions[i].optname, cfgoptions[i].type);
        status = CONFIG_FALSE;
        break;
    } /* switch on param type */
    if (status != CONFIG_TRUE) {
      errors++;
      fprintf(stderr, "[LIBCONFIG] Error creating setting %i: %s.%s type %i\n", i, secprefix, cfgoptions[i].optname, cfgoptions[i].type);
    }
  }
  printf_params(cfg, "[LIBCONFIG], in group \"%s\" %i settings \n", secprefix, numoptions);
  if (notused > 0)
    printf_params(cfg, "[LIBCONFIG], ..... %i settings with NULL value pointer\n", notused);
  if (errors > 0)
    fprintf(stderr, "[LIBCONFIG] .....%i settings creation errors \n", errors);
  if (emptyla > 0)
    fprintf(stderr, "[LIBCONFIG] .....%i empty lists or arrays settings \n", emptyla);

  if (cfg->status) {
    cfg->status->num_err_nullvalue += notused;
    cfg->status->num_err_write += errors;
    cfg->status->num_write += numoptions;
    cfg->status->emptyla += emptyla;
  }
  return errors;
}

int config_libconfig_set(configmodule_interface_t *cfg, paramdef_t *cfgoptions, int numoptions, char *prefix)
{
  char *tokctx1 = NULL;
  int listidx = -1;
  char *prefixbck = NULL;
  char *prefix_elem1 = NULL;
  config_setting_t *asetting = config_root_setting(&(libconfig_privdata.runtcfg));
  if (prefix != NULL) {
    prefixbck = strdup(prefix);
    prefix_elem1 = strtok_r(prefixbck, ".", &tokctx1);
  }
  printf_params(cfg, "[LIBCONFIG], processing prefix %s\n", (prefix == NULL) ? "NULL" : prefix);

  /* parse the prefix , possibly creating groups, lists and list elements */
  while (prefix_elem1 != NULL) {
    int n1 = strlen(prefix_elem1);
    char *prefix_elem2 = prefix_elem1 + n1 + 1;
    printf_params(cfg, "[LIBCONFIG], processing elem1 %s elem2 %s\n", prefix_elem1, prefix_elem2);
    /* prefix (the path to the parameter name) may contain groups and elements from a list, which are specified with [] */
    if (prefix_elem2[0] != '[') { // not a list

      config_setting_t *tmpset = config_setting_lookup(asetting, prefix_elem1);
      if (tmpset == NULL)
        asetting = config_setting_add(asetting, prefix_elem1, CONFIG_TYPE_GROUP);
      else
        asetting = tmpset;
      printf_params(cfg, "[LIBCONFIG], creating or looking for group %s: %s\n", prefix_elem1, (asetting == NULL) ? "NOK" : "OK");
    } else { // a list
      listidx = (int)strtol(prefix_elem2 + 1, NULL, 10);
      if (errno == EINVAL || errno == ERANGE) {
        printf_params(cfg, "[LIBCONFIG], Error %s looking for list index in  %s \n", strerror(errno), prefix_elem2);
        break;
      }
      config_setting_t *tmpset = config_setting_lookup(asetting, prefix_elem1);
      if (tmpset == NULL)
        asetting = config_setting_add(asetting, prefix_elem1, CONFIG_TYPE_LIST);
      else
        asetting = tmpset;
      printf_params(cfg, "[LIBCONFIG], creating or looking for list %s: %s\n", prefix_elem1, (asetting == NULL) ? "NOK" : "OK");
      if (asetting != NULL) {
        tmpset = config_setting_get_elem(asetting, listidx);
        if (tmpset == NULL)
          asetting = config_setting_add(asetting, NULL, CONFIG_TYPE_GROUP);
        else
          asetting = tmpset;
        printf_params(cfg,
                      "[LIBCONFIG], creating or looking for list element %i: %s\n",
                      listidx,
                      (asetting == NULL) ? "NOK" : "OK");
      }
      prefix_elem1 = strtok_r(NULL, ".", &tokctx1); // skip the [x] elements we already took care of it
      if (prefix_elem1 == NULL) {
        fprintf(stderr, "[LICONFIG] End of parsing %s \n", prefix);
        break;
      }
    }
    if (asetting == NULL) {
      fprintf(stderr, "[LIBCONFIG] Error creating setting %s %s %i\n", prefix_elem1, prefix_elem2, listidx);
    }
    prefix_elem1 = strtok_r(NULL, ".", &tokctx1);
  }
  free(prefixbck);
  if (asetting != NULL) {
    config_libconfig_setparams(cfg, cfgoptions, numoptions, asetting, prefix);
    printf_params(cfg, "[LIBCONFIG] %i settings added in group %s\n", numoptions, (prefix == NULL) ? "" : prefix);
    return 0;
  } else {
    fprintf(stderr, "[LIBCONFIG] Error parsing %s params, skipped...\n", (prefix == NULL) ? "" : prefix);
    return -1;
  }
}

int config_libconfig_get(configmodule_interface_t *cfg, paramdef_t *cfgoptions, int numoptions, char *prefix)
{
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
      printf_params(cfg, "[LIBCONFIG] %s.%s ignored\n", cfgpath, cfgoptions[i].optname);
      continue;
    }

    notfound=0;
    defval=0;

    switch(cfgoptions[i].type) {
      case TYPE_STRING:
        if ( config_lookup_string(&(libconfig_privdata.cfg), cfgpath, (const char**)&str)) {
          config_check_valptr(cfg, &cfgoptions[i], 1, strlen(str) + 1);
          if ( strlen(str)+1 > cfgoptions[i].numelt )
            fprintf(stderr,"[LIBCONFIG] %s:  %s exceeds maximum length of %i bytes, value truncated\n",
                    cfgpath,str,cfgoptions[i].numelt);
          snprintf( *cfgoptions[i].strptr , cfgoptions[i].numelt, "%s", str);
          printf_params(cfg, "[LIBCONFIG] %s: \"%s\"\n", cfgpath, *cfgoptions[i].strptr);
        } else {
          defval = config_setdefault_string(cfg, &cfgoptions[i], prefix);
        }

        break;

      case TYPE_STRINGLIST:
        setting = config_setting_lookup (config_root_setting(&(libconfig_privdata.cfg)),cfgpath );

        if ( setting != NULL) {
          read_strlist(cfg, &cfgoptions[i], setting, cfgpath);
        } else {
          defval = config_setdefault_stringlist(cfg, &(cfgoptions[i]), prefix);
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
          config_check_valptr(cfg, &(cfgoptions[i]), sizeof(*cfgoptions[i].iptr), 1);
          config_assign_int(cfg, &(cfgoptions[i]), cfgpath, u);
        } else {
          defval = config_setdefault_int(cfg, &(cfgoptions[i]), prefix);
        }

        break;

      case TYPE_UINT64:
      case TYPE_INT64:
        if ( config_lookup_int64(&(libconfig_privdata.cfg),cfgpath, &llu)) {
          config_check_valptr(cfg, &cfgoptions[i], sizeof(*cfgoptions[i].i64ptr), 1);

          if(cfgoptions[i].type==TYPE_UINT64) {
            *(cfgoptions[i].u64ptr) = (uint64_t)llu;
            printf_params(cfg, "[LIBCONFIG] %s: %lu\n", cfgpath, *cfgoptions[i].u64ptr);
          } else {
            *(cfgoptions[i].i64ptr) = llu;
            printf_params(cfg, "[LIBCONFIG] %s: %ld\n", cfgpath, *cfgoptions[i].i64ptr);
          }
        } else {
          defval = config_setdefault_int64(cfg, &(cfgoptions[i]), prefix);
        }

        break;

      case TYPE_UINTARRAY:
      case TYPE_INTARRAY:
        setting = config_setting_lookup (config_root_setting(&(libconfig_privdata.cfg)),cfgpath );

        if ( setting != NULL) {
          read_intarray(cfg, &cfgoptions[i], setting, cfgpath);
        } else {
          defval = config_setdefault_intlist(cfg, &(cfgoptions[i]), prefix);
        }

        break;

      case TYPE_DOUBLE:
        if ( config_lookup_float(&(libconfig_privdata.cfg),cfgpath, &dbl)) {
          config_check_valptr(cfg, &cfgoptions[i], sizeof(*cfgoptions[i].dblptr), 1);
          *cfgoptions[i].dblptr = dbl;
          printf_params(cfg, "[LIBCONFIG] %s: %lf\n", cfgpath, *(cfgoptions[i].dblptr));
        } else {
          defval = config_setdefault_double(cfg, &(cfgoptions[i]), prefix);
        }

        break;

      case TYPE_IPV4ADDR:
        if ( !config_lookup_string(&(libconfig_privdata.cfg),cfgpath, (const char **)&str)) {
          defval = config_setdefault_ipv4addr(cfg, &(cfgoptions[i]), prefix);
        } else {
          rst = config_assign_ipv4addr(cfg, cfgoptions, str);

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
    config_libconfig_end(cfg);
    end_configmodule(cfg);
  }

  return status;
}

int config_libconfig_getlist(configmodule_interface_t *cfg,
                             paramlist_def_t *ParamList,
                             paramdef_t *params,
                             int numparams,
                             char *prefix)
{
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
    printf_params(cfg, "[LIBCONFIG] %i %s in config file %s \n", ParamList->numelt, listpath, libconfig_privdata.configfile);
  } else {
    printf("[LIBCONFIG] list %s not found in config file %s \n",
           listpath,libconfig_privdata.configfile );
    ParamList->numelt= 0;
    status = -1;
  }

  if (ParamList->numelt > 0 && params != NULL) {
    ParamList->paramarray = config_allocate_new(cfg, ParamList->numelt * sizeof(paramdef_t *), true);

    for (i=0 ; i < ParamList->numelt ; i++) {
      ParamList->paramarray[i] = config_allocate_new(cfg, numparams * sizeof(paramdef_t), true);
      memcpy(ParamList->paramarray[i], params, sizeof(paramdef_t)*numparams);

      for (j=0; j<numparams; j++) {
        ParamList->paramarray[i][j].strptr = NULL ;
      }

      sprintf(cfgpath,"%s.[%i]",listpath,i);
      config_libconfig_get(cfg, ParamList->paramarray[i], numparams, cfgpath);
    } /* for i... */
  } /* ParamList->numelt > 0 && params != NULL */

  if (listpath != NULL)
    free(listpath);

  return status;
}

int config_libconfig_init(configmodule_interface_t *cfg)
{
  char **cfgP = cfg->cfgP;
  int numP = cfg->num_cfgP;
  config_init(&(libconfig_privdata.cfg));
  libconfig_privdata.configfile = strdup((char *)cfgP[0]);
  cfg->numptrs = 0;
  pthread_mutex_init(&cfg->memBlocks_mutex, NULL);
  memset(cfg->oneBlock, 0, sizeof(cfg->oneBlock));
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
  if (cfg->rtflags & CONFIG_SAVERUNCFG) {
    config_init(&(libconfig_privdata.runtcfg));
    //	  config_set_options (&(libconfig_privdata.runtcfg), CONFIG_OPTION_ALLOW_OVERRIDES, CONFIG_TRUE);
  }
  free(tmppath);
  return 0;
}

void config_libconfig_write_parsedcfg(configmodule_interface_t *cfg)
{
  if (cfg->rtflags & CONFIG_SAVERUNCFG) {
    char *fname = strdup(libconfig_privdata.configfile);
    int newcfgflen = strlen(libconfig_privdata.configfile) + strlen(cfg->tmpdir) + 20;
    cfg->status->debug_cfgname = realloc(cfg->status->debug_cfgname, newcfgflen);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(cfg->status->debug_cfgname,
             newcfgflen,
             "%s/%s-run%d_%02d_%02d_%02d%02d",
             cfg->tmpdir,
             basename(fname),
             tm.tm_year + 1900,
             tm.tm_mon + 1,
             tm.tm_mday,
             tm.tm_hour,
             tm.tm_min);
    if (config_write_file(&(libconfig_privdata.runtcfg), cfg->status->debug_cfgname) != CONFIG_TRUE) {
      fprintf(stderr,
              "[LIBCONFIG] %s %d file %s - line %d: %s\n",
              __FILE__,
              __LINE__,
              cfg->status->debug_cfgname,
              config_error_line(&(libconfig_privdata.runtcfg)),
              config_error_text(&(libconfig_privdata.runtcfg)));
    } else {
      printf("[LIBCONFIG] file %s created successfully\n", cfg->status->debug_cfgname);
    }
    free(fname);
  } else {
    printf("[LIBCONFIG] Cannot create config file after parsing: CONFIG_SAVERUNCFG flag not specified\n");
  }
}

void config_libconfig_end(configmodule_interface_t *cfg)
{
  config_destroy(&(libconfig_privdata.cfg));
  if (cfg->rtflags & CONFIG_SAVERUNCFG) {
    config_destroy(&(libconfig_privdata.runtcfg));
  }
  if ( libconfig_privdata.configfile != NULL ) {
    free(libconfig_privdata.configfile);
    libconfig_privdata.configfile=NULL;
  }
}
