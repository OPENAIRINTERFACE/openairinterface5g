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

/*! \file OCG_get_opt.c
* \brief Get Options of the OCG command
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

/*--- INCLUDES ---------------------------------------------------------------*/
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "OCG.h"
#include "OCG_get_opt.h"
//#include "log.h"
/*----------------------------------------------------------------------------*/

char filename[FILENAME_LENGTH_MAX];

int get_opt(int argc, char *argv[])
{
  char opts;

  while((opts = getopt(argc, argv, "f:h")) != -1) {

    switch (opts) {
    case 'f' :
      strcpy(filename, optarg);
      LOG_D(OCG, "User specified configuration file is \"%s\"\n", filename);
      return MODULE_OK;

    case 'h' :
      LOG_I(OCG, "OCG command :	OCG -f \"filename.xml\"\n");
      return GET_HELP;

    default :
      LOG_E(OCG, "OCG command :	OCG -f \"filename.xml\"\n");
      return GET_HELP;
    }
  }

  return NO_FILE;
}
