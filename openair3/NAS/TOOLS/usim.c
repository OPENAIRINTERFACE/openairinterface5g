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

 Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source    usim_data.c

Version   0.1

Date    2012/10/31

Product   USIM data generator

Subsystem USIM data generator main process

Author    Frederic Maurel

Description Implements the utility used to generate data stored in the
    USIM application

 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "conf_parser.h"
#include "display.h"

#define DEFAULT_NAS_PATH "PWD"
#define OUTPUT_DIR_ENV "USIM_DIR"
void _display_usage(const char* command);

int main (int argc, char * const argv[])
{
  enum usim_command {
    USIM_COMMAND_NONE,
    USIM_COMMAND_PRINT,
    USIM_COMMAND_GEN,
  } command = USIM_COMMAND_NONE;

  char *output_dir = NULL;
  char *conf_file = NULL;
  const char options[]="gpc:o:h";
  const struct option options_long_option[] = {
    {"gen",    no_argument, NULL, 'g'},
    {"print",  no_argument, NULL, 'p'},
    {"conf",   required_argument, NULL, 'c'},
    {"output", required_argument, NULL, 'o'},
    {"help",   no_argument, NULL, 'h'},
    {NULL,     0,           NULL, 0}
  };
  int option_index;
  char option_short;

  /*
   * Read command line parameters
   */
  while ( true ) {
    option_short = getopt_long(argc, argv, options, options_long_option, &option_index );

    if ( option_short == -1 )
      break;

    switch (option_short) {
      case 'c':
        conf_file = optarg;
        break;
      case 'g':
        command = USIM_COMMAND_GEN;
        break;
      case 'p':
        command = USIM_COMMAND_PRINT;
        break;
      case 'o':
        output_dir = optarg;
        break;
      default:
        break;
    }
  }

  if ( command == USIM_COMMAND_NONE ) {
    _display_usage(argv[0]);
    exit(EXIT_SUCCESS);
  }

  /* compute default data directory if no output_dir is given */
  if ( output_dir == NULL ) {
    output_dir = getenv(OUTPUT_DIR_ENV);

    if (output_dir == NULL) {
      output_dir = getenv(DEFAULT_NAS_PATH);
    }

    if (output_dir == NULL) {
      fprintf(stderr, "%s and %s environment variables are not defined trying local directory",
              OUTPUT_DIR_ENV, DEFAULT_NAS_PATH);
      output_dir = ".";
    }
  }

  if ( command == USIM_COMMAND_GEN ) {
    if ( conf_file == NULL ) {
      printf("No Configuration file is given\n");
      _display_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

    if ( parse_config_file(output_dir, conf_file, OUTPUT_USIM) == false ) {
      exit(EXIT_FAILURE);
    }
  }

  if ( display_data_from_directory(output_dir, DISPLAY_USIM) == 0) {
    fprintf(stderr, "No USIM files found in %s\n", output_dir);
  }

  exit(EXIT_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * Displays command line usage
 */
void _display_usage(const char* command)
{
  fprintf(stderr, "usage: %s [OPTION]\n", command);
  fprintf(stderr, "\t[--gen|-g]\tGenerate the USIM data file\n");
  fprintf(stderr, "\t[--print|-p]\tDisplay the content of the USIM data file\n");
	fprintf(stderr, "\t[-c]\tConfig file to use\n");
	fprintf(stderr, "\t[-o]\toutput file directory\n");
  fprintf(stderr, "\t[--help|-h]\tDisplay this usage\n");
  const char* path = getenv("USIM_DIR");

  if (path != NULL) {
    fprintf(stderr, "USIM_DIR = %s\n", path);
  } else {
    fprintf(stderr, "USIM_DIR environment variable is not defined\n");
  }
}
