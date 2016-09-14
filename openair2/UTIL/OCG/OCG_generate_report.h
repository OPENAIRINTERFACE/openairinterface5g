/*! \file OCG_generate_report.h
* \brief
* \author Lusheng Wang and navid nikaein
* \date 2011
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OCG_GENERATE_REPORT_H__

#define __OCG_GENERATE_REPORT_H__

#ifdef __cplusplus
extern "C" {
#endif
/** @defgroup _generate_report Generate report
 *  @ingroup _fn
 *  @brief Generate a report to show the states of OCG modules
 * @{*/
int generate_report(char dst_dir[DIR_LENGTH_MAX], char filename[FILENAME_LENGTH_MAX]);
/* @}*/

#ifdef __cplusplus
}
#endif

#endif
