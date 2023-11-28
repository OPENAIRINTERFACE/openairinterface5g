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

/*! \file common/utils/websrv/websrv.c
 * \brief: implementation of web API
 * \author Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <libgen.h>
#include <jansson.h>
#include <ulfius.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include "common/config/config_userapi.h"
#include "common/utils/LOG/log.h"
#include "common/utils/websrv/websrv.h"
#include "executables/softmodem-common.h"
#define WEBSERVERCODE
#include "common/utils/telnetsrv/telnetsrv.h"
#include "common/utils/load_module_shlib.h"
#include <arpa/inet.h>

//#define WEBSRV_AUTH

static websrv_params_t websrvparams;
static telnetsrv_params_t *dummy_telnetsrv_params = NULL;
paramdef_t websrvoptions[] = {
    /*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    /*                                            configuration parameters for telnet utility                                                                                      */
    /*   optname                              helpstr                paramflags           XXXptr                               defXXXval               type                 numelt */
    /*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    {"listenaddr", "<listen ip address>\n", 0, uptr : &websrvparams.listenaddr, defstrval : "0.0.0.0", TYPE_IPV4ADDR, 0},
    {"listenport", "<local port>\n", 0, uptr : &(websrvparams.listenport), defuintval : 8090, TYPE_UINT, 0},
    {"priority", "<scheduling policy (0-99)\n", 0, iptr : &websrvparams.priority, defuintval : 0, TYPE_INT, 0},
    {"debug", "<debug level>\n", 0, uptr : &websrvparams.dbglvl, defuintval : 0, TYPE_UINT, 0},
    {"fpath", "<file directory>\n", 0, strptr : &websrvparams.fpath, defstrval : "websrv", TYPE_STRING, 0},
    {"cert", "<cert file>\n", 0, strptr : &websrvparams.certfile, defstrval : NULL, TYPE_STRING, 0},
    {"key", "<key file>\n", 0, strptr : &websrvparams.keyfile, defstrval : NULL, TYPE_STRING, 0},
    {"rootca", "<root ca file>\n", 0, strptr : &websrvparams.rootcafile, defstrval : NULL, TYPE_STRING, 0},
};

void register_module_endpoints(cmdparser_t *module);

void websrv_gettbldata_response(struct _u_response *response, webdatadef_t *wdata, char *module, char *cmdtitle);
int websrv_callback_get_softmodemcmd(const struct _u_request *request, struct _u_response *response, void *user_data);

/*--------------------------------------------------------------------------------------------------*/
/* format a json response from a result table returned from a call to a telnet server command       */
void websrv_gettbldata_response(struct _u_response *response, webdatadef_t *wdata, char *module, char *cmdtitle)
{
  json_t *jcols = json_array();
  json_t *jdata = json_array();
  char *coltype;
  for (int i = 0; i < wdata->numcols; i++) {
    if (wdata->columns[i].coltype & TELNET_CHECKVAL_BOOL)
      coltype = "boolean";
    else if (wdata->columns[i].coltype & TELNET_CHECKVAL_LOGLVL)
      coltype = "loglvl";
    else if (wdata->columns[i].coltype & TELNET_CHECKVAL_SIMALGO)
      coltype = "string"; //"simuTypes";
    else if (wdata->columns[i].coltype & TELNET_VARTYPE_STRING)
      coltype = "string";
    else
      coltype = "number";
    json_t *acol = json_pack("{s:s,s:s,s:b,s:b}",
                             "name",
                             wdata->columns[i].coltitle,
                             "type",
                             coltype,
                             "modifiable",
                             (wdata->columns[i].coltype & TELNET_CHECKVAL_RDONLY) ? 0 : 1,
                             "help",
                             websrv_utils_testhlp(websrvparams.fpath, module, cmdtitle, wdata->columns[i].coltitle));
    json_array_append_new(jcols, acol);
  }
  for (int i = 0; i < wdata->numlines; i++) {
    json_t *jval;
    json_t *jline = json_array();
    for (int j = 0; j < wdata->numcols; j++) {
      if (wdata->columns[j].coltype & TELNET_CHECKVAL_BOOL)
        jval = json_string(wdata->lines[i].val[j]);
      else if (wdata->columns[j].coltype & TELNET_VARTYPE_STRING)
        jval = json_string(wdata->lines[i].val[j]);
      //          else if (wdata->columns[j].coltype == TELNET_VARTYPE_DOUBLE)
      //            jval=json_real((double)(wdata->lines[i].val[j]));
      else
        jval = json_integer((long)(wdata->lines[i].val[j]));
      json_array_append_new(jline, jval);
    }
    json_array_append_new(jdata, jline);
  }
  json_t *jbody = json_pack("{s:[],s:{s:o,s:o}}", "display", "table", "columns", jcols, "rows", jdata);
  websrv_jbody(response, jbody, 200, websrvparams.dbglvl);
  telnetsrv_freetbldata(wdata);
}

int websrv_callback_auth(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] authenticating %s %s\n", request->http_verb, request->http_url);
  websrv_dump_request("authentication ", request, websrvparams.dbglvl);

