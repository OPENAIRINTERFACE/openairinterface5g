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
 #include "common/config/config_userapi.h"
 #include "common/utils/LOG/log.h"
 #include "common/utils/websrv/websrv.h"
 #include "executables/softmodem-common.h"
 #define WEBSERVERCODE
 #include "common/utils/telnetsrv/telnetsrv.h"
 
 
 static websrv_params_t websrvparams;
 paramdef_t websrvoptions[] = {
  /*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
  /*                                            configuration parameters for telnet utility                                                                                      */
  /*   optname                              helpstr                paramflags           XXXptr                               defXXXval               type                 numelt */
  /*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
  {"listenaddr",                   "<listen ip address>\n",         0,                 uptr:&websrvparams.listenaddr,        defstrval:"0.0.0.0",            TYPE_IPV4ADDR,  0 },
  {"listenport",                   "<local port>\n",                0,                 uptr:&(websrvparams.listenport),      defuintval:8090,                TYPE_UINT,      0 },
  {"priority",                     "<scheduling policy (0-99)\n",   0,                 iptr:&websrvparams.priority,          defuintval:0,                   TYPE_INT,       0 },
  {"debug",                        "<debug level>\n",               0,                 uptr:&websrvparams.dbglvl,            defuintval:0,                   TYPE_UINT,      0 },
  {"url",                          "<server url>\n",                0,                 strptr:&websrvparams.url,             defstrval:"index.html",         TYPE_STRING,    0 }, 
  {"cert",                         "<cert file>\n",                 0,                 strptr:&websrvparams.certfile,        defstrval:NULL,                 TYPE_STRING,    0 }, 
  {"key",                          "<key file>\n",                  0,                 strptr:&websrvparams.keyfile,         defstrval:NULL,                 TYPE_STRING,    0 },
};

void websrv_printjson(char * label, json_t *jsonobj){
	char *jstr = json_dumps(jsonobj,0);
	LOG_I(UTIL,"%s:%s\n", label, (jstr==NULL)?"??\n":jstr);
}

char * websrv_read_file(const char * filename) {
  char * buffer = NULL;
  long length;
  FILE * f = fopen (filename, "rb");
  if (f != NULL) {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length + 1);
    if (buffer != NULL) {
      int rlen = fread (buffer, 1, length, f);
      if (rlen !=length) {
        free(buffer);
        LOG_E(UTIL,"websrv couldn't read %s_\n",filename);
        return NULL;
      }
      buffer[length] = '\0';
    }
    fclose (f);
  }
  return buffer;
}
/* callbacks to send static streams */
static ssize_t callback_stream(void * cls, uint64_t pos, char * buf, size_t max) {
  if (cls != NULL) {
    return fread (buf, sizeof(char), max, (FILE *)cls);
  } else {
    return U_STREAM_END;
  }
}

static void callback_stream_free(void * cls) {
  if (cls != NULL) {
    fclose((FILE *)cls);
  }
}

FILE *websrv_getfile(char *filename, struct _u_response * response) {
    FILE *f = fopen (filename, "rb");
  int length;
  
  if (f) {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    LOG_I(UTIL,"websrv sending %d bytes from %s\n", length, filename);
  } else {
    LOG_E(UTIL,"websrv couldn't open %s\n",filename);
    return NULL;
  }

  char *content_type="text/html";
  size_t nl = strlen(filename);
  if ( nl >= 3 && !strcmp(filename + nl - 3, "css"))
     content_type="text/css";

  int ust=ulfius_add_header_to_response(response,"content-type" ,content_type);
  if (ust != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	  LOG_E(UTIL,"websrv cannot set response header type ulfius error %d \n",ust);
      fclose(f);
      return NULL;
  }
  
  ust=ulfius_set_stream_response(response, 200, callback_stream, callback_stream_free, length, 1024, f);
  if(ust != U_OK) {
    LOG_E(UTIL,"websrv ulfius_set_stream_response error %d\n",ust);
    fclose(f);
    return NULL;
  }
  return f;  
    
}

/* callback processing main ((initial) url (<address>/<websrvparams.url> */
int websrv_callback_get_mainurl(const struct _u_request * request, struct _u_response * response, void * user_data) {
  LOG_I(UTIL,"Requested file is: %s\n",request->http_url);  

  FILE *f = websrv_getfile(websrvparams.url,response) ;
  if (f == NULL)
    return U_CALLBACK_ERROR;
  return U_CALLBACK_CONTINUE;
}

 int websrv_callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  LOG_I(UTIL,"Requested file is: %s %s\n",request->http_verb,request->http_url);
  if (request->map_post_body != NULL)
    for (int i=0; i<u_map_count(request->map_post_body) ; i++)
      LOG_I(UTIL,"POST parameter %i %s : %s\n",i,u_map_enum_keys(request->map_post_body)[i], u_map_enum_values(request->map_post_body)[i]	);
  char *tmpurl = strdup(websrvparams.url);
  char *srvdir = dirname(tmpurl);
  if (srvdir==NULL) {
      LOG_E(UTIL,"Cannot extract dir name from %s requested file is %s\n",websrvparams.url,request->http_url); 
      return U_CALLBACK_ERROR;
  }
  char *fpath = malloc( strlen(request->http_url)+strlen(srvdir)+2);
  sprintf(fpath,"%s/%s",srvdir, request->http_url);
  FILE *f = websrv_getfile(fpath,response) ;
  free(fpath);
  free(tmpurl);
  if (f == NULL)
    return U_CALLBACK_ERROR;
  return U_CALLBACK_CONTINUE;
}
/* callback processing module url (<address>/oaisoftmodem/module/variables), post method */
int websrv_callback_okset_softmodemvar(const struct _u_request * request, struct _u_response * response, void * user_data) {
	 LOG_I(UTIL,"websrv : callback_okset_softmodemvar received %s %s\n",request->http_verb,request->http_url);
     for (int i=0; i<u_map_count(request->map_header) ; i++)
       LOG_I(UTIL,"header variable %i %s : %s\n",i,u_map_enum_keys(request->map_header)[i], u_map_enum_values(request->map_header)[i]	);
     int us=ulfius_add_header_to_response(response,"Access-Control-Request-Method" ,"POST");
      if (us != U_OK){
	    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	    LOG_E(UTIL,"websrv cannot set response header type ulfius error %d \n",us);
      }  
      us=ulfius_add_header_to_response(response,"Access-Control-Allow-Headers", "content-type"); 
      us=ulfius_set_empty_body_response(response, 200);
      if (us != U_OK){
	    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_empty_body_response)");
	    LOG_E(UTIL,"websrv cannot set empty body response ulfius error %d \n",us);
      }  
	 return U_CALLBACK_CONTINUE;   
}
int websrv_callback_set_softmodemvar(const struct _u_request * request, struct _u_response * response, void * user_data) {
	 LOG_I(UTIL,"websrv : callback_set_softmodemvar received %s %s\n",request->http_verb,request->http_url);
	 json_error_t jserr;
	 json_t* jsbody = ulfius_get_json_body_request (request, &jserr);
	 if (jsbody == NULL) {
       LOG_E(UTIL,"websrv cannot find json body in %s %s\n",request->http_url, jserr.text );
       		 
	 } else {
	   websrv_printjson("callback_set_softmodemcmd: ",jsbody);
	 }
//	 cmdparser_t * modulestruct = (cmdparser_t *)user_data;
	 ulfius_set_empty_body_response(response, 200);
	 return U_CALLBACK_COMPLETE;   
}

