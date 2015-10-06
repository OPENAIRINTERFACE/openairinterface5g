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
