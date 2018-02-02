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

/*****************************************************************************
Source      nas_parser.h

Version     0.1

Date        2012/02/27

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS command line parser

*****************************************************************************/
#ifndef __NAS_PARSER_H__
#define __NAS_PARSER_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Logging trace level default value */
#define NAS_PARSER_DEFAULT_TRACE_LEVEL      "0" /* No trace */

/* Network layer default hostname */
#define NAS_PARSER_DEFAULT_NETWORK_HOSTNAME "localhost"

/* Network layer default port number */
#define NAS_PARSER_DEFAULT_NETWORK_PORT_NUMBER  "12000"

/* User Identifier default value */
#define NAS_PARSER_DEFAULT_UE_ID        "1"

/* User application layer default hostname */
#define NAS_PARSER_DEFAULT_USER_HOSTNAME    "localhost"

/* User application layer default port number */
#define NAS_PARSER_DEFAULT_USER_PORT_NUMBER "10000"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void nas_parser_print_usage(const char *version);
int  nas_parser_get_options(int argc, const char **argv);

int  nas_parser_get_nb_options(void);
int  nas_parser_get_trace_level(void);
const char *nas_parser_get_network_host(void);
const char *nas_parser_get_network_port(void);

int  nas_parser_get_ueid(void);
const char *nas_parser_get_user_host(void);
const char *nas_parser_get_user_port(void);
const char *nas_parser_get_device_path(void);
const char *nas_parser_get_device_params(void);

#endif /* __NAS_PARSER_H__*/