/* callback processing module url (<address>/oaisoftmodem/module/variables), get method*/
int websrv_callback_get_softmodemvar(const struct _u_request * request, struct _u_response * response, void * user_data) {
	cmdparser_t * modulestruct = (cmdparser_t *)user_data;
	

	LOG_I(UTIL,"websrv received  %s variables request\n", modulestruct->module);
	json_t *moduleactions = json_array();

     for(int j=0; modulestruct->var[j].varvalptr != NULL ; j++) {
	   char*strval=telnet_getvarvalue(modulestruct->var, j);
	   char *strbool="true";
	   if (modulestruct->var[j].checkval & TELNET_CHECKVAL_RDONLY)
	     strbool="false";
	   json_t *oneaction =json_pack("{s:s,s:s,s:s,s:s}","type","string","name",modulestruct->var[j].varname,"value",strval,"modifiable",strbool);
       if (oneaction==NULL) {
	     LOG_E(UTIL,"websrv cannot encode oneaction %s/%s\n",modulestruct->module,modulestruct->var[j].varname);
       } else {
	     websrv_printjson("oneaction",oneaction);
       }   
	   free(strval);
       json_array_append(moduleactions , oneaction);
     }
     if (moduleactions==NULL) {
	   LOG_E(UTIL,"websrv cannot encode moduleactions response for %s\n",modulestruct->module);
     } else {
	   websrv_printjson("moduleactions",moduleactions);
     }

     int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
     if (us != U_OK){
	   ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	   LOG_E(UTIL,"websrv cannot set response header type ulfius error %d \n",us);
     }   
     us=ulfius_set_json_body_response(response, 200, moduleactions);
     if (us != U_OK){
	   ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	   LOG_E(UTIL,"websrv cannot set body response ulfius error %d \n",us);
     }   
    return U_CALLBACK_CONTINUE;     
}

