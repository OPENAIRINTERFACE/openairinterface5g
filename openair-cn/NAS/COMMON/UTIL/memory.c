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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source    memory.c

Version   0.1

Date    2012/10/09

Product   NAS stack

Subsystem Utilities

Author    Frederic Maurel

Description Memory access utilities

*****************************************************************************/

#include "memory.h"
#include "commonDef.h"
#include "nas_log.h"

#include <stdio.h>  // fopen, fread, fclose
#include <stdlib.h> // getenv, malloc, free
#include <string.h> // strlen

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  memory_get_path()                                         **
 **                                                                        **
 ** Description: Gets the absolute path of the file where non-volatile     **
 **    data are located                                          **
 **                                                                        **
 ** Inputs:  dirname: The directory where data file is located   **
 **    filename:  The name of the data file                  **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  The absolute path of the non-volatile data **
 **       file. The returned value is a dynamically  **
 **       allocated octet string that needs to be    **
 **       freed after usage.                         **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
char* memory_get_path(const char* dirname, const char* filename)
{
  /* Get non-volatile data directory */
  const char* path = getenv(dirname);

  if (path == NULL) {
    path = getenv(DEFAULT_NAS_PATH);
  }

  if (path == NULL) {
    LOG_TRACE(WARNING, "MEMORY  - %s and %s environment variables are not defined trying local directory", dirname, DEFAULT_NAS_PATH);
    path = ".";
  }

  /* Append non-volatile data file name */
  size_t size = strlen(path) + strlen(filename) + 1;
  char* data_filename = (char*)malloc(size+1);

  if (data_filename != NULL) {
    if (size != sprintf(data_filename, "%s/%s", path, filename)) {
      free(data_filename);
      return NULL;
    }
  }

  return data_filename;
}

/****************************************************************************
 **                                                                        **
 ** Name:  memory_read()                                             **
 **                                                                        **
 ** Description: Reads data from a non-volatile data file                  **
 **                                                                        **
 ** Inputs:  datafile:  The absolute path to the data file         **
 **    size:    The size of the data to read               **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   data:    Pointer to the data read                   **
 **    Return:  RETURNerror, RETURNok                      **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int memory_read(const char* datafile, void* data, size_t size)
{
  int rc = RETURNerror;

  /* Open the data file for reading operation */
  FILE* fp = fopen(datafile, "rb");

  if (fp != NULL) {
    /* Read data */
    size_t n = fread(data, size, 1, fp);

    if (n == 1) {
      rc = RETURNok;
    }

    /* Close the data file */
    fclose(fp);
  }

  return (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:  memory_write()                                            **
 **                                                                        **
 ** Description: Writes data to a non-volatile data file                   **
 **                                                                        **
 ** Inputs:  datafile:  The absolute path to the data file         **
 **    data:    Pointer to the data to write               **
 **    size:    The size of the data to write              **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   None                                                      **
 **    Return:  RETURNerror, RETURNok                      **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int memory_write(const char* datafile, const void* data, size_t size)
{
  int rc = RETURNerror;

  /* Open the data file for writing operation */
  FILE* fp = fopen(datafile, "wb");

  if (fp != NULL) {
    /* Write data */
    size_t n = fwrite(data, size, 1, fp);

    if (n == 1) {
      rc = RETURNok;
    }

    /* Close the data file */
    fclose(fp);
  }

  return (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

