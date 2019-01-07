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

Source    user_parser.c

Version   0.1

Date    2012/10/09

Product   User Simulator

Subsystem Command line parser

Author    Frederic Maurel

Description Command line parser of the User Simulator process

*****************************************************************************/

#include "user_parser.h"

#include "util/parser.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * Identifiers of the User Simulator command line options
 */
enum {
  USER_PARSER_HOST,   /* Remote hostname    */
  USER_PARSER_PORT,   /* Remote port number   */
  USER_PARSER_DEVPATH,  /* Device pathname    */
  USER_PARSER_DEVATTR,  /* Device attribute parameters  */
  USER_PARSER_NB_OPTIONS
};

/* ----------------------------------------------------------------
 * Definition of the internal User Simulator command line structure
 * ----------------------------------------------------------------
 *  The command line is defined with a name (default is "UserProcess"
 *  but it will be replaced by the command name actually used at
 *  runtime), a number of options and the list of options.
 *  An option is defined with a name, an argument following the name,
 *  the usage displayed by the usage function and a default value.
 */
static parser_command_line_t userParserCommandLine = {
  "UserProcess",      /* Command name     */
  USER_PARSER_NB_OPTIONS,   /* Number of options    */
  {         /* Command line options   */
    {"-host", "<host>", "\tRemote hostname\t\t", USER_PARSER_DEFAULT_REMOTE_HOSTNAME},
    {"-port", "<port>", "\tRemote port number\t", USER_PARSER_DEFAULT_REMOTE_PORT_NUMBER},
    {"-dev", "<devpath>", "Device pathname\t\t", "NULL"},
    {"-params", "<params>", "Device attribute parameters", "NULL"},
  }
};

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_print_usage()                                 **
 **                                                                        **
 ** Description: Displays the command line options used to run the User    **
 **    Simulator process                                         **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  None                                       **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
void user_parser_print_usage(void)
{
  parser_print_usage(&userParserCommandLine);
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_get_options()                                 **
 **                                                                        **
 ** Description: Gets the command line options used to run the User        **
 **    Simulator process                                         **
 **                                                                        **
 ** Inputs:  argc:    Number of options                          **
 **      argv:    Pointer to the list of options             **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  RETURNerror, RETURNok                      **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int user_parser_get_options(int argc, const char** argv)
{
  return parser_get_options(argc, argv, &userParserCommandLine);
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_get_nb_options()                              **
 **                                                                        **
 ** Description: Returns the number of the command line options used to    **
 **    run the User Simulator process                            **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  Number of command line options             **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int user_parser_get_nb_options(void)
{
  return userParserCommandLine.nb_options;
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_get_host()                                    **
 **                                                                        **
 ** Description: Returns the value of the remote hostname                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  Value of the remote hostname               **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
const char* user_parser_get_host(void)
{
  return userParserCommandLine.options[USER_PARSER_HOST].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_get_port()                                    **
 **                                                                        **
 ** Description: Returns the value of the remote port number               **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  Value of the remote port number            **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
const char* user_parser_get_port(void)
{
  return userParserCommandLine.options[USER_PARSER_PORT].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_get_devpath()                                 **
 **                                                                        **
 ** Description: Returns the value of the device pathname                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  Value of the device pathname               **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
const char* user_parser_get_devpath(void)
{
  return userParserCommandLine.options[USER_PARSER_DEVPATH].pvalue;
}

/****************************************************************************
 **                                                                        **
 ** Name:  user_parser_get_devattr()                                 **
 **                                                                        **
 ** Description: Returns the value of the device attribute parameters      **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:  None                                       **
 **                                                                        **
 ** Outputs:   Return:  Value of the device attribute parameters   **
 **      Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
const char* user_parser_get_devattr(void)
{
  return userParserCommandLine.options[USER_PARSER_DEVATTR].pvalue;
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

