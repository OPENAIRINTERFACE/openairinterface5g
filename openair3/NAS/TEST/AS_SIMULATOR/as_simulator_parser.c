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
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_simulator_parser.c

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem Command line parser

Author    Frederic Maurel

Description Command line parser of the AS Simulator process

*****************************************************************************/

#include "as_simulator_parser.h"

#include "parser.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * Identifiers of the AS Simulator command line options
 */
enum {
  AS_SIMULATOR_PARSER_UE_HOST,  /* UE process hostname    */
  AS_SIMULATOR_PARSER_UE_PORT,  /* UE process port number */
  AS_SIMULATOR_PARSER_MME_HOST, /* MME process hostname   */
  AS_SIMULATOR_PARSER_MME_PORT, /* MME process port number  */
  AS_SIMULATOR_PARSER_NB_OPTIONS
};

/* --------------------------------------------------------------
 * Definition of the internal AS Simulator command line structure
 * --------------------------------------------------------------
 *  The command line is defined with a name (default is "ASprocess"
 *  but it will be replaced by the command name actually used at
 *  runtime), a number of options and the list of options.
 *  An option is defined with a name, an argument following the name,
 *  the usage displayed by the usage function and a default value.
 */
static parser_command_line_t asParserCommandLine = {
  "ASprocess",      /* Command name     */
  AS_SIMULATOR_PARSER_NB_OPTIONS, /* Number of options    */
  {         /* Command line options   */
    {
      "-uhost", "<uhost>", "UE process hostname\t\t",
      AS_SIMULATOR_PARSER_DEFAULT_UE_HOSTNAME
    },
    {
      "-uport", "<uport>", "UE process port number\t\t",
      AS_SIMULATOR_PARSER_DEFAULT_UE_PORT_NUMBER
    },
    {
      "-mhost", "<mhost>", "MME process hostname\t\t",
      AS_SIMULATOR_PARSER_DEFAULT_MME_HOSTNAME
    },
    {
      "-mport", "<mport>", "MME process port number\t\t",
      AS_SIMULATOR_PARSER_DEFAULT_MME_PORT_NUMBER
    },
  }
};

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_print_usage()                         **
 **                                                                        **
 ** Description: Displays the command line options used to run the AS      **
 **    Simulator process                                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ** Outputs:   Return:  None                                       **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
inline void as_simulator_parser_print_usage(void)
{
  parser_print_usage(&asParserCommandLine);
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_get_options()                         **
 **                                                                        **
 ** Description: Gets the command line options used to run the AS          **
 **    Simulator process                                         **
 **                                                                        **
 ** Inputs:  argc:    Number of options                          **
 **      argv:    Pointer to the list of options             **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  RETURNerror, RETURNok                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ***************************************************************************/
inline int as_simulator_parser_get_options(int argc, const char** argv)
{
  return parser_get_options(argc, argv, &asParserCommandLine);
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_get_nb_options()                      **
 **                                                                        **
 ** Description: Returns the number of the command line options used to    **
 **    run the As Simulator process                              **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ** Outputs:   Return:  Number of command line options             **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
inline int as_simulator_parser_get_nb_options(void)
{
  return asParserCommandLine.nb_options;
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_get_uhost()                           **
 **                                                                        **
 ** Description: Returns the value of the UE process hostname              **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ** Outputs:   Return:  Value of the remote hostname               **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
inline const char* as_simulator_parser_get_uhost(void)
{
  return asParserCommandLine.options[AS_SIMULATOR_PARSER_UE_HOST].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_get_uport()                           **
 **                                                                        **
 ** Description: Returns the value of the UE process port number           **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ** Outputs:   Return:  Value of the remote port number            **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
inline const char* as_simulator_parser_get_uport(void)
{
  return asParserCommandLine.options[AS_SIMULATOR_PARSER_UE_PORT].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_get_mhost()                           **
 **                                                                        **
 ** Description: Returns the value of the MME process hostname             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ** Outputs:   Return:  Value of the remote hostname               **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
inline const char* as_simulator_parser_get_mhost(void)
{
  return asParserCommandLine.options[AS_SIMULATOR_PARSER_MME_HOST].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_simulator_parser_get_mport()                           **
 **                                                                        **
 ** Description: Returns the value of the MME process port number          **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  asParserCommandLine.options                **
 **                                                                        **
 ** Outputs:   Return:  Value of the remote port number            **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
inline const char* as_simulator_parser_get_mport(void)
{
  return asParserCommandLine.options[AS_SIMULATOR_PARSER_MME_PORT].pvalue;
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

