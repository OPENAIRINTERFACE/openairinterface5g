/*! \file OCG_save_XML.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_SAVE_XML_H__

#define __OCG_SAVE_XML_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _save_XML Save XML
 *  @ingroup _fn
 *  @brief Save the XML configuration file in dst_dir
 * @{*/
int save_XML(int demo_or_user, char src_file[FILENAME_LENGTH_MAX + DIR_LENGTH_MAX], char dst_dir[DIR_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