#ifdef WEBSRV_AUTH
  char *dn = NULL, *issuer_dn = NULL;
  size_t lbuf = 0, libuf = 0;
  if (request->client_cert != NULL) {
    gnutls_x509_crt_get_dn(request->client_cert, NULL, &lbuf);
    gnutls_x509_crt_get_issuer_dn(request->client_cert, NULL, &libuf);
    dn = malloc(lbuf + 1);
    issuer_dn = malloc(libuf + 1);
    if (dn != NULL && issuer_dn != NULL) {
      gnutls_x509_crt_get_dn(request->client_cert, dn, &lbuf);
      gnutls_x509_crt_get_issuer_dn(request->client_cert, issuer_dn, &libuf);
      dn[lbuf] = '\0';
      issuer_dn[libuf] = '\0';
      LOG_I(UTIL, "[websrv] dn of the client: %s, dn of the issuer: %s \n", dn, issuer_dn);
    } else {
      LOG_I(UTIL, "[websrv] dn of the client: %s, dn of the issuer: %s \n", (dn == NULL) ? "null" : dn, (issuer_dn == NULL) ? "null" : issuer_dn);
      ulfius_set_string_body_response(response, 400, "Couldn't authenticate client");
    }
    free(dn);
    free(issuer_dn);
  } else {
    LOG_I(UTIL, "No client certificate\n");
    // ulfius_set_string_body_response(response, 400, "Invalid client certificate");
    return U_CALLBACK_CONTINUE;
  }
#endif
  //    return U_CALLBACK_ERROR;
  return U_CALLBACK_ERROR;
}
/*----------------------------------------------------------------------------------------------*/
/* callback to answer a request to download a file from  the softmodem (example: a config file  */
int websrv_callback_get_softmodemfile(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] %s received: %s %s\n", __FUNCTION__, request->http_verb, request->http_url);
  websrv_dump_request("get softmodem file ", request, websrvparams.dbglvl);
  if (request->map_post_body == NULL) {
    LOG_W(UTIL, "[websrv] No post parameters in: %s %s\n", request->http_verb, request->http_url);
    websrv_string_response("Invalid file request, no post parameters", response, 404, websrvparams.dbglvl);
  }
  char *fname = (char *)u_map_get(request->map_post_body, "fname");
  if (fname == NULL) {
    LOG_W(UTIL, "[websrv] fname post parameters not found: %s %s\n", request->http_verb, request->http_url);
    websrv_string_response("Invalid file request, no file name in post parameters", response, 404, websrvparams.dbglvl);
  }
  if (websrv_getfile(fname, response) == NULL) {
    LOG_W(UTIL, "[websrv] file %s not found\n", fname);
    websrv_string_response("file not found", response, 501, websrvparams.dbglvl);
  }

  return U_CALLBACK_COMPLETE;
}
/*------------------------------------------------------------------------------------------------------------------------*/
int websrv_callback_set_moduleparams(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  websrv_printf_start(response, 200, false);
  LOG_I(UTIL, "[websrv] %s received: %s %s\n", __FUNCTION__, request->http_verb, request->http_url);
  json_error_t jserr;
  json_t *jsbody = ulfius_get_json_body_request(request, &jserr);
  int httpstatus = 404;
  if (jsbody == NULL) {
    websrv_printf("cannot find json body in %s %s\n", request->http_url, jserr.text);
    httpstatus = 400;
  } else {
    websrv_printjson((char *)__FUNCTION__, jsbody, websrvparams.dbglvl);
    if (user_data == NULL) {
      httpstatus = 500;
      websrv_printf("%s: NULL user data", request->http_url);
    } else {
      cmdparser_t *modulestruct = (cmdparser_t *)user_data;
      int rawval;
      char *cmdName;
      json_t *parray = json_array();
      json_error_t jerror;
      int ures = json_unpack_ex(jsbody, &jerror, 0, "{s:i,s:s,s:o}", "rawIndex", &rawval, "cmdName", &cmdName, "params", &parray);
      if (ures != 0) {
        websrv_printf("cannot unpack json body from %s: %s \n", request->http_url, jerror.text);
      } else {
        int psize = json_array_size(parray);
        webdatadef_t datatbl;
        datatbl.numlines = rawval;
        datatbl.numcols = psize;
        LOG_I(UTIL, "[websrv] rawIndex=%i, cmdName=%s, params=array of %i table values\n", rawval, cmdName, psize);
        for (int i = 0; i < psize; i++) {
          json_t *jelem = json_array_get(parray, i);
          char *cvalue;
          char *cname;
          char *ctype;
          int cmod;
          ures = json_unpack_ex(jelem, &jerror, 0, "{s:s,s:{s:s,s:s,s:b}}", "value", &cvalue, "col", "name", &cname, "type", &ctype, "modifiable", &cmod);
          if (ures != 0) {
            websrv_printf("cannot unpack json element %i %s\n", i, jerror.text);
          } else {
            LOG_I(UTIL, "[websrv] element%i, value=%s, name %s type %s\n", i, cvalue, cname, ctype);
            snprintf(datatbl.columns[i].coltitle, TELNET_CMD_MAXSIZE - 1, "%s", cname);
            datatbl.columns[i].coltype = TELNET_VARTYPE_STRING;
            datatbl.lines[0].val[i] = cvalue;
          } // json_unpack_ex(jelem OK
        } // for i
        for (telnetshell_cmddef_t *cmd = modulestruct->cmd; cmd->cmdfunc != NULL; cmd++) {
          if (strcmp(cmd->cmdname, cmdName) == 0 && ((cmd->cmdflags & TELNETSRV_CMDFLAG_TELNETONLY) == 0)) {
            httpstatus = 200;
            char *pvalue = NULL;
            if (strncmp(cmdName, "show", 4) == 0) {
              sprintf(cmdName, "%s", "set");
              if (cmd->cmdflags & TELNETSRV_CMDFLAG_NEEDPARAM) {
                json_t *Jparam = json_object_get(jsbody, "param");
                if (Jparam != NULL) {
                  char *pname = NULL;
                  ures = json_unpack_ex(Jparam, &jerror, 0, "{s:s,s:s}", "name", &pname, "value", &pvalue);
                  if (ures != 0) {
                    websrv_printf("cannot unpack json param %s\n", jerror.text);
                  } else {
                    LOG_I(UTIL, "[websrv] parameter %s=%s\n", pname, pvalue);
                    snprintf(datatbl.tblname, sizeof(datatbl.tblname), "%s=%s", pname, pvalue);
                  }
                }
              }
              cmdName[3] = ' ';
            }
            cmd->webfunc_getdata(cmdName, websrvparams.dbglvl, &datatbl, websrv_printf);
            if (cmd->cmdflags & TELNETSRV_CMDFLAG_WEBSRV_SETRETURNTBL) {
              httpstatus = 1000;
              websrv_printf_end(httpstatus, websrvparams.dbglvl);
              websrv_gettbldata_response(response, &datatbl, modulestruct->module, cmd->cmdname);
              return U_CALLBACK_COMPLETE;
            }
            break;
          }
        } // for	*cmd
      } // json_unpack_ex(jsbody OK
    } // user_data
  } // jsbody not null
  websrv_printf_end(httpstatus, websrvparams.dbglvl);
  return U_CALLBACK_COMPLETE;
}
/*------------------------------------------------------------------------------------------------------------------------*/
/* callback to format a help response */
int websrv_callback_get_softmodemhelp(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] Requested file is: %s %s\n", request->http_verb, request->http_url);
  websrv_dump_request("help ", request, websrvparams.dbglvl);
  char *help_string = NULL;
  int httpstatus = 204; // no content
  char *hlpfile = strstr(request->http_url, "helpfiles");
  if (hlpfile != NULL) {
    char *hlppath = malloc(strlen(hlpfile) + strlen(websrvparams.fpath) + 1);
    sprintf(hlppath, "%s/%s", websrvparams.fpath, hlpfile);
    help_string = websrv_read_file(hlppath);
    if (help_string == NULL) {
      help_string = strdup("Help file not found");
    } else {
      httpstatus = 201; // created
    }
    free(hlppath);
  } else {
    help_string = strdup("Help request format error");
  }
  json_t *jhelp = json_pack("{s:s}", "text", help_string);
  websrv_jbody(response, jhelp, httpstatus, websrvparams.dbglvl);
  free(help_string);
  return U_CALLBACK_CONTINUE;
}

