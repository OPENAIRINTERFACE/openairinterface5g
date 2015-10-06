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

/*! \file OCG_generate_report.c
* \brief Generate a brief report for debug of OCG
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
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include "OCG_vars.h"
#include "OCG_generate_report.h"
#include "UTIL/LOG/log.h"

/*----------------------------------------------------------------------------*/


int generate_report(char dst_dir[DIR_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX])
{
  // for the xml writer, refer to http://xmlsoft.org/html/libxml-xmlwriter.html

  char dst_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX] = "";
  strncat(dst_file, dst_dir, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(dst_file) - 1);
  strncat(dst_file, filename, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(dst_file) - 1);

  xmlTextWriterPtr writer;

  writer = xmlNewTextWriterFilename(dst_file, 0);

  // set the output format of the XML file
  xmlTextWriterSetIndent(writer, 1);
  xmlTextWriterSetIndentString(writer,(unsigned char*) "	");

  xmlTextWriterStartDocument(writer, NULL, NULL, NULL);

  /* Write an element named "X_ORDER_ID" as child of HEADER. */
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "COMMENT           ", "	in this output file, %d means NOT_PROCESSED; %d means NO_FILE; %d means ERROR; %d means OK	", MODULE_NOT_PROCESSED,
                                  NO_FILE, MODULE_ERROR, MODULE_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "OCG_GET_OPT       ", "	%d	", get_opt_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "OCG_DETECT_FILE   ", "	%d	", detect_file_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "OCG_PARSE_FILENAME", "	%d	", parse_filename_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "OCG_CREATE_DIR    ", "	%d	", create_dir_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "OCG_PARSE_XML     ", "	%d	", parse_XML_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char*) "OCG_SAVE_XML      ", "	%d	", save_XML_OK);
  //  xmlTextWriterWriteFormatElement(writer, "OCG_CALL_EMU      ", " %d  ", call_emu_OK);

  xmlTextWriterEndDocument(writer);

  xmlFreeTextWriter(writer);

  LOG_I(OCG, "A report of OCG is generated in directory \"%s\"\n\n", dst_dir);
  return MODULE_OK;
}
