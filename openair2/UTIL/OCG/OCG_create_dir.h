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

/*! \file OCG_create_dir.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_CREATE_DIR_H__

#define __OCG_CREATE_DIR_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _create_dir Create Dir
 *  @ingroup _fn
 *  @brief Create directory in OUTPUT_DIR for current emulation
 * @{*/
int create_dir(char output_dir[DIR_LENGTH_MAX], char user_name[FILENAME_LENGTH_MAX / 2], char file_date[FILENAME_LENGTH_MAX / 2]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
