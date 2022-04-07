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
	LOG_I(UTIL,"[websrv] %s:%s\n", label, (jstr==NULL)?"??\n":jstr);
}

int websrv_add_endpoint( char **http_method, int num_method, const char * url_prefix,const char * url_format,
                         int (* callback_function[])(const struct _u_request * request, 
                                                   struct _u_response * response,
                                                   void * user_data),
                         void * user_data) {
  int status;
  int j=0;
  for (int i=0; i<num_method; i++) {
    status=ulfius_add_endpoint_by_val(websrvparams.instance,http_method[i],url_prefix,url_format,0,callback_function[i],user_data);
    if (status != U_OK) {
      LOG_E(UTIL,"[websrv] cannot add endpoint %s %s/%s\n",http_method[i],url_prefix,url_format);
    } else {
      j++;
      LOG_I(UTIL,"[websrv] endpoint %s %s/%s added\n",http_method[i],url_prefix,url_format);
    }
  }
  return j;
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
        LOG_E(UTIL,"[websrv] couldn't read %s_\n",filename);
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
    LOG_I(UTIL,"[websrv] sending %d bytes from %s\n", length, filename);
  } else {
    LOG_E(UTIL,"[websrv] couldn't open %s\n",filename);
    return NULL;
  }

  char *content_type="text/html";
  size_t nl = strlen(filename);
  if ( nl >= 3 && !strcmp(filename + nl - 3, "css"))
     content_type="text/css";

  int ust=ulfius_add_header_to_response(response,"content-type" ,content_type);
  if (ust != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	  LOG_E(UTIL,"[websrv] cannot set response header type ulfius error %d \n",ust);
      fclose(f);
      return NULL;
  }
  
  ust=ulfius_set_stream_response(response, 200, callback_stream, callback_stream_free, length, 1024, f);
  if(ust != U_OK) {
    LOG_E(UTIL,"[websrv] ulfius_set_stream_response error %d\n",ust);
    fclose(f);
    return NULL;
  }
  return f;  
    
}

/* callback processing main ((initial) url (<address>/<websrvparams.url> */
int websrv_callback_get_mainurl(const struct _u_request * request, struct _u_response * response, void * user_data) {
  LOG_I(UTIL,"[websrv] Requested file is: %s\n",request->http_url);  

  FILE *f = websrv_getfile(websrvparams.url,response) ;
  if (f == NULL)
    return U_CALLBACK_ERROR;
  return U_CALLBACK_CONTINUE;
}

 int websrv_callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  LOG_I(UTIL,"[websrv] Requested file is: %s %s\n",request->http_verb,request->http_url);
  if (request->map_post_body != NULL)
    for (int i=0; i<u_map_count(request->map_post_body) ; i++)
      LOG_I(UTIL,"[websrv] POST parameter %i %s : %s\n",i,u_map_enum_keys(request->map_post_body)[i], u_map_enum_values(request->map_post_body)[i]	);
  char *tmpurl = strdup(websrvparams.url);
  char *srvdir = dirname(tmpurl);
  if (srvdir==NULL) {
      LOG_E(UTIL,"[websrv] Cannot extract dir name from %s requested file is %s\n",websrvparams.url,request->http_url); 
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
/* callback processing  url (<address>/oaisoftmodem/module/variables or <address>/oaisoftmodem/module/commands), options method */
int websrv_callback_okset_softmodem_cmdvar(const struct _u_request * request, struct _u_response * response, void * user_data) {
	 LOG_I(UTIL,"[websrv] : callback_okset_softmodem_cmdvar received %s %s\n",request->http_verb,request->http_url);
     for (int i=0; i<u_map_count(request->map_header) ; i++)
       LOG_I(UTIL,"[websrv] header variable %i %s : %s\n",i,u_map_enum_keys(request->map_header)[i], u_map_enum_values(request->map_header)[i]	);
     int us=ulfius_add_header_to_response(response,"Access-Control-Request-Method" ,"POST");
      if (us != U_OK){
	    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	    LOG_E(UTIL,"[websrv] cannot set response header type ulfius error %d \n",us);
      }  
      us=ulfius_add_header_to_response(response,"Access-Control-Allow-Headers", "content-type"); 
      us=ulfius_set_empty_body_response(response, 200);
      if (us != U_OK){
	    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_empty_body_response)");
	    LOG_E(UTIL,"[websrv] cannot set empty body response ulfius error %d \n",us);
      }  
	 return U_CALLBACK_CONTINUE;   
}
int websrv_callback_set_softmodemvar(const struct _u_request * request, struct _u_response * response, void * user_data) {
	 LOG_I(UTIL,"[websrv] : callback_set_softmodemvar received %s %s\n",request->http_verb,request->http_url);
	 json_error_t jserr;
	 json_t* jsbody = ulfius_get_json_body_request (request, &jserr);
     int httpstatus=404;
	 if (jsbody == NULL) {
       LOG_E(UTIL,"[websrv] cannot find json body in %s %s\n",request->http_url, jserr.text );
       httpstatus=400;	 
	 } else {
	   websrv_printjson("callback_set_softmodemvar: ",jsbody);
       if (user_data == NULL) {
         httpstatus=500;
         LOG_E(UTIL,"[websrv] %s: NULL user data\n",request->http_url);
       } else {
	     cmdparser_t * modulestruct = (cmdparser_t *)user_data;
         const char *vname=NULL;
         const char *vval=NULL;
         json_t *J=json_object_get(jsbody, "name");
         vname=(J==NULL)?"": json_string_value(J);
         for ( telnetshell_vardef_t *var = modulestruct->var; var!= NULL ;var++) {
           if (strncmp(var->varname,vname,TELNET_CMD_MAXSIZE) == 0){
             J=json_object_get(jsbody, "value");
             vval=(J==NULL)?"": json_string_value(J);
             int s = telnet_setvarvalue(var,(char *)vval,NULL);
             if ( s!=0 ) {
               httpstatus=500; 
               LOG_E(UTIL,"[websrv] Cannot set var %s to %s\n",vname,vval );
             } else {
               httpstatus=200;
               LOG_I(UTIL,"[websrv] var %s set to %s\n",vname,vval );
             }
             break;
           }
         }//for
       }//user_data
     } //sbody
     ulfius_set_empty_body_response(response, httpstatus);
	 return U_CALLBACK_COMPLETE;   
}
/* callback processing module url (<address>/oaisoftmodem/module/commands), post method */

