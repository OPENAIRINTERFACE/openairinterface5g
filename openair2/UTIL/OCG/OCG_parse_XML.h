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

/*! \file OCG_parse_XML.h
* \brief Variables indicating the element which is currently parsed
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_PARSE_XML_H__

#define __OCG_PARSE_XML_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _parsing_position_indicator Parsing Position Indicator
 *  @ingroup _parse_XML
 *  @brief Indicate the position where the program is current parsing in the XML file
 * @{*/

/* @}*/

/** @defgroup _parse_XML Parse XML
 *  @ingroup _fn
 *  @brief Parse the XML configuration file
 * @{*/
int parse_XML(char src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
