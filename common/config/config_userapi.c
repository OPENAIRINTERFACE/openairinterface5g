/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief configuration module, api implementation to access configuration parameters
 */

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include "common/platform_types.h"
#include "config_userapi.h"
#include "config_common.h"
#include "../utils/LOG/log.h"

static void config_assign_processedint(configmodule_interface_t *cfg, paramdef_t *cfgoption, int val)
{
  cfgoption->processedvalue = config_allocate_new(cfg, sizeof(int), !(cfgoption->paramflags & PARAMFLAG_NOFREE));

  if (  cfgoption->processedvalue != NULL) {
    *(cfgoption->processedvalue) = val;
  } else {
    CONFIG_PRINTF_ERROR("[CONFIG] %s %d malloc error\n",__FILE__, __LINE__);
  }
}

int config_get_processedint(configmodule_interface_t *cfg, paramdef_t *cfgoption)
{
  int ret;

  if (  cfgoption->processedvalue != NULL) {
    ret=*(cfgoption->processedvalue);
    free( cfgoption->processedvalue);
    cfgoption->processedvalue=NULL;
    printf_params(cfg, "[CONFIG] %s:  set from %s to %i\n", cfgoption->optname, *cfgoption->strptr, ret);
  } else {
    fprintf (stderr,"[CONFIG] %s %d %s has no processed integer availablle\n",__FILE__, __LINE__, cfgoption->optname);
    ret=0;
  }

  return ret;
}
void config_printhelp(paramdef_t *params,int numparams, const char *prefix) {
  printf("\n-----Help for section %-26s: %03i entries------\n",(prefix==NULL)?"(root section)":prefix,numparams);

  for (int i=0 ; i<numparams ; i++) {
    printf("    %s%s: %s",
           (strlen(params[i].optname) <= 1) ? "-" : "--",
           params[i].optname,
           (params[i].helpstr != NULL)?params[i].helpstr:"Help string not specified\n");
  }   /* for on params entries */

  printf("--------------------------------------------------------------------\n\n");
}

int config_execcheck(configmodule_interface_t *cfg, paramdef_t *params, int numparams, const char *prefix)
{
  int st=0;

  for (int i=0 ; i<numparams ; i++) {
    if ( params[i].chkPptr == NULL) {
      continue;
    }

    if (params[i].chkPptr->s4.f4 != NULL) {
      st += params[i].chkPptr->s4.f4(cfg, &(params[i]));
    }
  }

  if (st != 0) {
    CONFIG_PRINTF_ERROR("[CONFIG] config_execcheck: section %s %i parameters with wrong value\n", prefix, -st);
  }

  return st;
}

int config_paramidx_fromname(const paramdef_t *params, int numparams, const char *name)
{
  for (int i = 0; i < numparams; i++) {
    if (strcmp(name, params[i].optname) == 0)
      return i;
  }

  fprintf(stderr, "[CONFIG]config_paramidx_fromname , %s is not a valid parameter name\n", name);
  return -1;
}

int config_get(configmodule_interface_t *cfgif, paramdef_t *params, int numparams, const char *prefix)
{
  int ret= -1;

  if (CONFIG_ISFLAGSET(CONFIG_ABORT)) {
    fprintf(stderr,"[CONFIG] config_get, section %s skipped, config module not properly initialized\n",prefix);
    return ret;
  }

  if (cfgif != NULL) {
    ret = cfgif->get(cfgif, params, numparams, prefix);

    if (ret >= 0) {
      config_process_cmdline(cfgif, params, numparams, prefix);
      if (cfgif->rtflags & CONFIG_SAVERUNCFG) {
        cfgif->set(params, numparams, prefix);
      }
      config_execcheck(cfgif, params, numparams, prefix);
    }

    return ret;
  }

  return ret;
}