int websrv_callback_set_softmodemcmd(const struct _u_request * request, struct _u_response * response, void * user_data) {
	 LOG_I(UTIL,"[websrv] : callback_set_softmodemcmd received %s %s\n",request->http_verb,request->http_url);
	 json_error_t jserr;
	 json_t* jsbody = ulfius_get_json_body_request (request, &jserr);
     int httpstatus=404;
	 if (jsbody == NULL) {
       httpstatus=400;	 
	 } else {
	   websrv_printjson("callback_set_softmodemcmd: ",jsbody);
       if (user_data == NULL) {
         httpstatus=500;
         LOG_E(UTIL,"[websrv] %s: NULL user data\n",request->http_url);
       } else {
         const char *vname=NULL;
         json_t *J=json_object_get(jsbody, "name");
         vname=(J==NULL)?"": json_string_value(J);

         if ( vname[0] == 0 ) {
		   LOG_E(UTIL,"[websrv] command name not found in body\n");
           httpstatus=400;
         } else {
		   httpstatus=501; 
		 }
       }//user_data
     } //sbody
     ulfius_set_empty_body_response(response, httpstatus);
	 return U_CALLBACK_COMPLETE;   
}
/* callback processing module url (<address>/oaisoftmodem/module/variables), get method*/
int websrv_callback_get_softmodemvar(const struct _u_request * request, struct _u_response * response, void * user_data) {
	cmdparser_t * modulestruct = (cmdparser_t *)user_data;
	

	LOG_I(UTIL,"[websrv] received  %s variables request\n", modulestruct->module);
	json_t *moduleactions = json_array();

     for(int j=0; modulestruct->var[j].varvalptr != NULL ; j++) {
	   char*strval=telnet_getvarvalue(modulestruct->var, j);
	   int modifiable=1;
	   if (modulestruct->var[j].checkval & TELNET_CHECKVAL_RDONLY)
	     modifiable=0;
	   json_t *oneaction =json_pack("{s:s,s:s,s:s,s:b}","type","string","name",modulestruct->var[j].varname,"value",strval,"modifiable",modifiable);
       if (oneaction==NULL) {
	     LOG_E(UTIL,"[websrv] cannot encode oneaction %s/%s\n",modulestruct->module,modulestruct->var[j].varname);
       } else {
	     websrv_printjson("oneaction",oneaction);
       }   
	   free(strval);
       json_array_append(moduleactions , oneaction);
     }
     if (moduleactions==NULL) {
	   LOG_E(UTIL,"[websrv] cannot encode moduleactions response for %s\n",modulestruct->module);
     } else {
	   websrv_printjson("moduleactions",moduleactions);
     }

     int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
     if (us != U_OK){
	   ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	   LOG_E(UTIL,"[websrv] cannot set response header type ulfius error %d \n",us);
     }   
     us=ulfius_set_json_body_response(response, 200, moduleactions);
     if (us != U_OK){
	   ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	   LOG_E(UTIL,"[websrv] cannot set body response ulfius error %d \n",us);
     }   
    return U_CALLBACK_CONTINUE;     
}

