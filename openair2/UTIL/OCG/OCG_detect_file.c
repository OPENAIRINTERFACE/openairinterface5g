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

/*! \file OCG_detect_file.c
* \brief Detect if a new XML is generated from the web portal
* \author Lusheng Wang  & Navid Nikaein
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
#include <dirent.h>
#include <sys/stat.h>
#include "OCG_vars.h"
#include "OCG_detect_file.h"
#include "UTIL/LOG/log.h"
/*----------------------------------------------------------------------------*/


int detect_file(char src_dir[DIR_LENGTH_MAX], char is_local_server[FILENAME_LENGTH_MAX])
{
  DIR *dir = NULL;
  struct dirent *file = NULL;
  char template[FILENAME_LENGTH_MAX] = "";
  char manual[FILENAME_LENGTH_MAX] = "";

  if((dir = opendir(src_dir)) == NULL) {
    LOG_E(OCG, "Folder %s for detecting the XML configuration file is not found\n", src_dir);
    return MODULE_ERROR;
  }

  while((file = readdir(dir)) != NULL) {
    if(strcmp(file->d_name, ".") && strcmp(file->d_name, "..")) {
      if(file->d_type != DT_DIR) {
        if ((!strcmp(is_local_server, "0")) || (!strcmp(is_local_server, "-1"))) { // for EURECOM web or local user without specifying the file name
          strncpy(template, file->d_name, sizeof("template") - 1);
          strncpy(manual, file->d_name, sizeof("manual") - 1);

          if ((!strcmp(template, "template")) || (!strcmp(manual, "manual"))) { // should skip the templates and the manual XML files

          } else {
            if (strlen(file->d_name) <= FILENAME_LENGTH_MAX) {
              strcpy(filename, file->d_name);
              LOG_I(OCG, "Configuration file \"%s\" is detected\n", filename);
              closedir(dir);
              return MODULE_OK; // find a good file and return
            } else {
              LOG_E(OCG, "File name too long: char filename[] should be less than 64 characters\n");
              closedir(dir);
              return MODULE_ERROR;
            }
          }
        } else { // should use a template or a manual XML file
          strncpy(template, is_local_server, sizeof("template") - 1);
          strncpy(manual, is_local_server, sizeof("manual") - 1);

          if ((!strcmp(template, "template")) || (!strcmp(manual, "manual"))) {
            strcpy(filename, is_local_server);
          } else {
            strcpy(filename, "template_");
            strcat(filename, is_local_server);
            strcat(filename, ".xml");
          }

          char check_src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX];
          strncpy(check_src_file, src_dir, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX);
          check_src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - 1] = 0; // terminate string
          strncat(check_src_file, filename, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(check_src_file) - 1);
          struct stat st;

          if(stat(check_src_file, &st) != 0) {
            LOG_E(OCG, "file %s does not exist\n", check_src_file);
            closedir(dir);
            return MODULE_ERROR;
          } else {
            LOG_I(OCG, "template/manual file \"%s\" is used\n", filename);
            closedir(dir);
            return MODULE_OK;
          }
        }
      } else { // this option is not used at this moment : NO directory should be put in the src_dir
        //detect_file(strncat(src_dir, file->d_name, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX));
      }
    }
  }

  closedir(dir);

  if (strcmp(is_local_server, "0") && strcmp(is_local_server, "-1")) {
    LOG_E(OCG, "file %s does not exist in directory %s\n", is_local_server, src_dir);
    return MODULE_ERROR;
  } else return NO_FILE;
}
