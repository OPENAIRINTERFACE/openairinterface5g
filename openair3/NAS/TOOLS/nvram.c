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

/*****************************************************************************
Source    usim_data.c

Version   0.1

Date    2012/10/31

Product   NVRAM data generator

Subsystem NVRAM data generator main process

Author    Frederic Maurel

Description Implements the utility used to generate data stored in the
    NVRAM application

 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "conf_parser.h"
#include "display.h"

#define DEFAULT_NAS_PATH "PWD"
#define OUTPUT_DIR_ENV "NVRAM_DIR"
void _display_usage(const char* command);

int main (int argc, char * const argv[])
{
  enum usim_command {
    NVRAM_COMMAND_NONE,
    NVRAM_COMMAND_PRINT,
    NVRAM_COMMAND_GEN,
  } command = NVRAM_COMMAND_NONE;

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
        command = NVRAM_COMMAND_GEN;
        break;
      case 'p':
        command = NVRAM_COMMAND_PRINT;
        break;
      case 'o':
        output_dir = optarg;
        break;
      default:
        break;
    }
  }

  if ( command == NVRAM_COMMAND_NONE ) {
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

  if ( command == NVRAM_COMMAND_GEN ) {
    if ( conf_file == NULL ) {
      printf("No Configuration file is given\n");
      _display_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

    if ( parse_config_file(output_dir, conf_file, OUTPUT_UEDATA|OUTPUT_EMM) == false ) {
      exit(EXIT_FAILURE);
    }
  }

  if ( display_data_from_directory(output_dir, DISPLAY_UEDATA|DISPLAY_EMM) == 0) {
    fprintf(stderr, "No NVRAM files found in %s\n", output_dir);
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
  fprintf(stderr, "\t[--gen|-g]\tGenerate the NVRAM data file\n");
  fprintf(stderr, "\t[--print|-p]\tDisplay the content of the NVRAM data file\n");
	fprintf(stderr, "\t[-c]\tConfig file to use\n");
	fprintf(stderr, "\t[-o]\toutput file directory\n");
  fprintf(stderr, "\t[--help|-h]\tDisplay this usage\n");
  const char* path = getenv("NVRAM_DIR");

  if (path != NULL) {
    fprintf(stderr, "NVRAM_DIR = %s\n", path);
  } else {
    fprintf(stderr, "NVRAM_DIR environment variable is not defined\n");
  }
}