/* callback processing module url (<address>/oaisoftmodem/module/commands)*/
int websrv_callback_get_softmodemcmd(const struct _u_request * request, struct _u_response * response, void * user_data) {
	cmdparser_t *modulestruct = (cmdparser_t *)user_data;
	

	LOG_I(UTIL,"websrv received  %s commands request\n", modulestruct->module);
	    json_t *modulesubcom = json_array();
        for(int j=0; modulestruct->cmd[j].cmdfunc != NULL ; j++) {
		  json_t *jsstr=json_string(modulestruct->cmd[j].cmdname);
		  json_array_append(modulesubcom , jsstr);
        }
        if (modulesubcom==NULL) {
	      LOG_E(UTIL,"websrv cannot encode modulesubcom response for %s\n",modulestruct->module);
        } else {
	      websrv_printjson("modulesubcom",modulesubcom);
        }             
        int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
        if (us != U_OK){
	      ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	      LOG_E(UTIL,"websrv cannot set response header type ulfius error %d \n",us);
        }   
        us=ulfius_set_json_body_response(response, 200, modulesubcom);
        if (us != U_OK){
	      ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	      LOG_E(UTIL,"websrv cannot set body response ulfius error %d \n",us);
        }        
	return U_CALLBACK_CONTINUE;
}

int websrv_callback_get_softmodemmodules(const struct _u_request * request, struct _u_response * response, void * user_data) {
  telnetsrv_params_t *telnetparams= get_telnetsrv_params();

  json_t *cmdnames = json_array();
  for (int i=0; telnetparams->CmdParsers[i].var != NULL && telnetparams->CmdParsers[i].cmd != NULL; i++) {
	  json_t *acmd =json_pack( "{s:s}", "name",telnetparams->CmdParsers[i].module);
	  json_array_append(cmdnames, acmd);
    }

  
  int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	  LOG_E(UTIL,"websrv cannot set modules response header type ulfius error %d \n",us);
  }  
  
  us=ulfius_set_json_body_response(response, 200, cmdnames);
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	  LOG_E(UTIL,"websrv cannot set modules body response ulfius error %d \n",us);
  } else {
	  websrv_printjson("cmdnames",cmdnames);
  }  
//  ulfius_set_string_body_response(response, 200, cfgfile);
  return U_CALLBACK_CONTINUE;
}
/* callback processing initial url (<address>/oaisoftmodem)*/
int websrv_callback_get_softmodemstatus(const struct _u_request * request, struct _u_response * response, void * user_data) {
  char *cfgfile=CONFIG_GETCONFFILE ;
  char *execfunc=get_softmodem_function(NULL);
  char *strbool="false";
  char *strtype="string";
  json_t *moduleactions = json_array();
  json_t *body1=json_pack("{s:s,s:s,s:s,s:s}","name","config_file", "value",cfgfile, "type",strtype,"modifiable",strbool);
  if (body1==NULL) {
	  LOG_E(UTIL,"websrv cannot encode status body1 response\n");
  } else {
	  websrv_printjson("status body1",body1);
  }  

  json_t *body2=json_pack("{s:s,s:s,s:s,s:s}","name","exec_function", "value",execfunc, "type", strtype, "modifiable",strbool); 
  if (body2==NULL) {
	  LOG_E(UTIL,"websrv cannot encode status body1 response\n");
  } else {
	  websrv_printjson("status body2",body2);
  } 

  json_array_append(moduleactions , body1);
  json_array_append(moduleactions , body2);
  
  int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	  LOG_E(UTIL,"websrv cannot set status response header type ulfius error %d \n",us);
  }  
  
  us=ulfius_set_json_body_response(response, 200, moduleactions);
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	  LOG_E(UTIL,"websrv cannot set status body response ulfius error %d \n",us);
  }
