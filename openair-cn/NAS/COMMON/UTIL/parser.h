/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source    parser.h

Version   0.1

Date    2012/02/27

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Usefull command line parser

*****************************************************************************/
#ifndef __PARSER_H__
#define __PARSER_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* -----------
 * Option type
 * -----------
 *  An option is defined with a name, an argument following the option's
 *  name, the usage message and a value
 */
typedef struct {
  const char* name;     /* Option name         */
  const char* argument;   /* Argument following the option   */
  const char* usage;      /* Option and Argument usage     */
#define PARSER_OPTION_VALUE_SIZE  32
  char value[PARSER_OPTION_VALUE_SIZE]; /* Option value      */
  char* pvalue;
} parser_option_t;

/* -----------------
 * Command line type
 * -----------------
 *  An command line is defined with a name, the number of options and the
 *  list of command's options
 */
typedef struct {
#define PARSER_COMMAND_NAME_SIZE  32
  char name[PARSER_COMMAND_NAME_SIZE];  /* Command name      */
  const int nb_options;     /* Number of options     */
  parser_option_t options[];      /* Command line options    */
} parser_command_line_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void parser_print_usage(const parser_command_line_t* commamd_line);
int  parser_get_options(int argc, const char** argv,
                        parser_command_line_t* commamd_line);

#endif /* __PARSER_H__*/
