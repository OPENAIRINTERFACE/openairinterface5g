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

/*! \file common/utils/websrv/websrv_utils.c
 * \brief: utility functions for all websrv back-end sources
 * \author Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "common/utils/LOG/log.h"
#include <libgen.h>
#include <jansson.h>
#include <ulfius.h>

/* websrv_printf_t is an internal structure storing messages while processing a request */
/* The meaage is used to fill a response body                                           */
typedef struct {
  pthread_mutex_t mutex; // protect the message betwween the websrv_printf_start and websrv_print_end calls
  struct _u_response *response; // the ulfius response structure, used to send the message
  char *buff; // a buffer to store the message, allocated in websrv_printf_start, free in websrv_print_end
  int buffsize; //
  char *buffptr; // pointer to free portion of buff
  bool async; // the buffer will be used to print from another thread than the callback (command pushed in a tpool queue)
} websrv_printf_t;
static websrv_printf_t websrv_printf_buff;
/*--------------------- functions for help ------------------------*/
/* possibly build a help file to be downloaded by the frontend */
void websrv_utils_build_hlpfile(char *fname)
{
  char *info = NULL;

  if (strstr(fname, "core.html") != NULL) {
    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    if (number_of_processors > 0) {
      char *pinfo = malloc(255);
      snprintf(pinfo, 254, "Available cores on this system: 0 to %d", (int)number_of_processors - 1);
      info = pinfo;
    }
  }
  if (info != NULL) {
    FILE *f = fopen(fname, "w");
    if (f == NULL) {
      LOG_I(UTIL, "[websrv]: Help file %s couldn't be created\n", fname);
    } else {
      fprintf(f, "%s", info);
      fclose(f);
    }
    free(info);
  }
}

void websrv_utils_build_hlpfiles(char *path)
{
  char fname[255];
  snprintf(fname, sizeof(fname), "%s/helpfiles/softmodem_show_threadsched_core.html", path);
  websrv_utils_build_hlpfile(fname);
}
int websrv_utils_testhlp(char *dir, char *module, char *cmdtitle, char *coltitle)
{
  char fname[255];

  snprintf(fname, sizeof(fname) - 1, "%s/helpfiles/%s_%s_%s.html", dir, module, cmdtitle, coltitle);
  for (int i = 0; i < strlen(fname); i++) {
    if (fname[i] == ' ')
      fname[i] = '_';
  }
  int st = access(fname, R_OK);
  return ((st == 0) ? 1 : 0);
}
/*-----------------------------------------------------------*/
/* functions http request/response helpers                  */
/* dump a json objects */
void websrv_printjson(char *label, json_t *jsonobj, int dbglvl)
{
  if (dbglvl > 0) {
    char *jstr = json_dumps(jsonobj, 0);
    LOG_I(UTIL, "[websrv] %s:%s\n", label, (jstr == NULL) ? "??\n" : jstr);
    free(jstr);
  }
}

/*------------------------------------------------------------------------------*/
/* dump a request                                                               */
void websrv_dump_request(char *label, const struct _u_request *request, int dbglvl)
{
  if (dbglvl > 0) {
    LOG_I(UTIL, "[websrv] %s, request %s, proto %s, verb %s, path %s\n", label, request->http_url, request->http_protocol, request->http_verb, request->http_verb);
    if (request->map_post_body != NULL)
      for (int i = 0; i < u_map_count(request->map_post_body); i++)
        LOG_I(UTIL, "[websrv] POST parameter %i %s : %s\n", i, u_map_enum_keys(request->map_post_body)[i], u_map_enum_values(request->map_post_body)[i]);
    if (request->map_cookie != NULL)
      for (int i = 0; i < u_map_count(request->map_cookie); i++)
        LOG_I(UTIL, "[websrv] cookie variable %i %s : %s\n", i, u_map_enum_keys(request->map_cookie)[i], u_map_enum_values(request->map_cookie)[i]);
    if (request->map_header != NULL)
      for (int i = 0; i < u_map_count(request->map_header); i++)
        LOG_I(UTIL, "[websrv] header variable %i %s : %s\n", i, u_map_enum_keys(request->map_header)[i], u_map_enum_values(request->map_header)[i]);
    if (request->map_url != NULL)
      for (int i = 0; i < u_map_count(request->map_url); i++)
        LOG_I(UTIL, "[websrv] url variable %i %s : %s\n", i, u_map_enum_keys(request->map_url)[i], u_map_enum_values(request->map_url)[i]);
  }
}
/*-----------------------------------*/
/* build a json body in a response   */
void websrv_jbody(struct _u_response *response, json_t *jbody, int httpstatus, int dbglvl)
{
  websrv_printjson((char *)__FUNCTION__, jbody, dbglvl);
  int us = ulfius_add_header_to_response(response, "content-type", "application/json");
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 500, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set response header type ulfius error %d \n", us);
  }
  us = ulfius_set_json_body_response(response, httpstatus, jbody);
  if (us != U_OK) {
    ulfius_set_string_body_response(response, 500, "Internal server error (ulfius_set_json_body_response)");
    LOG_E(UTIL, "[websrv] cannot set body response ulfius error %d \n", us);
  }
  return;
}