/* callback processing module url (<address>/oaisoftmodem/module/commands)*/
int websrv_callback_get_softmodemcmd(const struct _u_request * request, struct _u_response * response, void * user_data) {
	cmdparser_t *modulestruct = (cmdparser_t *)user_data;
	

	LOG_I(UTIL,"[websrv] received  %s commands request\n", modulestruct->module);
	    json_t *modulesubcom = json_array();
        for(int j=0; modulestruct->cmd[j].cmdfunc != NULL ; j++) {
		  json_t *acmd =json_pack( "{s:s}", "name",modulestruct->cmd[j].cmdname);
		  json_array_append(modulesubcom , acmd);
        }
        if (modulesubcom==NULL) {
	      LOG_E(UTIL,"[websrv] cannot encode modulesubcom response for %s\n",modulestruct->module);
        } else {
	      websrv_printjson("modulesubcom",modulesubcom);
        }             
        int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
        if (us != U_OK){
	      ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	      LOG_E(UTIL,"[websrv] cannot set response header type ulfius error %d \n",us);
        }   
        us=ulfius_set_json_body_response(response, 200, modulesubcom);
        if (us != U_OK){
	      ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	      LOG_E(UTIL,"[websrv] cannot set body response ulfius error %d \n",us);
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
	  LOG_E(UTIL,"[websrv] cannot set modules response header type ulfius error %d \n",us);
  }  
  
  us=ulfius_set_json_body_response(response, 200, cmdnames);
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	  LOG_E(UTIL,"[websrv] cannot set modules body response ulfius error %d \n",us);
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
  char *strtype="string";
  json_t *moduleactions = json_array();
  json_t *body1=json_pack("{s:s,s:s,s:s,s:b}","name","config_file", "value",cfgfile, "type",strtype,"modifiable",0);
  if (body1==NULL) {
	  LOG_E(UTIL,"[websrv] cannot encode status body1 response\n");
  } else {
	  websrv_printjson("status body1",body1);
  }  

  json_t *body2=json_pack("{s:s,s:s,s:s,s:b}","name","exec_function", "value",execfunc, "type", strtype, "modifiable",0); 
  if (body2==NULL) {
	  LOG_E(UTIL,"[websrv] cannot encode status body1 response\n");
  } else {
	  websrv_printjson("status body2",body2);
  } 

  json_array_append(moduleactions , body1);
  json_array_append(moduleactions , body2);
  
  int us=ulfius_add_header_to_response(response,"content-type" ,"application/json");
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
	  LOG_E(UTIL,"[websrv] cannot set status response header type ulfius error %d \n",us);
  }  
  
  us=ulfius_set_json_body_response(response, 200, moduleactions);
  if (us != U_OK){
	  ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_set_json_body_response)");
	  LOG_E(UTIL,"[websrv] cannot set status body response ulfius error %d \n",us);
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
    LOG_W(UTIL, "[websrv] Error,cannot init websrv\n");
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
//  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", "oaisoftmodem", "variables", 0, &websrv_callback_get_softmodemstatus, NULL);
  ulfius_add_endpoint_by_val(websrvparams.instance, "GET", "oaisoftmodem", "commands", 0, &websrv_callback_get_softmodemmodules, NULL);

  //3 default_endpoint declaration, it tries to open the file with the url name as specified in the request.It looks for the file 
  ulfius_set_default_endpoint(websrvparams.instance, &websrv_callback_default, NULL);

  // endpoints 
  int (* callback_functions_var[3])(const struct _u_request * request, 
                                               struct _u_response * response,
                                               void * user_data) ={websrv_callback_get_softmodemstatus,websrv_callback_okset_softmodem_cmdvar,websrv_callback_set_softmodemvar};
  char *http_methods[3]={"GET","OPTIONS","POST"};

  websrv_add_endpoint(http_methods,3,"oaisoftmodem","variables" ,callback_functions_var,NULL);
  callback_functions_var[0]=websrv_callback_get_softmodemvar;

int (* callback_functions_cmd[3])(const struct _u_request * request, 
                                               struct _u_response * response,
                                               void * user_data) ={websrv_callback_get_softmodemcmd,websrv_callback_okset_softmodem_cmdvar,websrv_callback_set_softmodemcmd};
  for (int i=0; telnetparams->CmdParsers[i].var != NULL && telnetparams->CmdParsers[i].cmd != NULL; i++) {
	  char prefixurl[TELNET_CMD_MAXSIZE+20];
	  snprintf(prefixurl,TELNET_CMD_MAXSIZE+19,"oaisoftmodem/%s",telnetparams->CmdParsers[i].module);
	  LOG_I(UTIL,"[websrv] add endpoints %s/[variables or commands] \n",prefixurl);

	  websrv_add_endpoint(http_methods,3,prefixurl,"commands" ,callback_functions_cmd , &(telnetparams->CmdParsers[i]) );
      websrv_add_endpoint(http_methods,3,prefixurl,"variables" ,callback_functions_var,&(telnetparams->CmdParsers[i]));
      
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
      LOG_E(UTIL,"[websrv] Unable to load key %s and cert %s_\n",websrvparams.keyfile,websrvparams.certfile);
    }
  } else {
    ret = ulfius_start_framework(websrvparams.instance);
  }

  if (ret == U_OK) {
    LOG_I(UTIL, "[websrv] Web server started on port %d", websrvparams.instance->port);
  } else {
    LOG_W(UTIL,"[websrv] Error starting web server on port %d\n",websrvparams.instance->port);
  }
 return websrvparams.instance;

}

void websrv_end(void *webinst) {
  ulfius_stop_framework((struct _u_instance *)webinst);
  ulfius_clean_instance((struct _u_instance *)webinst);
  
  return;
}
