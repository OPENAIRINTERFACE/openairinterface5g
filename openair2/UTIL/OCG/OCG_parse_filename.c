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

/*! \file OCG_parse_filename.c
* \brief Parse the filename of the XML file
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

/*--- INCLUDES ---------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "OCG_vars.h"
#include "OCG_parse_filename.h"
#include "UTIL/LOG/log.h"

/*----------------------------------------------------------------------------*/

int parse_filename(char filename[FILENAME_LENGTH_MAX])
{
  char *delim = "._";
  char tmp_filename[FILENAME_LENGTH_MAX];
  char *fd_tmp;
  char *un_tmp;
  char *ex_tmp;

  //delim = "._";
  strncpy(tmp_filename, filename, FILENAME_LENGTH_MAX);
  tmp_filename[FILENAME_LENGTH_MAX - 1] = 0; // terminate string

  un_tmp = strtok(tmp_filename, delim);
  fd_tmp = strtok(NULL, delim);
  ex_tmp = strtok(NULL, delim);

  if ((ex_tmp == NULL) || ((strcmp(ex_tmp, "xml")) && (strcmp(ex_tmp, "XML")))) {
    LOG_E(OCG,
          "Please use .xml file for configuration with the format \"user_name.file_date.xml\"\nfile_date = \"year month day hour minute second\" without space, \ne.g. 20100201193045 represents in the year 2010, February 1st, 19:30:45\n");
    return MODULE_ERROR;
  } else {
    strncpy(file_date, fd_tmp, sizeof(file_date));
    file_date[sizeof(file_date) - 1] = 0; // terminate string
    strncpy(user_name, un_tmp, sizeof(user_name));
    user_name[sizeof(user_name) - 1] = 0; // terminate string

    LOG_I(OCG, "File name is parsed as user_name = %s, file_date = %s\n", user_name, file_date);
    return MODULE_OK;
  }
}
