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