int config_getlist(configmodule_interface_t *cfg, paramlist_def_t *ParamList, paramdef_t *params, int numparams, const char *prefix)
{
  if (CONFIG_ISFLAGSET(CONFIG_ABORT)) {
    fprintf(stderr,"[CONFIG] config_get skipped, config module not properly initialized\n");
    return -1;
  }
  const int ret = cfg->getlist(cfg, ParamList, params, numparams, prefix);

  /* build newprefix OUTSIDE the params check so we can use it below too */
  char *newprefix = NULL;
  if (prefix) {
    int rc = asprintf(&newprefix, "%s.%s", prefix, ParamList->listname);
    if (rc < 0)
      newprefix = NULL;
  } else {
    newprefix = ParamList->listname;
  }

  char cfgpath[MAX_OPTNAME_SIZE * 2 + 6]; /* prefix.listname.[listindex] */

  if (ret >= 0 && params) {
    for (int i = 0; i < ParamList->numelt; ++i) {
      sprintf(cfgpath, "%s.[%i]", newprefix, i);
      config_process_cmdline(cfg, ParamList->paramarray[i], numparams, cfgpath);
      if (cfg->rtflags & CONFIG_SAVERUNCFG)
        cfg->set(ParamList->paramarray[i], numparams, cfgpath);
      config_execcheck(cfg, ParamList->paramarray[i], numparams, cfgpath);
    }
  }

  if (params && newprefix && (ret >= 0 || ParamList->numelt == 0)) {
    sprintf(cfgpath, "%s", newprefix);
    char searchstr[MAX_OPTNAME_SIZE * 2 + 10];
    snprintf(searchstr, sizeof(searchstr), "--%s.", cfgpath);
    char *endptr;
    int valid_idx = ParamList->numelt;

    for (int i = 1; i < cfg->argc; i++) {
      char *res = strstr(cfg->argv[i], searchstr);
      if (res != NULL) {
        char *bracket = strchr(res + strlen(searchstr), '[');
        if (bracket == NULL)
          continue;
        bracket++;
        long num = strtol(bracket, &endptr, 10);
        if (num < valid_idx)
          continue;
        if (valid_idx == num) {
          valid_idx++;
        } else if (num > valid_idx) {
          LOG_E(UTIL, "Out of Order Element Creation\n index: %s, valid_idx: %d, num: %ld\n", cfg->argv[i], valid_idx, num);
          return -1;
        } else {
          LOG_E(UTIL, "[CONFIG] Invalid Configuration\n index: %s, valid_idx: %d, num: %ld\n", cfg->argv[i], valid_idx, num);
          return -1;
        }
      }
    }

    while (ParamList->numelt < valid_idx) {
      int new_idx = ParamList->numelt;
      sprintf(cfgpath, "%s.[%i]", newprefix, new_idx);
      paramdef_t **old = ParamList->paramarray;
      ParamList->paramarray = config_allocate_new(cfg, (new_idx + 1) * sizeof(paramdef_t *), true);
      if (new_idx > 0 && old != NULL) {
        memcpy(ParamList->paramarray, old, new_idx * sizeof(paramdef_t *));
      }
      ParamList->paramarray[new_idx] = config_allocate_new(cfg, numparams * sizeof(paramdef_t), true);
      memcpy(ParamList->paramarray[new_idx], params, sizeof(paramdef_t) * numparams);

      for (int p = 0; p < numparams; p++) {
        ParamList->paramarray[new_idx][p].voidptr = NULL;
      }

      ParamList->numelt++;
      fprintf(stderr, "[CONFIG] Created new array parameter %s.[%d]\n", newprefix, new_idx);
      config_process_cmdline(cfg, ParamList->paramarray[new_idx], numparams, cfgpath);

      for (int p = 0; p < numparams; p++) {
        paramdef_t *pd = &ParamList->paramarray[new_idx][p];
        if (pd->paramflags & PARAMFLAG_PARAMSET)
          continue;
        config_common_getdefault(cfg, pd, cfgpath);
      }

      config_execcheck(cfg, ParamList->paramarray[new_idx], numparams, cfgpath);

      for (int p = 0; p < numparams; p++) {
        if (ParamList->paramarray[new_idx][p].paramflags & PARAMFLAG_PARAMSET)
          fprintf(stderr, "[CONFIG] New parameter set: %s\n", ParamList->paramarray[new_idx][p].optname);
      }
    }
  }

  if (prefix)
    free(newprefix);

  /* when added parameters via CLI, return numelt instead of original ret */
  return (ParamList->numelt > 0) ? (int)ParamList->numelt : ret;
}

int config_isparamset(paramdef_t *params, int paramidx)
{
  if ((params[paramidx].paramflags & PARAMFLAG_PARAMSET) != 0) {
    return 1;
  } else {
    return 0;
  }
}

void print_intvalueerror(paramdef_t *param, char *fname, int *okval, int numokval) {
  fprintf(stderr,"[CONFIG] %s: %s: %i invalid value, authorized values:\n       ",
          fname,param->optname, (int)*(param->uptr));

  for ( int i=0; i<numokval ; i++) {
    fprintf(stderr, " %i",okval[i]);
  }

  fprintf(stderr, " \n");
}

