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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*! \file gtpv1u.h
* \brief
* \author Sebastien ROUX, Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef GTPV1_U_H_
#define GTPV1_U_H_

#define GTPU_STACK_ENB 0
#define GTPU_STACK_SGW 1

/* When gtpv1u is compiled for eNB use MACRO from UTILS/log.h,
 * otherwise use standard fprintf as logger.
 */
#if defined(ENB_MODE)
# define GTPU_DEBUG(x, args...)   LOG_D(GTPU, x, ##args)
# define GTPU_INFO(x, args...)    LOG_I(GTPU, x, ##args)
# define GTPU_WARNING(x, args...) LOG_W(GTPU, x, ##args)
# define GTPU_ERROR(x, args...)   LOG_E(GTPU, x, ##args)
#else
# define GTPU_DEBUG(x, args...)   fprintf(stdout, "[GTPU][D]"x, ##args)
# define GTPU_INFO(x, args...)    fprintf(stdout, "[GTPU][I]"x, ##args)
# define GTPU_WARNING(x, args...) fprintf(stdout, "[GTPU][W]"x, ##args)
# define GTPU_ERROR(x, args...)   fprintf(stderr, "[GTPU][E]"x, ##args)
#endif

//#warning "TO BE REFINED"
# define GTPU_HEADER_OVERHEAD_MAX 64

uint32_t gtpv1u_new_teid(void);

#endif /* GTPV1_U_H_ */
