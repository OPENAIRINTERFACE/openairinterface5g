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

#ifndef __OCG_VARS_H__
#define __OCG_VARS_H__

#include "OCG.h"

/** @defgroup _main_val Main Variables
 *  @ingroup _OCG
 *  @brief Variables used by the main function
 * @{*/
char filename[FILENAME_LENGTH_MAX]; /*!< \brief user_name.file_date.xml */
char user_name[FILENAME_LENGTH_MAX / 2]; /*!< \brief user_name  */
char file_date[FILENAME_LENGTH_MAX / 2]; /*!< \brief file_date */
char src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX]; /*!< \brief USER_XML_FOLDER/user_name.file_date.xml or DEMO_XML_FOLDER/user_name.file_date.xml */
char dst_dir[DIR_LENGTH_MAX]; /*!< \brief user_name/file_date/ */
int copy_or_move; /*!< \brief indicating if the current emulation is with a local XML or an XML generated from the web portal */
int file_detected; /*!< \brief indicate whether a new file is detected */
/* @}*/

/** @defgroup _oks OCG Module State Indicators
 *  @ingroup _OCG
 *  @brief Indicate whether a module has processed successfully
 * @{*/
int get_opt_OK; /*!< \brief value: -9999, -1, 0 or 1 */
int detect_file_OK; /*!< \brief value: -9999, -1 or 0 */
int parse_filename_OK; /*!< \brief value: -9999, -1 or 0 */
int create_dir_OK; /*!< \brief value: -9999, -1 or 0 */
int parse_XML_OK; /*!< \brief value: -9999, -1 or 0 */
int save_XML_OK; /*!< \brief value: -9999, -1 or 0 */
int call_emu_OK; /*!< \brief value: -9999, -1 or 0 */
int config_mobi_OK; /*!< \brief value: -9999, -1 or 0 */
int generate_report_OK; /*!< \brief value: -9999, -1 or 0 */

OAI_Emulation oai_emulation;


/* @}*/
#endif

