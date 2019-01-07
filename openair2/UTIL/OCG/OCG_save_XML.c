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

/*! \file OCG_save_XML.c
* \brief Save the XML configuration file in the created directory for current emulation
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
#include "OCG_save_XML.h"
#include "UTIL/LOG/log.h"
/*----------------------------------------------------------------------------*/

//int save_XML(int copy_or_move, char src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX], char dst_dir[DIR_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX]) {
int save_XML(int copy_or_move, char *src_file, char *output_dir, char *filename)
{

  FILE *fs, *ft;
  int ch;
  char dst_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX + 32] = "";
  char XML_saving_dir[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX + 32] = "";

  strncpy(dst_file, output_dir, sizeof(dst_file));
  dst_file[sizeof(dst_file) - 1] = 0; // terminate string
  //strcat(dst_file, "SCENARIO/XML/");
  strncpy(XML_saving_dir, dst_file, sizeof(XML_saving_dir));
  XML_saving_dir[sizeof(XML_saving_dir) - 1] = 0; // terminate string
  strncat(dst_file, filename, sizeof(dst_file) - strlen(dst_file) - 1);
  fs = fopen(src_file, "r");
  ft = fopen(dst_file, "w");

  if ((ft !=NULL)&&(fs!=NULL)) {
    while(1) {
      ch = getc(fs);

      if(ch == EOF) {
        break;
      } else {
        putc(ch, ft);
      }
    }

  }

  if (fs)
    fclose(fs);

  if (ft)
    fclose(ft);

  if (copy_or_move == 2) remove(src_file);

  LOG_I(OCG, "The file is saved in directory \"%s\"\n", XML_saving_dir);
  return MODULE_OK;
}
