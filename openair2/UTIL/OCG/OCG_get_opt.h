/*! \file OCG_get_opt.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/
#ifndef __OCG_GET_OPT_H__

#define __OCG_GET_OPT_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _get_opt Get Opt
 *  @ingroup _fn
 *  @brief Get options of the OCG command, e.g. "OCG -f" and "OCG -h"
 * @{*/
int get_opt(int argc, char *argv[]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
