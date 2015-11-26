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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
