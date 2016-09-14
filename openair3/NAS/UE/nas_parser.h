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