/*----------------------------------------------------------------*/
/* format a json string array in a response body                  */
int websrv_string_response(char *astring, struct _u_response *response, int httpstatus, int dbglvl)
{
  json_t *jstr = json_array();
  char *tokctx;
  char *aline = strtok_r(astring, "\n", &tokctx);
  while (aline != NULL) {
    json_t *jline = json_string(aline);
    json_array_append_new(jstr, jline);
    aline = strtok_r(NULL, "\n", &tokctx);
  }
  json_t *jbody = json_pack("{s:o}", "display", jstr);
  websrv_jbody(response, jbody, httpstatus, dbglvl);
  return 0;
}
/*----------------------------------------------------------------------------------*/
/* set of calls to fill a buffer with a string and  use this buffer in a response */
void websrv_printf_start(struct _u_response *response, int buffsize, bool async)
{
  pthread_mutex_lock(&(websrv_printf_buff.mutex));
  websrv_printf_buff.buff = malloc(buffsize);
  websrv_printf_buff.buffptr = websrv_printf_buff.buff;
  websrv_printf_buff.buffsize = buffsize;
  websrv_printf_buff.response = response;
  websrv_printf_buff.async = async;
}

void websrv_printf_atpos(int pos, const char *message, ...)
{
  va_list va_args;
  va_start(va_args, message);

  websrv_printf_buff.buffptr = websrv_printf_buff.buff + pos + vsnprintf(websrv_printf_buff.buff + pos, websrv_printf_buff.buffsize - pos - 1, message, va_args);

  va_end(va_args);
  return;
}

void websrv_printf(const char *message, ...)
{
  va_list va_args;
  va_start(va_args, message);
  websrv_printf_buff.buffptr += vsnprintf(websrv_printf_buff.buffptr, websrv_printf_buff.buffsize - (websrv_printf_buff.buffptr - websrv_printf_buff.buff) - 1, message, va_args);

  va_end(va_args);
  return;
}

void websrv_printf_end(int httpstatus, int dbglvl)
{
  int count = 0;
  while (__atomic_load_n(&(websrv_printf_buff.async), __ATOMIC_SEQ_CST)) {
    usleep(10);
    if (count == 99) {
      websrv_printf("No response from server...");
      break;
    }
  }
  if (httpstatus >= 200 && httpstatus < 300) {
    LOG_I(UTIL, "[websrv] %s\n", websrv_printf_buff.buff);
    websrv_string_response(websrv_printf_buff.buff, websrv_printf_buff.response, httpstatus, dbglvl);
  } else if (httpstatus < 1000) {
    LOG_W(UTIL, "[websrv] %s\n", websrv_printf_buff.buff);
    ulfius_set_binary_body_response(websrv_printf_buff.response, httpstatus, websrv_printf_buff.buff, websrv_printf_buff.buffptr - websrv_printf_buff.buff);
  }

  free(websrv_printf_buff.buff);
  websrv_printf_buff.buff = NULL;
  websrv_printf_buff.async = false;
  pthread_mutex_unlock(&(websrv_printf_buff.mutex));
}

void websrv_async_printf(const char *message, ...)
{
  if (__atomic_load_n(&(websrv_printf_buff.async), __ATOMIC_SEQ_CST)) {
    va_list va_args;
    va_start(va_args, message);
    websrv_printf_buff.buffptr += vsnprintf(websrv_printf_buff.buff, websrv_printf_buff.buffsize - 1, message, va_args);

    va_end(va_args);
    __atomic_store_n(&(websrv_printf_buff.async), 0, __ATOMIC_SEQ_CST);
  } else {
    LOG_W(UTIL, "[websrv], delayed  websrv_async_printf skipped\n");
  }
  return;
}
/* */
/*----------------------------------------------------------------------------------------------------------*/
/* callbacks and utility functions to stream a file */
char *websrv_read_file(const char *filename)
{
  char *buffer = NULL;
  long length;
  struct stat statbuf;

  FILE *f = fopen(filename, "rb");
  int st = stat(filename, &statbuf);
  if (f != NULL && st == 0) {
    length = statbuf.st_size;
    buffer = malloc(length + 1);
    if (buffer != NULL) {
      int rlen = fread(buffer, 1, length, f);
      if (rlen != length) {
        free(buffer);
        LOG_E(UTIL, "[websrv] couldn't read %s_\n", filename);
        fclose(f);
        return NULL;
      }
      buffer[length] = '\0';
    }
  } else {
    LOG_E(UTIL, "[websrv] error accessing %s: %s\n", filename, strerror(errno));
  }
  if (f != NULL)
    fclose(f);
  return buffer;
}
/* callbacks to send static streams */
static ssize_t callback_stream(void *cls, uint64_t pos, char *buf, size_t max)
{
  if (cls != NULL) {
    return fread(buf, sizeof(char), max, (FILE *)cls);
  } else {
    return U_STREAM_END;
  }
}

static void callback_stream_free(void *cls)
{
  if (cls != NULL) {
    fclose((FILE *)cls);
  }
}

FILE *websrv_getfile(char *filename, struct _u_response *response)
{
  FILE *f = fopen(filename, "rb");
  int length;

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    LOG_I(UTIL, "[websrv] sending %d bytes from %s\n", length, filename);
  } else {
    LOG_E(UTIL, "[websrv] couldn't open %s\n", filename);
    return NULL;
  }

  char *content_type = "text/html";
  size_t nl = strlen(filename);
  if (nl >= 3 && !strcmp(filename + nl - 3, "css"))
    content_type = "text/css";
  if (nl >= 2 && !strcmp(filename + nl - 2, "js"))
    content_type = "text/javascript";

  int ust = ulfius_add_header_to_response(response, "content-type", content_type);
  if (ust != U_OK) {
    ulfius_set_string_body_response(response, 501, "Internal server error (ulfius_add_header_to_response)");
    LOG_E(UTIL, "[websrv] cannot set response header type ulfius error %d \n", ust);
    fclose(f);
    return NULL;
  }

  ust = ulfius_set_stream_response(response, 200, callback_stream, callback_stream_free, length, 1024, f);
  if (ust != U_OK) {
    LOG_E(UTIL, "[websrv] ulfius_set_stream_response error %d\n", ust);
    fclose(f);
    return NULL;
  }
  return f;
}
/* */