/* default callback tries to find a file in the web server repo (path exctracted from <websrvparams.url>) and if found streams it */
int websrv_callback_default(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] Requested file is: %s %s\n", request->http_verb, request->http_url);
  websrv_dump_request("default ", request, websrvparams.dbglvl);

  char *fpath = malloc(strlen(request->http_url) + strlen(websrvparams.fpath) + 2);
  if ((strcmp(request->http_url + 1, websrvparams.fpath) == 0) || (strcmp(request->http_url, "/") == 0)) {
    sprintf(fpath, "%s/index.html", websrvparams.fpath);
  } else {
    sprintf(fpath, "%s/%s", websrvparams.fpath, request->http_url);
  }
  FILE *f = websrv_getfile(fpath, response);
  free(fpath);
  if (f == NULL)
    return U_CALLBACK_ERROR;
  return U_CALLBACK_CONTINUE;
}
/* callback processing  url (<address>/oaisoftmodem/:module/variables or <address>/oaisoftmodem/:module/commands) */
/* is called at low priority to process requests from a module initialized after the web server started           */
int websrv_callback_newmodule(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] callback_newmodule received %s %s\n", request->http_verb, request->http_url);
  if (user_data == NULL) {
    ulfius_set_string_body_response(response, 500, "Cannot access oai softmodem data");
    LOG_E(UTIL, "[websrv] callback_newmodule: user-data is NULL");
    return U_CALLBACK_COMPLETE;
  }
  telnetsrv_params_t *telnetparams = (telnetsrv_params_t *)user_data;
  for (int i = 0; i < u_map_count(request->map_url); i++) {
    LOG_I(UTIL, "[websrv]   url element %i %s : %s\n", i, u_map_enum_keys(request->map_url)[i], u_map_enum_values(request->map_url)[i]);
    if (strcmp(u_map_enum_keys(request->map_url)[i], "module") == 0) {
      for (int j = 0; telnetparams->CmdParsers[j].cmd != NULL; j++) {
        /* found the module in the telnet server module array, it was likely not registered at init time */
        if (strcmp(telnetparams->CmdParsers[j].module, u_map_enum_values(request->map_url)[i]) == 0) {
          register_module_endpoints(&(telnetparams->CmdParsers[j]));
          websrv_callback_get_softmodemcmd(request, response, &(telnetparams->CmdParsers[j]));
          return U_CALLBACK_CONTINUE;
        }
      } /* for j */
    }
  } /* for i */
  ulfius_set_string_body_response(response, 500, "Request for an unknown module");
  return U_CALLBACK_COMPLETE;
}

