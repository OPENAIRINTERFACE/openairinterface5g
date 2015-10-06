/*****************************************************************************
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_simulator_parser.h

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Command line parser

Author    Frederic Maurel

Description Command line parser of the AS Simulator process

*****************************************************************************/

#ifndef __AS_SIMULATOR_PARSER_H__
#define __AS_SIMULATOR_PARSER_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* UE process default hostname */
#define AS_SIMULATOR_PARSER_DEFAULT_UE_HOSTNAME   "localhost"

/* MME process default hostname */
#define AS_SIMULATOR_PARSER_DEFAULT_MME_HOSTNAME  "localhost"

/* UE process default port number */
#define AS_SIMULATOR_PARSER_DEFAULT_UE_PORT_NUMBER  "12000"

/* MME process default port number */
#define AS_SIMULATOR_PARSER_DEFAULT_MME_PORT_NUMBER "14000"

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void as_simulator_parser_print_usage(void);
int as_simulator_parser_get_options(int argc, const char** argv);

int as_simulator_parser_get_nb_options(void);
const char* as_simulator_parser_get_uhost(void);
const char* as_simulator_parser_get_uport(void);
const char* as_simulator_parser_get_mhost(void);
const char* as_simulator_parser_get_mport(void);

#endif /* __AS_SIMULATOR_PARSER_H__*/
