/*! \file OCG_config_mobi.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_CONFIG_MOBI_H__

#define __OCG_CONFIG_MOBI_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _config_mobi Config Mobigen
 *  @ingroup _fn
 *  @brief Generate configuration XML for mobigen
 * @{*/
int config_mobi(char mobigen_filename[FILENAME_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