/* callback processing  url (<address>/oaisoftmodem/module/variables or <address>/oaisoftmodem/module/commands), options method */
int websrv_callback_okset_softmodem_cmdvar(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] : callback_okset_softmodem_cmdvar received %s %s\n", request->http_verb, request->http_url);
  for (int i = 0; i < u_map_count(request->map_header); i++)
    LOG_I(UTIL, "[websrv] header variable %i %s : %s\n", i, u_map_enum_keys(request->map_header)[i], u_map_enum_values(request->map_header)[i]);
  int us = ulfius_add_header_to_response(response, "Access-Control-Request-Method", "POST");
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set response header type ulfius error %d \n", us);
  }
  us = ulfius_add_header_to_response(response, "Access-Control-Allow-Headers", "content-type");
  us = ulfius_set_empty_body_response(response, 200);
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_empty_body_response)");
    LOG_E(UTIL, "[websrv] cannot set empty body response ulfius error %d \n", us);
  }
  return U_CALLBACK_COMPLETE;
}
int websrv_callback_set_softmodemvar(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] : callback_set_softmodemvar received %s %s\n", request->http_verb, request->http_url);
  websrv_printf_start(response, 256, false);
  json_error_t jserr;
  json_t *jsbody = ulfius_get_json_body_request(request, &jserr);
  int httpstatus = 404;
  if (jsbody == NULL) {
    websrv_printf("cannot find json body in %s %s\n", request->http_url, jserr.text);
    httpstatus = 400;
  } else {
    websrv_printjson("callback_set_softmodemvar: ", jsbody, websrvparams.dbglvl);
    if (user_data == NULL) {
      httpstatus = 500;
      websrv_printf("%s: NULL user data", request->http_url);
    } else {
      cmdparser_t *modulestruct = (cmdparser_t *)user_data;
      json_t *J = json_object_get(jsbody, "name");
      const char *vname = json_string_value(J);
      for (telnetshell_vardef_t *var = modulestruct->var; var->varvalptr != NULL; var++) {
        if (strncmp(var->varname, vname, TELNET_CMD_MAXSIZE) == 0) {
          J = json_object_get(jsbody, "value");
          if (J != NULL) {
            if (json_is_string(J)) {
              const char *vval = json_string_value(J);
              websrv_printf("var %s set to ", var->varname);
              int st = telnet_setvarvalue(var, (char *)vval, websrv_printf);
              if (st >= 0) {
                httpstatus = 200;
              } else {
                httpstatus = 500;
              }
            } else if (json_is_integer(J)) {
              json_int_t i = json_integer_value(J);
              switch (var->vartype) {
                case TELNET_VARTYPE_INT64:
                  *(int64_t *)var->varvalptr = (int64_t)i;
                  break;
                case TELNET_VARTYPE_INT32:
                  *(int32_t *)var->varvalptr = (int32_t)i;
                  break;
                case TELNET_VARTYPE_INT16:
                  *(int16_t *)var->varvalptr = (int16_t)i;
                  break;
                case TELNET_VARTYPE_INT8:
                  *(int8_t *)var->varvalptr = (int8_t)i;
                  break;
                case TELNET_VARTYPE_UINT:
                  *(unsigned int *)var->varvalptr = (unsigned int)i;
                  break;
                default:
                  httpstatus = 500;
                  websrv_printf(" Cannot set var %s, integer type mismatch\n", vname);
                  break;
              }
            } else if (json_is_real(J)) {
              double lf = json_real_value(J);
              if (var->vartype == TELNET_VARTYPE_DOUBLE) {
                *(double *)var->varvalptr = lf;
                httpstatus = 200;
                websrv_printf(" Var %s, set to %g\n", vname, *(double *)var->varvalptr);
              } else {
                httpstatus = 500;
                websrv_printf(" Cannot set var %s, real type mismatch\n", vname);
              }
            }
          } else {
            httpstatus = 500;
            websrv_printf("Cannot set var %s, json object is NULL\n", vname);
          }
          break;
        }
      } // for
    } // user_data
  } // sbody
  websrv_printf_end(httpstatus, websrvparams.dbglvl);
  return U_CALLBACK_COMPLETE;
}
/* callback processing module url (<address>/oaisoftmodem/module/commands), post method */
int websrv_processwebfunc(struct _u_response *response, cmdparser_t *modulestruct, telnetshell_cmddef_t *cmd, json_t *jparams)
{
  LOG_I(UTIL, "[websrv] : executing command %s %s\n", modulestruct->module, cmd->cmdname);
  if ((cmd->cmdflags & TELNETSRV_CMDFLAG_NEEDPARAM) && jparams == NULL) {
	  LOG_W(UTIL, "No parameters sent by frontend for %s %s\n", modulestruct->module, cmd->cmdname);
	  return 500;
  }
  int http_status = 200;
  char *pname[2], *pvalue[2];
  size_t np =0;
  if (jparams != NULL) {
     int b[2];
     char *ptype[2];
     json_error_t jerror;
     np = json_array_size(jparams);
     int jrt;
     switch(np) {
	   case 1:
	     jrt=json_unpack_ex(jparams, &jerror, 0, "[{s:s,s:s,s:s,s,b}]", "name", &pname[0], "value", &pvalue[0], "type", &ptype[0], "modifiable", &b);
	   break;
	   case 2:
	     jrt=json_unpack_ex(jparams, &jerror, 0, "[{s:s,s:s,s:s,s,b},{s:s,s:s,s:s,s,b}]", 
	                                          "name", &pname[0], "value", &pvalue[0], "type", &ptype[0], "modifiable", &b[0],
	                                          "name", &pname[1], "value", &pvalue[1], "type", &ptype[1], "modifiable", &b[1]);	   
	   break;
	   default:
	     http_status=500;	   
	   break;
//     json_unpack_ex(jparams, &jerror, 0, "[{s:s,s:s,s:s,s,b}]", "name", &pname, "value", &pvalue, "type", &ptype, "modifiable", &b);

	  }
    if (jrt <0 || http_status != 200) {
         LOG_I(UTIL, "[websrv], couldn't unpack jparams, module %s, command %s: %s\n", modulestruct->module, cmd->cmdname, jerror.text);
         websrv_printjson((char *)__FUNCTION__, jparams, websrvparams.dbglvl);
         return 500;  
    }	  
  }

  if (cmd->cmdflags & TELNETSRV_CMDFLAG_GETWEBTBLDATA) {
    webdatadef_t wdata;
    memset(&wdata, 0, sizeof(wdata));
    wdata.numlines = 1;
    for (int i=0; i<np; i++) {
      snprintf(wdata.columns[i].coltitle, sizeof(wdata.columns[i].coltitle) - 1, "%s", pname[i]);
      wdata.numcols = np;
      wdata.lines[0].val[i] = pvalue[i];
    }
    cmd->webfunc_getdata(cmd->cmdname, websrvparams.dbglvl, (webdatadef_t *)&wdata, NULL);
    websrv_gettbldata_response(response, &wdata, modulestruct->module, cmd->cmdname);
  } else {
    char *sptr = index(cmd->cmdname, ' ');
    char cmdbuff[TELNET_CMD_MAXSIZE*3]; //cmd + 2 parameters
    snprintf(cmdbuff,sizeof(cmdbuff)-1, "%s%s%s %s",(sptr == NULL) ? "" : sptr,(sptr == NULL) ? "" : " ",(np>0) ? pvalue[0] : "",(np>1) ? pvalue[1] : "");
    if (cmd->qptr != NULL) {
      websrv_printf_start(response, 16384, true);
      telnet_pushcmd(cmd, cmdbuff, websrv_async_printf);
    } else {
      websrv_printf_start(response, 16384, false);
      cmd->cmdfunc(cmdbuff, websrvparams.dbglvl, websrv_printf);
    }
    websrv_printf_end(http_status, websrvparams.dbglvl);
  }
  return http_status;
}

