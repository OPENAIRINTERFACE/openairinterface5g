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
