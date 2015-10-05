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
Source      nas_parser.c

Version     0.1

Date        2012/02/27

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS command line parser

*****************************************************************************/

#include "nas_parser.h"
#include "parser.h"

#include <stdio.h>  // fprintf
#include <stdlib.h> // atoi

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * Identifiers of the NAS command line options
 */
enum {
  NAS_PARSER_TRACE_LEVEL,     /* Logging trace level      */
  NAS_PARSER_NETWORK_HOST,        /* Network layer's hostname */
  NAS_PARSER_NETWORK_PORT,        /* Network layer's port number  */
  NAS_PARSER_NB_OPTIONS
};

/* -----------------------------------------------------
 * Definition of the internal NAS command line structure
 * -----------------------------------------------------
 *  The command line is defined with a name (default is "NASprocess"
 *  but it will be replaced by the command name actually used at
 *  runtime), a number of options and the list of options.
 *  An option is defined with a name, an argument following the name,
 *  the usage displayed by the usage function and a default value.
 */
static parser_command_line_t nasParserCommandLine = {
  "NASprocess",           /* Command name         */
  NAS_PARSER_NB_OPTIONS,      /* Number of options        */
  {                 /* Command line options     */
    {
      "-trace", "<mask>", "Logging trace level\t\t",
      NAS_PARSER_DEFAULT_TRACE_LEVEL
    },
    {
      "-nhost", "<nhost>", "Network layer's hostname\t",
      NAS_PARSER_DEFAULT_NETWORK_HOSTNAME
    },
    {
      "-nport", "<nport>", "Network layer's port number\t",
      NAS_PARSER_DEFAULT_NETWORK_PORT_NUMBER
    },
  }
};

/*
 * Converts a string, containing hexadecimal formatted integer, to an integer
 */
static int atohex(const char *a_char);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    nas_parser_print_usage()                                  **
 **                                                                        **
 ** Description: Displays the command line options used to run the NAS     **
 **      process and the firmware version defined at compilation   **
 **      time                                                      **
 **                                                                        **
 ** Inputs:  version:   Firmware version                           **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     Return:    None                                       **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void nas_parser_print_usage(const char *version)
{
  parser_print_usage(&nasParserCommandLine);
  fprintf(stderr, "Version: %s\n", version);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_parser_get_options()                                  **
 **                                                                        **
 ** Description: Gets the command line options used to run the NAS process **
 **                                                                        **
 ** Inputs:  argc:      Number of options                          **
 **          argv:      Pointer to the list of options             **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     Return:    RETURNerror, RETURNok                      **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_parser_get_options(int argc, const char **argv)
{
  return parser_get_options(argc, argv, &nasParserCommandLine);
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_parser_get_nb_options()                               **
 **                                                                        **
 ** Description: Returns the number of the command line options used to    **
 **      run the NAS process                                       **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     Return:    Number of command line options             **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_parser_get_nb_options(void)
{
  return nasParserCommandLine.nb_options;
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_parser_get_trace_level()                              **
 **                                                                        **
 ** Description: Returns the value of the logging trace level              **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     Return:    Value of the logging trace level           **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_parser_get_trace_level(void)
{
  return atohex( nasParserCommandLine.options[NAS_PARSER_TRACE_LEVEL].pvalue );
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_parser_get_network_host()                             **
 **                                                                        **
 ** Description: Returns the value of the network layer hostname           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     Return:    Value of the network layer hostname        **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *nas_parser_get_network_host(void)
{
  return nasParserCommandLine.options[NAS_PARSER_NETWORK_HOST].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:    nas_parser_get_network_port()                             **
 **                                                                        **
 ** Description: Returns the value of the network layer port number        **
 **                                                                        **
 ** Inputs:  None                                                      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     Return:    Value of the network layer port number     **
 **          Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *nas_parser_get_network_port(void)
{
  return nasParserCommandLine.options[NAS_PARSER_NETWORK_PORT].pvalue;
}



/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

static int atohex(const char *a_char)
{
  int result;

  for (result=0; a_char; a_char++) {
    if (*a_char >= '0' && *a_char <= '9') {
      result = (result << 4) + (*a_char - '0');
    } else if (*a_char >= 'A' && *a_char <= 'F') {
      result = (result << 4) + (*a_char - 'A' + 10);
    } else if (*a_char >= 'a' && *a_char <= 'f') {
      result = (result << 4) + (*a_char - 'a' + 10);
    } else {
      break;
    }
  }

  return result;
}