int websrv_callback_exec_softmodemcmd(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] : callback_exec_softmodemcmd received %s %s\n", request->http_verb, request->http_url);
  if (user_data == NULL) {
    ulfius_set_string_body_response(response, 500, "Cannot access oai softmodem data");
    LOG_E(UTIL, "[websrv] callback_exec_softmodemcmd: user-data is NULL");
    return U_CALLBACK_COMPLETE;
  }
  cmdparser_t *modulestruct = (cmdparser_t *)user_data;
  json_t *jsbody = ulfius_get_json_body_request(request, NULL);
  int httpstatus = 404;
  char *msg = "";
  if (jsbody == NULL) {
    msg = "Unproperly formatted request body";
    httpstatus = 400;
  } else {
    websrv_printjson("callback_exec_softmodemcmd: ", jsbody, websrvparams.dbglvl);
    json_t *J = json_object_get(jsbody, "name");
    const char *vname = json_string_value(J);
    if (vname == NULL) {
      msg = "No command name in request body";
      LOG_E(UTIL, "[websrv] command name not found in body\n");
      httpstatus = 400;
    } else {
      httpstatus = 501;
      msg = "Unknown command in request body";
      for (telnetshell_cmddef_t *cmd = modulestruct->cmd; cmd->cmdfunc != NULL; cmd++) {
        if (strcmp(cmd->cmdname, vname) == 0 && ((cmd->cmdflags & TELNETSRV_CMDFLAG_TELNETONLY) == 0)) {
          J = json_object_get(jsbody, "param");
          httpstatus = websrv_processwebfunc(response, modulestruct, cmd, J);
          break;
        }
      } // for
    }
  } // sbody
  if (httpstatus >= 300)
    ulfius_set_string_body_response(response, httpstatus, msg);
  return U_CALLBACK_COMPLETE;
}
/* callback processing module url (<address>/oaisoftmodem/module/variables), get method*/
int websrv_callback_get_softmodemvar(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] : callback_get_softmodemvar received %s %s  module %s\n", request->http_verb, request->http_url, (user_data == NULL) ? "NULL" : ((cmdparser_t *)user_data)->module);
  if (user_data == NULL) {
    ulfius_set_string_body_response(response, 500, "No variables defined for this module");
    return U_CALLBACK_COMPLETE;
  }
  cmdparser_t *modulestruct = (cmdparser_t *)user_data;
  LOG_I(UTIL, "[websrv] received  %s variables request\n", modulestruct->module);
  json_t *modulevars = json_array();

  for (int j = 0; modulestruct->var[j].varvalptr != NULL; j++) {
    char *strval = telnet_getvarvalue(modulestruct->var, j);
    int modifiable = 1;
    if (modulestruct->var[j].checkval & TELNET_CHECKVAL_RDONLY)
      modifiable = 0;
    json_t *oneaction;
    switch (modulestruct->var[j].vartype) {
      case TELNET_VARTYPE_DOUBLE:
        oneaction = json_pack("{s:s,s:s,s:g,s:b}", "type", "number", "name", modulestruct->var[j].varname, "value", *(double *)(modulestruct->var[j].varvalptr), "modifiable", modifiable);
      case TELNET_VARTYPE_INT32:
      case TELNET_VARTYPE_INT16:
      case TELNET_VARTYPE_INT8:
      case TELNET_VARTYPE_UINT:
        oneaction = json_pack("{s:s,s:s,s:i,s:b}", "type", "number", "name", modulestruct->var[j].varname, "value", (int)(*(int *)(modulestruct->var[j].varvalptr)), "modifiable", modifiable);
        break;
      case TELNET_VARTYPE_INT64:
        oneaction =
            json_pack("{s:s,s:s,s:lli,s:b}", "type", "number", "name", modulestruct->var[j].varname, "value", (int64_t)(*(int64_t *)(modulestruct->var[j].varvalptr)), "modifiable", modifiable);
        break;
      case TELNET_VARTYPE_STRING:
        oneaction = json_pack("{s:s,s:s,s:s,s:b}", "type", "string", "name", modulestruct->var[j].varname, "value", strval, "modifiable", modifiable);
        break;
      default:
        oneaction = json_pack("{s:s,s:s,s:s,s:b}", "type", "???", "name", modulestruct->var[j].varname, "value", "???", "modifiable", modifiable);
        break;
    }
    if (oneaction == NULL) {
      LOG_E(UTIL, "[websrv] cannot encode oneaction %s/%s\n", modulestruct->module, modulestruct->var[j].varname);
    } else {
      websrv_printjson("oneaction", oneaction, websrvparams.dbglvl);
    }
    free(strval);
    json_array_append_new(modulevars, oneaction);
  }
  if (json_array_size(modulevars) == 0) {
    LOG_I(UTIL, "[websrv] no defined variables for %s\n", modulestruct->module);
  } else {
    websrv_printjson("modulevars", modulevars, websrvparams.dbglvl);
  }

  int us = ulfius_add_header_to_response(response, "content-type", "application/json");
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 500, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set response header type ulfius error %d \n", us);
  }
  us = ulfius_set_json_body_response(response, 200, modulevars);
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 500, "Internal server error (ulfius_set_json_body_response)");
    LOG_E(UTIL, "[websrv] cannot set body response ulfius error %d \n", us);
  }
  return U_CALLBACK_COMPLETE;
}

