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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file otg.h
* \brief Data structure and functions for OTG
* \author N. Nikaein and A. Hafsaoui
* \date 2011
* \version 1.0
* \company Eurecom
* \email:  navid.nikaein@eurecom.fr
* \note
* \warning
*/


#ifndef __OTG_H__
# define __OTG_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "otg_defs.h"
#include "otg_models.h"
#include "otg_config.h"


#if STANDALONE==1
#define LOG_G(c, x...) printf(x)
#define LOG_A(c, x...) printf(x)
#define LOG_C(c, x...) printf(x)
#define LOG_E(c, x...) printf(x)
#define LOG_W(c, x...) printf(x)
#define LOG_N(c, x...) printf(x)
#define LOG_I(c, x...) printf(x)
#define LOG_D(c, x...) printf(x)
#define LOG_F(c, x...) printf(x)
#define LOG_T(c, x...) printf(x)
typedef enum {MIN_NUM_COMPS=0, PHY, OMG, OCM, OTG, MAX_NUM_COMPS} comp_t;
#else
#include "../UTIL/LOG/log.h"
#endif


/*! \fn void init_all_otg(void)
* \brief set initial values de 0
* \param[in] void
* \param[out] void
* \note
* @ingroup  _otg
*/
void init_all_otg(int max_nb_frames);

/*! \fn void set_ctime(int ctime)
* \brief set the simulation time to the ctime
* \param[in] simulation time , void
* \param[out] void
* \note
* @ingroup  _otg
*/
void set_ctime(int ctime);


/*! \fn void get_ctime(void)
* \brief get the simulation time
* \param[in] int
* \param[out] void
* \note
* @ingroup  _otg
*/
int get_ctime(void);



/*! \fn void free_otg(void);
* \brief free OTG pointers
* \param[in]
* \param[out]
* \note
* @ingroup  _otg
*/
void free_otg(void);

/*! \fn str_sub (const char *s, unsigned int start, unsigned int end);
* \brief substract string
* \param[in] const char *s
* \param[in] unsigned int start
*  \param[in] unsigned int end
* \param[out]char *str_sub
* @ingroup  _otg
*/
char *str_sub (const char *s, unsigned int start, unsigned int end);


#endif
