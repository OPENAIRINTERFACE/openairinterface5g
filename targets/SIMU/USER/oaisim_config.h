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

/*! \file oaisim_config.h
* \brief configure an emulation
* \author navid nikaein & Lusheng Wang
* \date 2006-2010
* \version 4.0
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note this a note
* \bug  this is a bug
* \warning  this is a warning
*/

//-----------------------------------begin group-----------------------------
/** @defgroup _oaisim The sturcture of OAISIM

The current sturcture of oaisim is shown by the figure.

\image html new_OCG_structure.png "new_OCG_structure"


 * @{*/

/* @}*/

#include "UTIL/LOG/log_if.h"
#include "UTIL/LOG/log_extern.h"
#include "UTIL/OCG/OCG.h"
#include "UTIL/OPT/opt.h" // to test OPT
#include "UTIL/OMG/omg.h"
#include "UTIL/CLI/cli_if.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SIMULATION/ETH_TRANSPORT/defs.h"
#include "PHY/defs.h"

/** @defgroup _init_oai Initial oaisim
 *  @ingroup _fn
 *  @brief Initialize all the parameters before start an emulation
 * @{*/
void init_oai_emulation(void);
/* @}*/

/** @defgroup _config_oaisim All the configurations for an emulation
 *  @ingroup _fn
 *  @brief This is the function that calls all the other configuration functions
 * @{*/
void oaisim_config(void);
/* @}*/


/** @defgroup _config_module Configuration functions for various modules
 *  @ingroup _fn
 *  @brief There are the functions to configure different various modules in the emulator
 * @{*/
int olg_config(void);
int ocg_config_env(void);
int ocg_config_omg(void);
int ocg_config_topo(void);
int ocg_config_app(void);
int ocg_config_emu(void);

int flow_start_time(int sid, int did, uint32_t n_frames, uint32_t start, uint32_t duration);
/* @}*/