/* callback processing module url (<address>/oaisoftmodem/module/commands)*/
int websrv_callback_get_softmodemcmd(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  LOG_I(UTIL, "[websrv] : callback_get_softmodemcmd received %s %s  module %s\n", request->http_verb, request->http_url, (user_data == NULL) ? "NULL" : ((cmdparser_t *)user_data)->module);
  if (user_data == NULL) {
    ulfius_set_string_body_response(response, 500, "No commands defined for this module");
    return U_CALLBACK_COMPLETE;
  }
  cmdparser_t *modulestruct = (cmdparser_t *)user_data;

  LOG_I(UTIL, "[websrv] received  %s commands request\n", modulestruct->module);
  json_t *modulesubcom = json_array();
  for (int j = 0; modulestruct->cmd[j].cmdfunc != NULL; j++) {
    if (strcasecmp("help", modulestruct->cmd[j].cmdname) == 0 || (modulestruct->cmd[j].cmdflags & TELNETSRV_CMDFLAG_TELNETONLY)) {
      continue;
    }
    json_t *acmd;
    if (modulestruct->cmd[j].cmdflags & TELNETSRV_CMDFLAG_CONFEXEC) {
      char confstr[256];
      snprintf(confstr, sizeof(confstr), "Confirm %s ?", modulestruct->cmd[j].cmdname);
      acmd = json_pack("{s:s,s:s}", "name", modulestruct->cmd[j].cmdname, "confirm", confstr);
    } else if (modulestruct->cmd[j].cmdflags & TELNETSRV_CMDFLAG_NEEDPARAM) {
      char *question[] = {NULL,NULL};
      char *helpcp = NULL;
      json_t *jQ1=NULL, *jQ2=NULL;
      json_t *jQs = json_array();
      if (modulestruct->cmd[j].helpstr != NULL) {
        helpcp = strdup(modulestruct->cmd[j].helpstr);
        int ns=sscanf(helpcp,"<%m[^<>]> <%m[^<>]>",&question[0],&question[1]);
        if (ns == 0) {
		  LOG_W(UTIL, "[websrv] Cannot find parameters for command %s %s\n", modulestruct->module, modulestruct->cmd[j].cmdname);
		  continue;		
		}  
        jQ1=json_pack("{s:s,s:s,s:s}", "display",question[0], "pname", "P0", "type", "string");
        json_array_append_new(jQs, jQ1);
        if (ns >1) {
            jQ2=json_pack("{s:s,s:s,s:s}","display", (question[1] == NULL) ? "" : question[1], "pname",  "P1" , "type", "string");
            json_array_append_new(jQs, jQ2);
	    }
      }
      acmd = json_pack("{s:s,s:o}", "name", modulestruct->cmd[j].cmdname, "question", jQs);
      free(helpcp);
      free(question[0]);
      free(question[1]);
    } else {
      acmd = json_pack("{s:s}", "name", modulestruct->cmd[j].cmdname);
    }
    if ( acmd == NULL) {
		LOG_W(UTIL, "[websrv] interface for command %s %s cannot be built\n", modulestruct->module, modulestruct->cmd[j].cmdname);
		continue;
	}
    json_t *jopts = json_array();
    if (modulestruct->cmd[j].cmdflags & TELNETSRV_CMDFLAG_AUTOUPDATE) {
      json_array_append_new(jopts, json_string("update"));
    }
    if (websrv_utils_testhlp(websrvparams.fpath, "cmd", modulestruct->module, modulestruct->cmd[j].cmdname)) {
      json_array_append_new(jopts, json_string("help"));
    }
    if (json_array_size(jopts) > 0) {
      json_t *cmdoptions = json_pack("{s:o}", "options", jopts);
      json_object_update_missing(acmd, cmdoptions);
    }
    json_array_append_new(modulesubcom, acmd);
  }
  if (modulesubcom == NULL) {
    LOG_E(UTIL, "[websrv] cannot encode modulesubcom response for %s\n", modulestruct->module);
  } else {
    websrv_printjson("modulesubcom", modulesubcom, websrvparams.dbglvl);
  }
  int us = ulfius_add_header_to_response(response, "content-type", "application/json");
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 500, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set response header type ulfius error %d \n", us);
  }
  us = ulfius_set_json_body_response(response, 200, modulesubcom);
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 500, "Internal server error (ulfius_set_json_body_response)");
    LOG_E(UTIL, "[websrv] cannot set body response ulfius error %d \n", us);
  }
  return U_CALLBACK_COMPLETE;
}

