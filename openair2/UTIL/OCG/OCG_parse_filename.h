/*! \file OCG_parse_filename.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_PARSE_FILENAME_H__

#define __OCG_PARSE_FILENAME_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _parse_filename Parse Filename
 *  @ingroup _fn
 *  @brief Parse the filename to get user_name and file_date
 * @{*/
int parse_filename(char filename[FILENAME_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
