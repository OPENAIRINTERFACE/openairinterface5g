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