int websrv_callback_get_softmodemmodules(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  telnetsrv_params_t *telnetparams = (telnetsrv_params_t *)websrvparams.telnetparams;

  json_t *cmdnames = json_array();
  for (int i = 0; telnetparams->CmdParsers[i].var != NULL && telnetparams->CmdParsers[i].cmd != NULL; i++) {
    json_t *acmd = json_pack("{s:s}", "name", telnetparams->CmdParsers[i].module);
    json_array_append_new(cmdnames, acmd);
  }

  int us = ulfius_add_header_to_response(response, "content-type", "application/json");
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set modules response header type ulfius error %d \n", us);
  }

  us = ulfius_set_json_body_response(response, 200, cmdnames);
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
    LOG_E(UTIL, "[websrv] cannot set modules body response ulfius error %d \n", us);
  } else {
    websrv_printjson("cmdnames", cmdnames, websrvparams.dbglvl);
  }
  //  ulfius_set_string_body_response(response, 200, cfgfile);
  return U_CALLBACK_COMPLETE;
}
/*---------------------------------------------------------------------------*/
/* callback processing first request (<address>/oaisoftmodem) after initial connection to server */

/* utility to add a line of info in status panel  */
void websrv_add_modeminfo(json_t *modemvars, char *infoname, char *infoval, char *strtype)
{
  json_t *info;
  int modifiable = 0;
  if (strcmp(strtype, "configfile") == 0)
    modifiable = 1;

  info = json_pack("{s:s,s:s,s:s,s:b}", "name", infoname, "value", infoval, "type", strtype, "modifiable", modifiable);

  if (info == NULL) {
    LOG_E(UTIL, "[websrv] cannot encode modem info %s response\n", infoname);
  } else {
    websrv_printjson("status body1", info, websrvparams.dbglvl);
  }
  json_array_append_new(modemvars, info);
}

int websrv_callback_get_softmodemstatus(const struct _u_request *request, struct _u_response *response, void *user_data)
{
  char ipstr[INET_ADDRSTRLEN];
  char srvinfo[255];
  if (websrvparams.instance.bind_address != NULL)
    inet_ntop(AF_INET, &(websrvparams.instance.bind_address->sin_addr), ipstr, INET_ADDRSTRLEN);
  else
    sprintf(ipstr, "%s", "0.0.0.0");
  snprintf(srvinfo, sizeof(srvinfo) - 1, "%s:%hu %s", ipstr, websrvparams.instance.port, get_softmodem_function(NULL));
  json_t *modemvars = json_array();
  websrv_add_modeminfo(modemvars, "connected to", srvinfo, "string");
  websrv_add_modeminfo(modemvars, "config_file", CONFIG_GETCONFFILE, "configfile");
  websrv_add_modeminfo(modemvars, "exec name", config_get_if()->argv[0], "string");
  int len = 0;
  int argc = 0;
  for (int i = 1; config_get_if()->argv[i] != NULL; i++) {
    len += strlen(config_get_if()->argv[i]);
    argc++;
  }
  char *cmdline = malloc(len + argc + 10);
  char *ptr = cmdline;
  for (int i = 1; config_get_if()->argv[i] != NULL; i++) {
    ptr += sprintf(ptr, "%s ", config_get_if()->argv[i]);
  }
  websrv_add_modeminfo(modemvars, "command line", cmdline, "string");
  if ((config_get_if()->rtflags & CONFIG_SAVERUNCFG) && (config_get_if()->write_parsedcfg != NULL)) {
    config_get_if()->write_parsedcfg(config_get_if());

    websrv_add_modeminfo(modemvars, "Parsed config file", config_get_if()->status->debug_cfgname, "configfile");
  }

  int us = ulfius_add_header_to_response(response, "content-type", "application/json");
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set status response header type ulfius error %d \n", us);
  }

  us = ulfius_set_json_body_response(response, 200, modemvars);
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
    LOG_E(UTIL, "[websrv] cannot set status body response ulfius error %d \n", us);
  }
  //  ulfius_set_string_body_response(response, 200, cfgfile);
  return U_CALLBACK_COMPLETE;
}

int websrv_add_endpoint(char **http_method,
                        int num_method,
                        const char *url_prefix,
                        const char *url_format,
                        int (*callback_function[])(const struct _u_request *request, struct _u_response *response, void *user_data),
                        void *user_data)
{
  int status;
  int j = 0;
  int priority = (user_data == NULL) ? 10 : 1;
  for (int i = 0; i < num_method; i++) {
    status = ulfius_add_endpoint_by_val(&(websrvparams.instance), http_method[i], url_prefix, url_format, priority, callback_function[i], user_data);
    if (status != U_OK) {
      LOG_E(UTIL, "[websrv] cannot add endpoint %s %s/%s\n", http_method[i], url_prefix, url_format);
    } else {
      j++;
      LOG_I(UTIL, "[websrv] endpoint %s %s/%s added\n", http_method[i], url_prefix, url_format);
    }
  }
  return j;
}
/* add endpoints for a module, as defined in cmdparser_t telnet server structure */
void register_module_endpoints(cmdparser_t *module)
{
  int (*callback_functions_var[3])(const struct _u_request *request, struct _u_response *response, void *user_data) = {
      websrv_callback_okset_softmodem_cmdvar, websrv_callback_set_softmodemvar, websrv_callback_get_softmodemvar};
  char *http_methods[3] = {"OPTIONS", "POST", "GET"};

  int (*callback_functions_cmd[3])(const struct _u_request *request, struct _u_response *response, void *user_data) = {
      websrv_callback_okset_softmodem_cmdvar, websrv_callback_exec_softmodemcmd, websrv_callback_get_softmodemcmd};
  char prefixurl[TELNET_CMD_MAXSIZE + 20];
  snprintf(prefixurl, TELNET_CMD_MAXSIZE + 19, "oaisoftmodem/%s", module->module);
  LOG_I(UTIL, "[websrv] add endpoints %s/[variables or commands] \n", prefixurl);

  websrv_add_endpoint(http_methods, 3, prefixurl, "commands", callback_functions_cmd, module);
  websrv_add_endpoint(http_methods, 3, prefixurl, "variables", callback_functions_var, module);

  callback_functions_cmd[0] = websrv_callback_okset_softmodem_cmdvar;
  callback_functions_cmd[1] = websrv_callback_set_moduleparams;
  websrv_add_endpoint(http_methods, 2, prefixurl, "set", callback_functions_cmd, module);
}

