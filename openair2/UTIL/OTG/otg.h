/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
