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

Lusheng Wang and
/*! \file OCG_call_emu.c
* \brief Call the emulator
* \author Lusheng Wang and Navid Nikaein
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
#include "OCG_call_emu.h"
#include "UTIL/LOG/log.h"
/*----------------------------------------------------------------------------*/

OAI_Emulation oai_emulation;

int call_emu(char dst_dir[DIR_LENGTH_MAX])
{


  ////////// print the configuration
  FILE *file;
  char dst_file[DIR_LENGTH_MAX] = "";
  strcat(dst_file, dst_dir);
  strcat(dst_file, "emulation_result.txt");
  file = fopen(dst_file,"w");
  //system("../../../openair1/SIMULATION/LTE_PHY_L2/physim --help");

  fclose(file);

  LOG_I(OCG, "Emulation finished\n");
  return MODULE_OK;
}