void *websrv_autoinit()
{
  int ret;
  memset(&websrvparams, 0, sizeof(websrvparams));
  config_get(config_get_if(), websrvoptions, sizeofArray(websrvoptions), "websrv");
  /* check if telnet server is loaded or not */
  add_telnetcmd_func_t addcmd = (add_telnetcmd_func_t)get_shlibmodule_fptr("telnetsrv", TELNET_ADDCMD_FNAME);
  if (addcmd != NULL) {
    websrvparams.telnetparams = get_telnetsrv_params();
  } else {
    LOG_I(UTIL, "[websrv], running without the telnet server\n");
    dummy_telnetsrv_params = calloc(1, sizeof(telnetsrv_params_t));
    websrvparams.telnetparams = dummy_telnetsrv_params;
  }
  telnetsrv_params_t *telnetparams = (telnetsrv_params_t *)websrvparams.telnetparams;

  if (ulfius_init_instance(&(websrvparams.instance), websrvparams.listenport, NULL, NULL) != U_OK) {
    LOG_W(UTIL, "[websrv] Error,cannot init websrv\n");
    return (NULL);
  }

  u_map_put(websrvparams.instance.default_headers, "Access-Control-Allow-Origin", "*");

  // Maximum body size sent by the client is 1 Kb
  websrvparams.instance.max_post_body_size = 1024;

  // 1: build the first page, when receiving the "oaisoftmodem" url
  ulfius_add_endpoint_by_val(&(websrvparams.instance), "GET", "oaisoftmodem", "commands", 1, &websrv_callback_get_softmodemmodules, NULL);

  // 2 default_endpoint declaration, it tries to open the file with the url name as specified in the request.It looks for the file
  ulfius_set_default_endpoint(&(websrvparams.instance), &websrv_callback_default, NULL);

  // 3 endpoints corresponding to loaded telnet modules
  int (*callback_functions_var[3])(const struct _u_request *request, struct _u_response *response, void *user_data) = {
      websrv_callback_get_softmodemstatus, websrv_callback_okset_softmodem_cmdvar, websrv_callback_set_softmodemvar};
  char *http_methods[3] = {"GET", "OPTIONS", "POST"};

  websrv_add_endpoint(http_methods, 3, "oaisoftmodem", "info", callback_functions_var, NULL);

  for (int i = 0; telnetparams->CmdParsers[i].cmd != NULL; i++) {
    register_module_endpoints(&(telnetparams->CmdParsers[i]));
  }

  /*4 callbacks to take care of modules not yet initialized, so not visible in telnet server data when this autoinit runs: */
  ulfius_add_endpoint_by_val(&(websrvparams.instance), "GET", "oaisoftmodem", "@module/commands", 10, websrv_callback_newmodule, telnetparams);
  ulfius_add_endpoint_by_val(&(websrvparams.instance), "GET", "oaisoftmodem", "@module/variables", 10, websrv_callback_newmodule, telnetparams);
  // 5 callback to handle file request
  ulfius_add_endpoint_by_val(&(websrvparams.instance), "POST", "oaisoftmodem", "file", 1, &websrv_callback_get_softmodemfile, NULL);
  // 6 callback for help request
  ulfius_add_endpoint_by_val(&(websrvparams.instance), "GET", "oaisoftmodem", "helpfiles/@hlpfile", 2, &websrv_callback_get_softmodemhelp, NULL);
  // init softscope interface support */
  websrv_init_scope(&websrvparams);

  // init websocket */
  websrv_init_websocket(&websrvparams, "softscope");

  // build some html files in the server repo
  websrv_utils_build_hlpfiles(websrvparams.fpath);

  // Start the framework
  ret = U_ERROR;
  if (websrvparams.keyfile != NULL && websrvparams.certfile != NULL) {
    char *key_pem = websrv_read_file(websrvparams.keyfile);
    char *cert_pem = websrv_read_file(websrvparams.certfile);
    char *root_ca_pem = websrv_read_file(websrvparams.rootcafile);
    if (key_pem != NULL && cert_pem != NULL) {
      ulfius_add_endpoint_by_val(&(websrvparams.instance), "*", "", "@anyword", 0, &websrv_callback_auth, NULL);
      LOG_I(UTIL, "[websrv] priority 0 authentication endpoint added/n");
#ifdef WEBSRV_AUTH
      if (root_ca_pem == NULL)
        ret = ulfius_start_secure_framework(&(websrvparams.instance), key_pem, cert_pem);
      else
        ret = ulfius_start_secure_ca_trust_framework(&(websrvparams.instance), key_pem, cert_pem, root_ca_pem);
#else
      ret = ulfius_start_framework(&(websrvparams.instance));
#endif
      free(key_pem);
      free(cert_pem);
      free(root_ca_pem);
    } else {
      LOG_E(UTIL, "[websrv] Unable to load key %s and cert %s\n", websrvparams.keyfile, websrvparams.certfile);
    }
  } else {
    ret = ulfius_start_framework(&(websrvparams.instance));
  }

  if (ret == U_OK) {
    LOG_I(UTIL, "[websrv] Web server started on port %d", websrvparams.instance.port);
  } else {
    LOG_W(UTIL, "[websrv] Error starting web server on port %d\n", websrvparams.instance.port);
  }
  return &(websrvparams.instance);
}

void websrv_end(void *webinst)
{
  ulfius_stop_framework((struct _u_instance *)webinst);
  ulfius_clean_instance((struct _u_instance *)webinst);
  free(dummy_telnetsrv_params);
  return;
}
