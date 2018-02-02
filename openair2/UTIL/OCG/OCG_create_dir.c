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

/*! \file OCG_create_dir.c
* \brief Create directory for current emulation
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
#include <sys/stat.h>
#include "OCG_vars.h"
#include "OCG_create_dir.h"
#include "UTIL/LOG/log.h"
/*----------------------------------------------------------------------------*/

int create_dir(char output_dir[DIR_LENGTH_MAX], char user_name[FILENAME_LENGTH_MAX / 2], char file_date[FILENAME_LENGTH_MAX / 2])
{

  char directory[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX] = "";
  mode_t process_mask = umask(0);

  strncpy(directory, output_dir, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX);
  directory[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - 1] = 0; // terminate string

  struct stat st;

  if(stat(directory, &st) != 0) { // if output_dir does not exist, we create it here
    mkdir(directory, S_IRWXU | S_IRWXG | S_IRWXO);
    LOG_I(OCG, "output_dir %s is created", directory);
  }

  strncat(directory, user_name, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(directory) - 1);

  mkdir(directory, S_IRWXU | S_IRWXG | S_IRWXO);

  strncat(directory, "/", FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(directory) - 1);
  strncat(directory, file_date, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(directory) - 1);

  mkdir(directory, S_IRWXU | S_IRWXG |S_IRWXO);

  //char directory_extension[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX + 64] = "";
  /*strcpy(directory_extension, directory); // to create some more folders
  strcat(directory_extension, "/LOGS");
  mkdir(directory_extension, S_IRWXU | S_IRWXG |S_IRWXO);

  strcpy(directory_extension, directory);
  strcat(directory_extension, "/PACKET_TRACE");
  mkdir(directory_extension, S_IRWXU | S_IRWXG |S_IRWXO);

  strcpy(directory_extension, directory);
  strcat(directory_extension, "/SCENARIO");
  mkdir(directory_extension, S_IRWXU | S_IRWXG |S_IRWXO);

  strcpy(directory_extension, directory);
  strcat(directory_extension, "/SCENARIO/XML");
  mkdir(directory_extension, S_IRWXU | S_IRWXG |S_IRWXO);

  strcpy(directory_extension, directory);
  strcat(directory_extension, "/SCENARIO/STATE");
  mkdir(directory_extension, S_IRWXU | S_IRWXG |S_IRWXO);
  */
  umask(process_mask);

  LOG_I(OCG, "Directory for current emulation is created\n");
  return MODULE_OK;

}
