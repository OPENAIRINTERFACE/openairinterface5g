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

#ifndef __OCG_EXTERN_H__
#define __OCG_EXTERN_H__

extern char filename[]; /*!< \brief user_name.file_date.xml */
extern char user_name[]; /*!< \brief user_name  */
extern char file_date[]; /*!< \brief file_date */
extern char src_file[]; /*!< \brief USER_XML_FOLDER/user_name.file_date.xml or DEMO_XML_FOLDER/user_name.file_date.xml */
extern char dst_dir[]; /*!< \brief user_name/file_date/ */
extern int copy_or_move; /*!< \brief indicating if the current emulation is with a local XML or an XML generated from the web portal */
extern int file_detected; /*!< \brief indicate whether a new file is detected */
/* @}*/

/** @defgroup _oks OCG Module State Indicators
 *  @ingroup _OCG
 *  @brief Indicate whether a module has processed successfully
 * @{*/
extern int get_opt_OK; /*!< \brief value: -9999, -1, 0 or 1 */
extern int detect_file_OK; /*!< \brief value: -9999, -1 or 0 */
extern int parse_filename_OK; /*!< \brief value: -9999, -1 or 0 */
extern int create_dir_OK; /*!< \brief value: -9999, -1 or 0 */
extern int parse_XML_OK; /*!< \brief value: -9999, -1 or 0 */
extern int save_XML_OK; /*!< \brief value: -9999, -1 or 0 */
extern int call_emu_OK; /*!< \brief value: -9999, -1 or 0 */
extern int config_mobi_OK; /*!< \brief value: -9999, -1 or 0 */
extern int generate_report_OK; /*!< \brief value: -9999, -1 or 0 */
/* @}*/

extern OAI_Emulation oai_emulation;

#endif