//  ulfius_set_string_body_response(response, 200, cfgfile);
  return U_CALLBACK_CONTINUE;
}

 void* websrv_autoinit() {
  int ret;
  telnetsrv_params_t *telnetparams= get_telnetsrv_params(); 
  memset(&websrvparams,0,sizeof(websrvparams));
  config_get( websrvoptions,sizeof(websrvoptions)/sizeof(paramdef_t),"websrv");
  websrvparams.instance = malloc(sizeof(struct _u_instance));
  
  
  
  if (ulfius_init_instance(websrvparams.instance, websrvparams.listenport, NULL, NULL) != U_OK) {
    LOG_W(UTIL, "Error,cannot init websrv\n");
    free(websrvparams.instance);
    return(NULL);
  }
  
  u_map_put(websrvparams.instance->default_headers, "Access-Control-Allow-Origin", "*");
  
  // Maximum body size sent by the client is 1 Kb
  websrvparams.instance->max_post_body_size = 1024;
  
  // Endpoint list declaration
  //1: load the frontend code: files contained in the websrvparams.url directory
  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", websrvparams.url, NULL, 0, &websrv_callback_get_mainurl, NULL);
  //2: build the first page, when receiving the "oaisoftmodem" url 
  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", "oaisoftmodem", "variables", 0, &websrv_callback_get_softmodemstatus, NULL);
  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", "oaisoftmodem", "commands", 0, &websrv_callback_get_softmodemmodules, NULL);
 //ulfius_add_endpoint_by_val(&instance, "GET", "softmodem", "", 0, &callback_get_empty_response, NULL);
 // ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/multiple/:multiple/:multiple/:not_multiple", 0, &callback_all_test_foo, NULL);
 // ulfius_add_endpoint_by_val(&instance, "POST", PREFIX, NULL, 0, &callback_post_test, NULL);
 // ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 1");
 // ulfius_add_endpoint_by_val(&instance, "POST", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 2");
 // ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 3");
//  ulfius_add_endpoint_by_val(&instance, "DELETE", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 4");
 // ulfius_add_endpoint_by_val(&instance, "GET", PREFIXCOOKIE, "/:lang/:extra", 0, &callback_get_cookietest, NULL);
  
  // default_endpoint declaration
  ulfius_set_default_endpoint(websrvparams.instance, &websrv_callback_default, NULL);
  int status=ulfius_add_endpoint_by_val(websrvparams.instance, "OPTIONS", "oaisoftmodem","variables" , 1, &websrv_callback_okset_softmodemvar, NULL );
  if (status != U_OK) {
	  LOG_E(UTIL,"websrv cannot add endpoint oaisoftmodem/variables\n");
  }  
  ulfius_add_endpoint_by_val(websrvparams.instance, "POST", "oaisoftmodem","variables" , 0, &websrv_callback_set_softmodemvar, NULL );

  for (int i=0; telnetparams->CmdParsers[i].var != NULL && telnetparams->CmdParsers[i].cmd != NULL; i++) {
	  char prefixurl[TELNET_CMD_MAXSIZE+20];
	  snprintf(prefixurl,TELNET_CMD_MAXSIZE+19,"oaisoftmodem/%s",telnetparams->CmdParsers[i].module);
	  LOG_I(UTIL,"websrv add endpoints %s/[variables or commands] \n",prefixurl);
	  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", prefixurl,"variables" , 0, &websrv_callback_get_softmodemvar, &(telnetparams->CmdParsers[i]) );
	  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", prefixurl,"commands" , 0, &websrv_callback_get_softmodemcmd, &(telnetparams->CmdParsers[i]) );
      status=ulfius_add_endpoint_by_val(websrvparams.instance, "OPTIONS", prefixurl,"variables" , 0, &websrv_callback_okset_softmodemvar, &(telnetparams->CmdParsers[i]) );
      if (status != U_OK) {
	    LOG_E(UTIL,"websrv cannot add endpoint %s/variables\n",prefixurl);}
      ulfius_add_endpoint_by_val(websrvparams.instance, "POST", prefixurl,"variables" , 0, &websrv_callback_set_softmodemvar, &(telnetparams->CmdParsers[i]) ); 
    }
  // Start the framework
  ret=U_ERROR;
  if (websrvparams.keyfile!=NULL && websrvparams.certfile!=NULL) {
    char * key_pem = websrv_read_file(websrvparams.keyfile);
    char * cert_pem = websrv_read_file(websrvparams.certfile);
    if ( key_pem == NULL && cert_pem != NULL) {
      ret = ulfius_start_secure_framework(websrvparams.instance, key_pem, cert_pem);
      free(key_pem);
      free(cert_pem);
    } else {
      LOG_E(UTIL,"Unable to load key %s and cert %s_\n",websrvparams.keyfile,websrvparams.certfile);
    }
  } else {
    ret = ulfius_start_framework(websrvparams.instance);
  }

  if (ret == U_OK) {
    LOG_I(UTIL, "Web server started on port %d", websrvparams.instance->port);
  } else {
    LOG_W(UTIL,"Error starting web server on port %d\n",websrvparams.instance->port);
  }
 return websrvparams.instance;

}

void websrv_end(void *webinst) {
  ulfius_stop_framework((struct _u_instance *)webinst);
  ulfius_clean_instance((struct _u_instance *)webinst);
  
  return;
}
