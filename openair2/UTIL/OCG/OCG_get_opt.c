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