int config_check_intval(configmodule_interface_t *cfg, paramdef_t *param)
{
  UNUSED(cfg);
  if ( param == NULL ) {
    fprintf(stderr,"[CONFIG] config_check_intval: NULL param argument\n");
    return -1;
  }

  if (param->type == TYPE_INT32 || param->type == TYPE_INT) {
    if (param->iptr == NULL) {
      fprintf(stderr, "[CONFIG] config_check_intval: %s: NULL iptr\n", param->optname);
      return -1;
    }
    const int v = *param->iptr;
    for (int i = 0; i < param->chkPptr->s1.num_okintval; i++) {
      if (v == param->chkPptr->s1.okintval[i])
        return 0;
    }
    fprintf(stderr, "[CONFIG] config_check_intval: %s: %i invalid value, authorized values:\n       ", param->optname, v);
    for (int i = 0; i < param->chkPptr->s1.num_okintval; i++) {
      fprintf(stderr, " %i", param->chkPptr->s1.okintval[i]);
    }
    fprintf(stderr, " \n");
    return -1;
  } else {
    if (param->uptr == NULL) {
      fprintf(stderr, "[CONFIG] config_check_intval: %s: NULL uptr\n", param->optname);
      return -1;
    }
    for (int i = 0; i < param->chkPptr->s1.num_okintval; i++) {
      if (*(param->uptr) == (uint32_t)param->chkPptr->s1.okintval[i]) {
        return 0;
      }
    }
    print_intvalueerror(param, "config_check_intval", param->chkPptr->s1.okintval, param->chkPptr->s1.num_okintval);
    return -1;
  }
}

int config_check_modify_integer(configmodule_interface_t *cfg, paramdef_t *param)
{
  for (int i=0; i < param->chkPptr->s1a.num_okintval ; i++) {
    if (*(param->uptr) == param->chkPptr->s1a.okintval[i] ) {
      printf_params(cfg,
                    "[CONFIG] %s:  read value %i, set to %i\n",
                    param->optname,
                    *(param->uptr),
                    param->chkPptr->s1a.setintval[i]);
      *(param->uptr) = param->chkPptr->s1a.setintval [i];
      return 0;
    }
  }

  print_intvalueerror(param,"config_check_modify_integer", param->chkPptr->s1a.okintval,param->chkPptr->s1a.num_okintval);
  return -1;
}

int config_check_intrange(const configmodule_interface_t *cfg, const paramdef_t *param)
{
  UNUSED(cfg);
  if( *(param->iptr) >= param->chkPptr->s2.okintrange[0]  && *(param->iptr) <= param->chkPptr->s2.okintrange[1]  ) {
    return 0;
  }

  fprintf(stderr,
          "[CONFIG] config_check_intrange: %s: %i invalid value, authorized range: %i %i\n",
          param->optname,
          (int)*(param->iptr),
          param->chkPptr->s2.okintrange[0],
          param->chkPptr->s2.okintrange[1]);
  return -1;
}

int config_check_uintrange(const configmodule_interface_t *cfg, const paramdef_t *param)
{
  (void)cfg;
  const uint32_t *v = param->uptr;
  const int *range = param->chkPptr->s2.okintrange;

  if (*v >= (uint32_t)range[0] && *v <= (uint32_t)range[1]) {
    return 0;
  }

  fprintf(stderr,
          "[CONFIG] config_check_uintrange: %s: %u invalid, authorized range: %i %i\n",
          param->optname,
          *v,
          range[0],
          range[1]);
  return -1;
}

void print_strvalueerror(paramdef_t *param, char *fname, char **okval, int numokval) {
  fprintf(stderr,"[CONFIG] %s: %s: %s invalid value, authorized values:\n       ",
          fname,param->optname, *param->strptr);

  for ( int i=0; i<numokval ; i++) {
    fprintf(stderr, " %s",okval[i]);
  }

  fprintf(stderr, " \n");
}

int config_check_strval(configmodule_interface_t *cfg, paramdef_t *param)
{
  UNUSED(cfg);
  if ( param == NULL ) {
    fprintf(stderr,"[CONFIG] config_check_strval: NULL param argument\n");
    return -1;
  }

  for ( int i=0; i<param->chkPptr->s3.num_okstrval ; i++) {
    if( strcasecmp(*param->strptr,param->chkPptr->s3.okstrval[i] ) == 0) {
      return 0;
    }
  }

  print_strvalueerror(param, "config_check_strval", param->chkPptr->s3.okstrval, param->chkPptr->s3.num_okstrval);
  return -1;
}

int config_checkstr_assign_integer(configmodule_interface_t *cfg, paramdef_t *param)
{
  for (int i=0; i < param->chkPptr->s3a.num_okstrval ; i++) {
    if (strcasecmp(*param->strptr,param->chkPptr->s3a.okstrval[i]  ) == 0) {
      config_assign_processedint(cfg, param, param->chkPptr->s3a.setintval[i]);
      return 0;
    }
  }

  print_strvalueerror(param, "config_check_strval", param->chkPptr->s3a.okstrval, param->chkPptr->s3a.num_okstrval);
  return -1;
}

void config_set_checkfunctions(paramdef_t *params, checkedparam_t *checkfunctions, int numparams) {
  for (int i=0; i< numparams ; i++ ) {
    params[i].chkPptr = &(checkfunctions[i]);
  }
}

const paramdef_t *config_get_paramdef_from_name(const paramdef_t *pd, int num, const char *name)
{
  int idx = config_paramidx_fromname(pd, num, (char *)name);
  AssertFatal(idx >= 0 && idx < num, "Invalid parameter name %s\n", name);
  return &pd[idx];
}
