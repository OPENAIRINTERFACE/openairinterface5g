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

