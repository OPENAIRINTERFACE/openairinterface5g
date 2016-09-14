/*! \file OCG_call_emu.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_CALL_EMU_H__

#define __OCG_CALL_EMU_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _call_emu Call Emu
 *  @ingroup _fn
 *  @brief Call the emulator
 * @{*/
int call_emu(char dst_dir[DIR_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
