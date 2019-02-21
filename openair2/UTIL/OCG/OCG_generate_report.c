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


int generate_report(char dst_dir[DIR_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX]) {
  // for the xml writer, refer to http://xmlsoft.org/html/libxml-xmlwriter.html
  char dst_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX];
  strncpy(dst_file, dst_dir, FILENAME_LENGTH_MAX + DIR_LENGTH_MAX - strlen(filename) - 1);
  strcat(dst_file, filename);
  xmlTextWriterPtr writer;
  writer = xmlNewTextWriterFilename(dst_file, 0);
  // set the output format of the XML file
  xmlTextWriterSetIndent(writer, 1);
  xmlTextWriterSetIndentString(writer,(unsigned char *) "	");
  xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
  /* Write an element named "X_ORDER_ID" as child of HEADER. */
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "COMMENT           ", "	in this output file, %d means NOT_PROCESSED; %d means NO_FILE; %d means ERROR; %d means OK	", MODULE_NOT_PROCESSED,
                                  NO_FILE, MODULE_ERROR, MODULE_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "OCG_GET_OPT       ", "	%d	", get_opt_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "OCG_DETECT_FILE   ", "	%d	", detect_file_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "OCG_PARSE_FILENAME", "	%d	", parse_filename_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "OCG_CREATE_DIR    ", "	%d	", create_dir_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "OCG_PARSE_XML     ", "	%d	", parse_XML_OK);
  xmlTextWriterWriteFormatElement(writer,(unsigned char *) "OCG_SAVE_XML      ", "	%d	", save_XML_OK);
  //  xmlTextWriterWriteFormatElement(writer, "OCG_CALL_EMU      ", " %d  ", call_emu_OK);
  xmlTextWriterEndDocument(writer);
  xmlFreeTextWriter(writer);
  LOG_I(OCG, "A report of OCG is generated in directory \"%s\"\n\n", dst_dir);
  return MODULE_OK;
}
