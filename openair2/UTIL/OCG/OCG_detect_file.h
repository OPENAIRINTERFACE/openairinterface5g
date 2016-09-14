/*! \file OCG_detect_file.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_DETECT_FILE_H__

#define __OCG_DETECT_FILE_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _detect_file Detect File
 *  @ingroup _fn
 *  @brief Detect new XML configuration file in USER_XML_FOLDER
 * @{*/
//int detect_file(int argc, char *argv[], char folder[DIR_LENGTH_MAX]);
int detect_file(char folder[DIR_LENGTH_MAX], char is_local_server[FILENAME_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
