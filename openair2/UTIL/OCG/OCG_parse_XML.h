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
